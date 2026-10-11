#include "rate-decimator.h"

#include <algorithm>

namespace {

/* 48-tap Hamming-windowed sinc, cutoff 7200 Hz at 48000 Hz, DC gain 1.
 * h[0] multiplies the newest sample. Symmetric, so the passband is linear-phase. */
constexpr float kCoeffs[] = {
	-1.692305604e-04f, 8.399340695e-04f,  1.406472277e-03f,  8.198737428e-04f,  -1.066958772e-03f,
	-3.036733861e-03f, -2.827555527e-03f, 8.048357967e-04f,  5.826020970e-03f,  7.316255087e-03f,
	1.595785295e-03f,  -8.879478601e-03f, -1.515756451e-02f, -8.470769742e-03f, 1.027018522e-02f,
	2.708958207e-02f,  2.359437022e-02f,  -6.401223103e-03f, -4.536252076e-02f, -5.783929410e-02f,
	-1.350105009e-02f, 8.759359634e-02f,  2.073152124e-01f,  2.882402561e-01f,  2.882402561e-01f,
	2.073152124e-01f,  8.759359634e-02f,  -1.350105009e-02f, -5.783929410e-02f, -4.536252076e-02f,
	-6.401223103e-03f, 2.359437022e-02f,  2.708958207e-02f,  1.027018522e-02f,  -8.470769742e-03f,
	-1.515756451e-02f, -8.879478601e-03f, 1.595785295e-03f,  7.316255087e-03f,  5.826020970e-03f,
	8.048357967e-04f,  -2.827555527e-03f, -3.036733861e-03f, -1.066958772e-03f, 8.198737428e-04f,
	1.406472277e-03f,  8.399340695e-04f,  -1.692305604e-04f,
};

} /* namespace */

bool RateDecimator::decimate(int input_rate, int output_rate, const float *in, size_t count, std::vector<float> &out)
{
	static_assert(sizeof(kCoeffs) / sizeof(kCoeffs[0]) == kTaps, "FIR tap count");

	if (input_rate != kInputRate || output_rate != kOutputRate || !in || count == 0)
		return false;

	out.clear();
	if (out.capacity() < count / static_cast<size_t>(kFactor) + 1)
		out.reserve(count / static_cast<size_t>(kFactor) + 1);

	for (size_t n = 0; n < count; n++) {
		hist_[hist_pos_] = in[n];
		++hist_pos_;
		if (hist_pos_ == kTaps)
			hist_pos_ = 0;

		++phase_;
		if (phase_ < kFactor)
			continue;
		phase_ = 0;

		float y = 0.0f;
		int idx = hist_pos_ - 1;
		if (idx < 0)
			idx = kTaps - 1;
		for (int k = 0; k < kTaps; k++) {
			y += kCoeffs[k] * hist_[idx];
			--idx;
			if (idx < 0)
				idx = kTaps - 1;
		}
		out.push_back(y);
	}
	return true;
}

void RateDecimator::reset()
{
	std::fill(std::begin(hist_), std::end(hist_), 0.0f);
	hist_pos_ = 0;
	phase_ = 0;
}
