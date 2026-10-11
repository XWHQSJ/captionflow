#include "rate-decimator.h"
#include "test-framework.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

std::vector<float> tone(float freq_hz, int sample_rate, size_t frames, float amp)
{
	std::vector<float> out(frames);
	for (size_t i = 0; i < frames; i++) {
		const double t = static_cast<double>(i) / static_cast<double>(sample_rate);
		out[i] = amp * static_cast<float>(std::sin(2.0 * M_PI * freq_hz * t));
	}
	return out;
}

float rms_from(const std::vector<float> &samples, size_t start)
{
	double sum = 0.0;
	size_t n = 0;
	for (size_t i = start; i < samples.size(); i++) {
		sum += static_cast<double>(samples[i]) * static_cast<double>(samples[i]);
		++n;
	}
	if (n == 0)
		return 0.0f;
	return static_cast<float>(std::sqrt(sum / static_cast<double>(n)));
}

float upward_zero_cross_hz(const std::vector<float> &samples, size_t start, int sample_rate)
{
	int crossings = 0;
	for (size_t i = start + 1; i < samples.size(); i++) {
		if (samples[i - 1] <= 0.0f && samples[i] > 0.0f)
			++crossings;
	}
	const double seconds = static_cast<double>(samples.size() - start - 1) / static_cast<double>(sample_rate);
	if (seconds <= 0.0)
		return 0.0f;
	return static_cast<float>(static_cast<double>(crossings) / seconds);
}

} /* namespace */

TEST(RateDecimator, UnsupportedRateLeavesOutput)
{
	RateDecimator decimator;
	std::vector<float> out{1.0f, 2.0f};
	const float in[8] = {};
	EXPECT_FALSE(decimator.decimate(44100, RateDecimator::kOutputRate, in, 8, out));
	ASSERT_EQ(out.size(), 2u);
	EXPECT_EQ(out[0], 1.0f);
	EXPECT_EQ(out[1], 2.0f);
}

TEST(RateDecimator, DecimatesExactMultipleOfThree)
{
	RateDecimator decimator;
	const std::vector<float> in(4800, 0.25f);
	std::vector<float> out;
	EXPECT_TRUE(
		decimator.decimate(RateDecimator::kInputRate, RateDecimator::kOutputRate, in.data(), in.size(), out));
	EXPECT_EQ(out.size(), 1600u);
}

TEST(RateDecimator, PassesDc)
{
	RateDecimator decimator;
	const std::vector<float> in(4800, 1.0f);
	std::vector<float> out;
	EXPECT_TRUE(
		decimator.decimate(RateDecimator::kInputRate, RateDecimator::kOutputRate, in.data(), in.size(), out));
	ASSERT_TRUE(out.size() > 64u);
	double sum = 0.0;
	size_t n = 0;
	for (size_t i = 16; i < out.size(); i++) {
		sum += out[i];
		++n;
	}
	EXPECT_NEAR(sum / static_cast<double>(n), 1.0, 1e-3);
}

TEST(RateDecimator, PreservesOneKilohertz)
{
	RateDecimator decimator;
	const auto in = tone(1000.0f, RateDecimator::kInputRate, 48000, 0.5f);
	std::vector<float> out;
	EXPECT_TRUE(
		decimator.decimate(RateDecimator::kInputRate, RateDecimator::kOutputRate, in.data(), in.size(), out));
	EXPECT_NEAR(rms_from(out, 32), 0.353f, 0.02f);
	EXPECT_NEAR(upward_zero_cross_hz(out, 32, RateDecimator::kOutputRate), 1000.0f, 5.0f);
}

TEST(RateDecimator, AttenuatesAboveNewNyquist)
{
	RateDecimator decimator;
	const auto in = tone(12000.0f, RateDecimator::kInputRate, 48000, 0.5f);
	std::vector<float> out;
	EXPECT_TRUE(
		decimator.decimate(RateDecimator::kInputRate, RateDecimator::kOutputRate, in.data(), in.size(), out));
	EXPECT_LT(rms_from(out, 32), 0.01f);
}

TEST(RateDecimator, ChunksMatchBlock)
{
	const auto in = tone(440.0f, RateDecimator::kInputRate, 4800, 0.4f);
	RateDecimator block;
	std::vector<float> whole;
	EXPECT_TRUE(block.decimate(RateDecimator::kInputRate, RateDecimator::kOutputRate, in.data(), in.size(), whole));

	RateDecimator streamed;
	std::vector<float> chunk_out;
	std::vector<float> all;
	for (size_t offset = 0; offset < in.size(); offset += 100) {
		const size_t n = std::min<size_t>(100, in.size() - offset);
		EXPECT_TRUE(streamed.decimate(RateDecimator::kInputRate, RateDecimator::kOutputRate, in.data() + offset,
					      n, chunk_out));
		all.insert(all.end(), chunk_out.begin(), chunk_out.end());
	}
	ASSERT_EQ(all.size(), whole.size());
	for (size_t i = 0; i < whole.size(); i++)
		EXPECT_EQ(all[i], whole[i]);
}

TEST(RateDecimator, QueuedFramesScaleToInputRate)
{
	EXPECT_EQ(queued_input_frames(160, 48000, 16000), 480u);
	EXPECT_EQ(queued_input_frames(0, 48000, 16000), 0u);
	EXPECT_EQ(queued_input_frames(10, 44100, 44100), 10u);
}
