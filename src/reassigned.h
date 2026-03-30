/*
 *  Copyright (C) 2026, Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 *  Reassigned and synchrosqueezed spectrogram for inspectrum ng.
 *
 *  Reassigned: 3 FFTs per column (h, t*h, h'). Energy redistributed
 *  to local centroids in both time and frequency.
 *
 *  Synchrosqueezed: 2 FFTs per column (h, h'). Energy redistributed
 *  in frequency only (preserves time grid, invertible transform).
 *
 *  Reference: Auger & Flandrin, "Improving the readability of
 *  time-frequency and time-scale representations by the reassignment
 *  method," IEEE TSP 1995.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#pragma once

#include <cstddef>

enum class TFRMode {
	Standard = 0,
	Reassigned,
	Synchrosqueezed,
	TFRModeCount
};

/*
 * Compute a reassigned spectrogram tile.
 *
 * Output is column-major (same layout as standard FFT tiles):
 *   dest[col * fftSize + bin] = power in dB.
 * Empty bins are set to -200 dB (floor).
 *
 * @param mode        Reassigned or Synchrosqueezed
 * @param samples     raw IQ samples (caller reads from InputSource)
 * @param sampleCount total samples available in buffer
 * @param windowSize  analysis window length (samples)
 * @param fftSize     FFT length (>= windowSize, includes zero-pad)
 * @param window      standard window coefficients (windowSize floats)
 * @param stride      hop size between columns (samples)
 * @param nCols       number of output columns to compute
 * @param thresholdDB energy threshold: bins below (peak - threshold) dB
 *                    are not reassigned (prevents noise scatter)
 * @param dest        output array, nCols * fftSize floats (column-major)
 */
void computeReassignedTile(TFRMode mode,
			   const float *samples,
			   size_t sampleCount,
			   int windowSize, int fftSize,
			   const float *window,
			   int stride, int nCols,
			   float thresholdDB,
			   float *dest);

/*
 * Generate the derivative window h'(t) from h(t) using central differences.
 * dest must hold windowSize floats.
 */
void generateDerivativeWindow(const float *window, int windowSize, float *dest);

/*
 * Generate the time-ramped window t*h(t).
 * t is centered: t[i] = (i - windowSize/2).
 * dest must hold windowSize floats.
 */
void generateTimeRampedWindow(const float *window, int windowSize, float *dest);

/* Display name for UI combo box */
const char *tfrModeName(TFRMode mode);

static inline int tfrModeCount()
{
	return static_cast<int>(TFRMode::TFRModeCount);
}
