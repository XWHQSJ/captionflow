#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

/* Streaming 48 kHz → 16 kHz lowpass decimator.
 *
 * The FIR is a 48-tap linear-phase sinc designed for this rate pair only.
 * State carries across calls. Not thread-safe.
 *
 * decimate() returns false when the rate pair is not supported. The output
 * vector is left untouched in that case so the caller can pass the original
 * samples through (sherpa-onnx resamples those itself).
 */
class RateDecimator {
public:
	static constexpr int kInputRate = 48000;
	static constexpr int kOutputRate = 16000;
	static constexpr int kFactor = 3;

	bool decimate(int input_rate, int output_rate, const float *in, size_t count, std::vector<float> &out);
	void reset();

private:
	static constexpr int kTaps = 48;

	float hist_[kTaps]{};
	int hist_pos_ = 0;
	int phase_ = 0;
};

/* Samples stored at `stored_rate`, expressed in `input_rate` frames.
 * Used so mute timing (which counts OBS frames) can subtract the ASR backlog
 * after 48 kHz audio has been decimated into the ring. */
inline size_t queued_input_frames(size_t queued, int input_rate, int stored_rate)
{
	if (queued == 0 || input_rate <= 0 || stored_rate <= 0 || input_rate == stored_rate)
		return queued;
	return queued * static_cast<size_t>(input_rate) / static_cast<size_t>(stored_rate);
}
