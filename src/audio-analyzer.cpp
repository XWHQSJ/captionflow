#include "audio-analyzer.h"
#include "constants.h"

#include <cmath>

using namespace ai_caption::constants;

void AudioAnalyzer::reset()
{
	rms_ = 0.0f;
	freq_ = 200.0f;
	sample_rate_ = 0;
	analysis_rate_ = 0;
	pitch_buf_.clear();
	linear_buf_.clear();
	window_size_ = 0;
	pitch_write_ = 0;
	update_counter_ = 0;
	min_lag_ = 0;
	max_lag_ = 0;
	decim_phase_ = 0;
	decim_count_ = 0;
	decim_sum_ = 0.0f;
}

void AudioAnalyzer::configure_for_rate(uint32_t sample_rate)
{
	sample_rate_ = sample_rate;
	analysis_rate_ = sample_rate < kPitchAnalysisRate ? sample_rate : kPitchAnalysisRate;
	window_size_ = static_cast<size_t>(analysis_rate_) * kAnalyzerWindowMs / 1000;
	if (window_size_ < 2)
		window_size_ = 2;
	pitch_buf_.assign(window_size_, 0.0f);
	linear_buf_.assign(window_size_, 0.0f);
	pitch_write_ = 0;
	update_counter_ = 0;
	decim_phase_ = 0;
	decim_count_ = 0;
	decim_sum_ = 0.0f;
	min_lag_ = static_cast<size_t>(static_cast<float>(analysis_rate_) / kMaxPitchHz);
	max_lag_ = static_cast<size_t>(static_cast<float>(analysis_rate_) / kMinPitchHz);
	if (min_lag_ < 1)
		min_lag_ = 1;
	if (max_lag_ >= window_size_)
		max_lag_ = window_size_ - 1;
}

void AudioAnalyzer::push_analysis(float sample)
{
	pitch_buf_[pitch_write_] = sample;
	++pitch_write_;
	if (pitch_write_ == window_size_)
		pitch_write_ = 0;
}

void AudioAnalyzer::feed(const float *mono, size_t frames, uint32_t sample_rate)
{
	if (frames == 0 || !mono || sample_rate == 0)
		return;

	if (sample_rate != sample_rate_ || pitch_buf_.empty())
		configure_for_rate(sample_rate);

	float sum_sq = 0.0f;
	for (size_t i = 0; i < frames; i++)
		sum_sq += mono[i] * mono[i];
	const float frame_rms = std::sqrt(sum_sq / static_cast<float>(frames));
	rms_ = rms_ * (1.0f - kRmsSmoothingAlpha) + frame_rms * kRmsSmoothingAlpha;

	if (analysis_rate_ == sample_rate_) {
		for (size_t i = 0; i < frames; i++)
			push_analysis(mono[i]);
	} else if (analysis_rate_ > 0) {
		for (size_t i = 0; i < frames; i++) {
			decim_sum_ += mono[i];
			++decim_count_;
			decim_phase_ += analysis_rate_;
			if (decim_phase_ >= sample_rate_) {
				decim_phase_ -= sample_rate_;
				const float y = decim_sum_ / static_cast<float>(decim_count_);
				decim_sum_ = 0.0f;
				decim_count_ = 0;
				push_analysis(y);
			}
		}
	}

	update_counter_ += frames;
	const size_t interval = static_cast<size_t>(sample_rate_) / static_cast<size_t>(kPitchUpdateDivisor);
	if (interval > 0 && update_counter_ >= interval) {
		update_counter_ = 0;
		detect_pitch();
	}
}

void AudioAnalyzer::detect_pitch()
{
	if (min_lag_ >= max_lag_ || window_size_ < max_lag_ + 1 || analysis_rate_ == 0)
		return;

	/* Oldest sample sits at pitch_write_. Linearize without a division. */
	for (size_t i = 0; i < window_size_; i++) {
		const size_t idx = pitch_write_ + i;
		linear_buf_[i] = pitch_buf_[idx < window_size_ ? idx : idx - window_size_];
	}

	float energy = 0.0f;
	for (size_t i = 0; i < window_size_; i++)
		energy += linear_buf_[i] * linear_buf_[i];

	if (energy < 1e-8f)
		return;

	const auto corr_at = [&](size_t lag) {
		float corr = 0.0f;
		const size_t n = window_size_ - lag;
		const float *samples = linear_buf_.data();
		for (size_t i = 0; i < n; i++)
			corr += samples[i] * samples[i + lag];
		return corr / energy;
	};

	float best_corr = 0.0f;
	size_t best_lag = 0;
	for (size_t lag = min_lag_; lag <= max_lag_; lag++) {
		const float corr = corr_at(lag);
		if (corr > best_corr) {
			best_corr = corr;
			best_lag = lag;
		}
	}

	if (best_corr <= kPitchCorrelationThreshold || best_lag == 0)
		return;

	float delta = 0.0f;
	if (best_lag > min_lag_ && best_lag < max_lag_) {
		const float c_prev = corr_at(best_lag - 1);
		const float c_next = corr_at(best_lag + 1);
		const float denom = (c_prev - 2.0f * best_corr) + c_next;
		if (std::fabs(denom) > 1e-12f) {
			delta = 0.5f * (c_prev - c_next) / denom;
			if (delta > 0.5f)
				delta = 0.5f;
			else if (delta < -0.5f)
				delta = -0.5f;
		}
	}

	const float period = static_cast<float>(best_lag) + delta;
	const float detected = static_cast<float>(analysis_rate_) / period;
	freq_ = freq_ * (1.0f - kPitchSmoothingAlpha) + detected * kPitchSmoothingAlpha;
}
