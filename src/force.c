#include <stdint.h>
#include "force.h"


int32_t Force_Filter(int32_t measure)
{
	// The control host can do its own filtering on the raw data obviously,
	// but this filtering is built-in so we have a canonical low-pass-filtered
	// signal to use for out-of-range detection.

	// Single pole IIR filter. Let's use that hardware floating point!
	// Assuming 80 Hz sampling rate, and frequencies of interest < 4 Hz or so.

	const float decay = 0.97f;
	static float state;
	state += (1.0f - decay) * (measure - (float)state);
	return (int32_t)state;
}
