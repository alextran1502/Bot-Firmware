#include <stdint.h>
#include "force.h"


float Force_Filter(int32_t measure)
{
	// The control host can do its own filtering on the raw data obviously,
	// but this filtering is built-in so we have a canonical low-pass-filtered
	// signal to use for out-of-range detection.

	// Single pole IIR filter. Let's use that hardware floating point!
	// Assuming 80 Hz sampling rate, and frequencies of interest < 4 Hz or so.

	// Enable the filter a short time after boot, so that we give the HX711 some
	// time to stabilize itself before we enable the slower filter. This avoids
	// a long transient toward zero at system reset.

	const float decay = 0.97f;
	static float state;
	static unsigned init = 20;

	if (init) {
		init--;
		state = (float)measure;
	} else {
		state += (1.0f - decay) * ((float)measure - state);
	}

	return state;
}
