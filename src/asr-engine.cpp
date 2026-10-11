#include "asr-engine.h"
#include "constants.h"

#include <sherpa-onnx/c-api/c-api.h>

#include <cstdio>
#include <cstring>
#include <exception>
#include <memory>
#include <vector>

using namespace ai_caption::constants;

/* -------------------------------------------------------- */
/* AsrEngine::Impl                                          */

struct AsrEngine::Impl {
	const SherpaOnnxOnlineRecognizer *recognizer = nullptr;
	const SherpaOnnxOnlineStream *stream = nullptr;
	AsrConfig config;
};

AsrEngine::AsrEngine() : impl_(std::make_unique<Impl>()), ring_(kAsrRingCapacity) {}

AsrEngine::~AsrEngine()
{
	shutdown();
}

bool AsrEngine::init(const AsrConfig &config)
{
	if (running_.load(std::memory_order_acquire))
		shutdown();

	shutting_down_.store(false, std::memory_order_release);
	impl_->config = config;

	SherpaOnnxOnlineRecognizerConfig c;
	std::memset(&c, 0, sizeof(c));

	c.feat_config.sample_rate = config.sample_rate;
	c.feat_config.feature_dim = config.feature_dim;

	c.model_config.transducer.encoder = config.encoder_path.c_str();
	c.model_config.transducer.decoder = config.decoder_path.c_str();
	c.model_config.transducer.joiner = config.joiner_path.c_str();
	c.model_config.tokens = config.tokens_path.c_str();
	c.model_config.num_threads = config.num_threads;
	c.model_config.provider = config.provider.c_str();
	/* debug=1 logs on the decode path. Leave it off in the plugin. */
	c.model_config.debug = 0;

	c.model_config.model_type = "";
	c.model_config.modeling_unit = config.modeling_unit.c_str();
	c.model_config.bpe_vocab = config.bpe_vocab_path.empty() ? "" : config.bpe_vocab_path.c_str();

	c.decoding_method = config.decoding_method.c_str();
	c.max_active_paths = config.max_active_paths;

	c.enable_endpoint = config.enable_endpoint ? 1 : 0;
	c.rule1_min_trailing_silence = config.rule1_min_trailing_silence;
	c.rule2_min_trailing_silence = config.rule2_min_trailing_silence;
	c.rule3_min_utterance_length = config.rule3_min_utterance_length;

	c.hotwords_file = "";
	c.hotwords_score = config.hotwords_score;
	c.hotwords_buf = nullptr;
	c.hotwords_buf_size = 0;
	c.rule_fsts = "";
	c.rule_fars = "";

	if (!config.hotwords_buf.empty()) {
		c.hotwords_buf = config.hotwords_buf.c_str();
		c.hotwords_buf_size = static_cast<int32_t>(config.hotwords_buf.size());
	} else if (!config.hotwords_file.empty()) {
		c.hotwords_file = config.hotwords_file.c_str();
	}

	const SherpaOnnxOnlineRecognizer *rec = nullptr;
	try {
		rec = SherpaOnnxCreateOnlineRecognizer(&c);
	} catch (const std::exception &e) {
		std::fprintf(stderr, "[CaptionFlow] CreateOnlineRecognizer failed: %s\n", e.what());
		return false;
	} catch (...) {
		std::fprintf(stderr, "[CaptionFlow] CreateOnlineRecognizer failed: unknown exception\n");
		return false;
	}

	if (!rec)
		return false;

	const SherpaOnnxOnlineStream *strm = SherpaOnnxCreateOnlineStream(rec);
	if (!strm) {
		SherpaOnnxDestroyOnlineRecognizer(rec);
		return false;
	}

	{
		std::lock_guard<std::mutex> lock(onnx_mutex_);
		impl_->recognizer = rec;
		impl_->stream = strm;
	}

	ring_.reset();
	target_sample_rate_.store(config.sample_rate > 0 ? config.sample_rate : 16000, std::memory_order_relaxed);
	running_.store(true, std::memory_order_release);
	decode_thread_ = std::thread(&AsrEngine::decode_thread_func, this);
	return true;
}

void AsrEngine::shutdown()
{
	bool expected = false;
	if (!shutting_down_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
		return;

	running_.store(false, std::memory_order_release);
	ring_.shutdown();

	if (decode_thread_.joinable())
		decode_thread_.join();

	{
		std::lock_guard<std::mutex> lock(onnx_mutex_);
		if (impl_->stream) {
			SherpaOnnxDestroyOnlineStream(impl_->stream);
			impl_->stream = nullptr;
		}
		if (impl_->recognizer) {
			SherpaOnnxDestroyOnlineRecognizer(impl_->recognizer);
			impl_->recognizer = nullptr;
		}
	}

	ring_.reset();
	last_text_.clear();
	reset_stats();

	/* Release shutting_down so subsequent init() can take the CAS path
	 * cleanly. ring_.reset() above also clears its shutdown flag. */
	shutting_down_.store(false, std::memory_order_release);
}

void AsrEngine::set_result_callback(AsrResultCallback cb)
{
	std::lock_guard<std::mutex> lk(callback_mutex_);
	result_callback_ = std::move(cb);
}

void AsrEngine::feed_audio(const float *samples, size_t count, int sample_rate)
{
	if (!running_.load(std::memory_order_acquire) || count == 0 || !samples)
		return;

	if (sample_rate <= 0)
		sample_rate = 16000;

	if (sample_rate != active_input_rate_) {
		decimator_.reset();
		active_input_rate_ = sample_rate;
	}

	const int target = target_sample_rate_.load(std::memory_order_relaxed);
	if (target > 0 && decimator_.decimate(sample_rate, target, samples, count, resample_buf_)) {
		/* Publish the rate pair before the ring release so a consumer
		 * that observes the new samples also observes the rates. */
		input_sample_rate_.store(sample_rate, std::memory_order_relaxed);
		source_sample_rate_.store(target, std::memory_order_relaxed);
		if (resample_buf_.empty())
			return;
		const size_t written = ring_.write(resample_buf_.data(), resample_buf_.size());
		if (written < resample_buf_.size()) {
			const size_t missed = resample_buf_.size() - written;
			const size_t scale = static_cast<size_t>(sample_rate) / static_cast<size_t>(target);
			dropped_samples_.fetch_add(missed * (scale == 0 ? 1 : scale), std::memory_order_relaxed);
		}
		return;
	}

	input_sample_rate_.store(sample_rate, std::memory_order_relaxed);
	source_sample_rate_.store(sample_rate, std::memory_order_relaxed);
	const size_t written = ring_.write(samples, count);
	if (written < count)
		dropped_samples_.fetch_add(count - written, std::memory_order_relaxed);
}

void AsrEngine::reset()
{
	should_reset_.store(true, std::memory_order_release);
}

AsrStats AsrEngine::get_stats() const
{
	return AsrStats{dropped_samples_.load(std::memory_order_relaxed),
			decode_errors_.load(std::memory_order_relaxed)};
}

void AsrEngine::reset_stats()
{
	dropped_samples_.store(0, std::memory_order_relaxed);
	decode_errors_.store(0, std::memory_order_relaxed);
}

/* -------------------------------------------------------- */
/* Decode thread                                            */

void AsrEngine::decode_thread_func()
{
	std::vector<float> chunk(kDecodeChunkSamples);
	uint64_t segment_samples_fed = 0;

	while (running_.load(std::memory_order_acquire)) {
		if (should_reset_.load(std::memory_order_acquire)) {
			should_reset_.store(false, std::memory_order_release);
			{
				std::lock_guard<std::mutex> lock(onnx_mutex_);
				if (impl_->recognizer && impl_->stream)
					SherpaOnnxOnlineStreamReset(impl_->recognizer, impl_->stream);
			}
			ring_.clear();
			last_text_.clear();
			segment_samples_fed = 0;
			continue;
		}

		ring_.wait_for_data(kRingWaitTimeoutMs);
		if (!running_.load(std::memory_order_acquire))
			break;

		const size_t frames_read = ring_.read(chunk.data(), kDecodeChunkSamples);
		if (frames_read == 0)
			continue;

		try {
			std::string text;
			bool have_update = false;
			bool is_partial = true;
			float token_age_sec = 0.0f;

			{
				std::lock_guard<std::mutex> lock(onnx_mutex_);

				if (shutting_down_.load(std::memory_order_acquire))
					break;
				if (!impl_->recognizer || !impl_->stream)
					break;

				SherpaOnnxOnlineStreamAcceptWaveform(
					impl_->stream, source_sample_rate_.load(std::memory_order_relaxed),
					chunk.data(), static_cast<int32_t>(frames_read));

				segment_samples_fed += frames_read;

				while (SherpaOnnxIsOnlineStreamReady(impl_->recognizer, impl_->stream))
					SherpaOnnxDecodeOnlineStream(impl_->recognizer, impl_->stream);

				using ResultPtr = std::unique_ptr<const SherpaOnnxOnlineRecognizerResult,
								  decltype(&SherpaOnnxDestroyOnlineRecognizerResult)>;
				ResultPtr result(SherpaOnnxGetOnlineStreamResult(impl_->recognizer, impl_->stream),
						 &SherpaOnnxDestroyOnlineRecognizerResult);

				if (result && result->text && result->text[0] != '\0') {
					const bool is_endpoint =
						SherpaOnnxOnlineStreamIsEndpoint(impl_->recognizer, impl_->stream) != 0;

					if (is_endpoint || last_text_ != result->text) {
						text.assign(result->text);
						have_update = true;
						is_partial = !is_endpoint;
						last_text_ = text;

						if (result->timestamps && result->count > 0) {
							const float last_token_time =
								result->timestamps[result->count - 1];
							const int sr =
								source_sample_rate_.load(std::memory_order_relaxed);
							const float segment_fed_time =
								sr > 0 ? static_cast<float>(segment_samples_fed) /
										 static_cast<float>(sr)
								       : 0.0f;
							token_age_sec = segment_fed_time - last_token_time;
							if (token_age_sec < 0.0f)
								token_age_sec = 0.0f;
						}

						if (is_endpoint) {
							SherpaOnnxOnlineStreamReset(impl_->recognizer, impl_->stream);
							last_text_.clear();
							segment_samples_fed = 0;
						}
					}
				}
			}

			if (have_update) {
				AsrResultCallback cb;
				{
					std::lock_guard<std::mutex> cb_lock(callback_mutex_);
					cb = result_callback_;
				}
				if (cb)
					cb(text, is_partial, token_age_sec);
			}

		} catch (const std::exception &e) {
			const uint64_t n = decode_errors_.fetch_add(1, std::memory_order_relaxed) + 1;
			if (n <= 3 || (n % 100) == 0)
				std::fprintf(stderr, "[CaptionFlow] sherpa-onnx decode exception: %s (total=%llu)\n",
					     e.what(), static_cast<unsigned long long>(n));
		} catch (...) {
			const uint64_t n = decode_errors_.fetch_add(1, std::memory_order_relaxed) + 1;
			if (n <= 3 || (n % 100) == 0)
				std::fprintf(stderr,
					     "[CaptionFlow] sherpa-onnx decode unknown exception (total=%llu)\n",
					     static_cast<unsigned long long>(n));
		}
	}
}
