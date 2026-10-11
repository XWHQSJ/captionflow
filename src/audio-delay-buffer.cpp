#include "audio-delay-buffer.h"

#include <algorithm>
#include <array>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

constexpr int kSineTableSize = 1024;

const float *sine_table()
{
	static const std::array<float, kSineTableSize> table = [] {
		std::array<float, kSineTableSize> values{};
		for (int i = 0; i < kSineTableSize; i++) {
			const double phase = 2.0 * M_PI * static_cast<double>(i) / static_cast<double>(kSineTableSize);
			values[static_cast<size_t>(i)] = static_cast<float>(std::sin(phase));
		}
		return values;
	}();
	return table.data();
}

} /* namespace */

void BeepGenerator::set_params(uint32_t sample_rate, float freq, float volume)
{
	if (sample_rate == 0)
		sample_rate = 1; /* guard against divide-by-zero */
	if (sample_rate != sample_rate_) {
		sample_rate_ = sample_rate;
		phase_ = 0.0;
	}
	freq_ = freq;
	volume_ = volume;
	phase_inc_ = 2.0 * M_PI * static_cast<double>(freq_) / static_cast<double>(sample_rate_);
}

float BeepGenerator::next()
{
	const double scaled = phase_ * (static_cast<double>(kSineTableSize) / (2.0 * M_PI));
	const int i0_raw = static_cast<int>(scaled);
	const float frac = static_cast<float>(scaled - static_cast<double>(i0_raw));
	const int i0 = i0_raw & (kSineTableSize - 1);
	const int i1 = (i0 + 1) & (kSineTableSize - 1);
	const float *table = sine_table();
	const float s = table[i0] + frac * (table[i1] - table[i0]);

	phase_ += phase_inc_;
	/* Normalize phase into [0, 2π) using fmod to remain correct even if
	 * phase_inc_ exceeds 2π (pathological freq/rate combo). */
	if (phase_ >= 2.0 * M_PI)
		phase_ = std::fmod(phase_, 2.0 * M_PI);
	return volume_ * s;
}

void AudioDelayBuffer::reset()
{
	if (!buf_.empty())
		std::fill(buf_.begin(), buf_.end(), 0.0f);
	write_pos_ = 0;
	filled_ = 0;
}

void AudioDelayBuffer::configure(uint32_t sample_rate, size_t channels, uint32_t delay_ms)
{
	/* Resize / clear only when the effective shape changes, to avoid wiping
	 * the ring on every audio callback. Any of sample_rate / channels /
	 * delay_ms changing resets state to avoid leaking stale frames. */
	if (sample_rate == sample_rate_ && channels == channels_ && delay_ms == delay_ms_)
		return;

	sample_rate_ = sample_rate;
	channels_ = channels;
	delay_ms_ = delay_ms;

	const size_t delay_samples = static_cast<size_t>(sample_rate) * delay_ms / 1000;
	delay_frames_ = delay_samples;
	const size_t total = delay_samples * channels;
	buf_.assign(total, 0.0f);
	write_pos_ = 0;
	filled_ = 0;
}

void AudioDelayBuffer::process(float **channel_data, uint32_t frames, const std::vector<uint8_t> &mute_flags,
			       BeepGenerator &beep)
{
	if (!is_configured())
		return;
	for (uint32_t i = 0; i < frames; i++) {
		const bool beep_active = (i < mute_flags.size()) && mute_flags[i] != 0;
		const float beep_sample = beep_active ? beep.next() : 0.0f;
		for (size_t ch = 0; ch < channels_; ch++) {
			if (!channel_data[ch])
				continue;
			const size_t idx = write_pos_ * channels_ + ch;
			const float delayed = (filled_ >= delay_frames_) ? buf_[idx] : 0.0f;
			buf_[idx] = channel_data[ch][i];
			channel_data[ch][i] = beep_active ? beep_sample : delayed;
		}
		write_pos_ = (write_pos_ + 1) % delay_frames_;
		if (filled_ < delay_frames_)
			filled_++;
	}
}
