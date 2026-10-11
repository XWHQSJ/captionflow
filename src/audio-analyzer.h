#pragma once

#include <cstdint>
#include <vector>

/* Real-time audio analyzer: smoothed RMS plus fundamental frequency (F0).
 *
 * RMS uses the full input rate. Pitch is a normalized autocorrelation on an
 * 8 kHz downsample of a 30 ms window — enough for an 80–500 Hz voice, and
 * cheap enough to run on the OBS audio callback.
 *
 * NOT thread-safe. Feed and read from the same thread (OBS audio callback).
 */
class AudioAnalyzer {
public:
	void feed(const float *mono, size_t frames, uint32_t sample_rate);

	float get_rms() const { return rms_; }
	float get_freq() const { return freq_; }

	/* Visible for tests. */
	void reset();

private:
	void configure_for_rate(uint32_t sample_rate);
	void push_analysis(float sample);
	void detect_pitch();

	float rms_ = 0.0f;
	float freq_ = 200.0f;
	uint32_t sample_rate_ = 0;
	uint32_t analysis_rate_ = 0;

	std::vector<float> pitch_buf_;
	std::vector<float> linear_buf_;
	size_t window_size_ = 0;
	size_t pitch_write_ = 0;
	size_t update_counter_ = 0;
	size_t min_lag_ = 0;
	size_t max_lag_ = 0;

	uint32_t decim_phase_ = 0;
	uint32_t decim_count_ = 0;
	float decim_sum_ = 0.0f;
};
