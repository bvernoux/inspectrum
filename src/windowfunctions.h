/*
 *  Window function library for inspectrum ng.
 *  Precomputed coefficient generation for common spectral windows.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#pragma once

enum class WindowType {
	Hann = 0,
	Hamming,
	BlackmanHarris,
	Kaiser,
	FlatTop,
	Rectangular,
	WindowTypeCount
};

/*
 * Generate window coefficients.
 *
 * @param type   window type
 * @param size   number of coefficients (= FFT window size)
 * @param dest   output array (must hold at least @size floats)
 * @param beta   Kaiser beta parameter (ignored for other types)
 */
void generateWindow(WindowType type, int size, float *dest, float beta = 6.0f);

/* Display name for UI combo box */
const char *windowTypeName(WindowType type);

static inline int windowTypeCount()
{
	return static_cast<int>(WindowType::WindowTypeCount);
}
