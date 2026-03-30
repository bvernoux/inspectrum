/*
 *  Copyright (C) 2026, Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 *  Noise floor estimation and subtraction for inspectrum ng.
 *  Per-frequency-bin noise floor estimation with multiple methods.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#pragma once

#include <cstddef>

enum class NoiseFloorMethod {
	Off = 0,
	SubtractMedian,
	SubtractPercentile,
	Normalize,
	NoiseFloorMethodCount
};

/*
 * Estimate the noise floor per frequency bin.
 *
 * Data is column-major: data[col * n_freq + bin].
 * Result is n_freq floats (dB values, one per frequency bin).
 *
 * @param data        spectrogram data in dB (column-major)
 * @param n_freq      number of frequency bins per column
 * @param n_time      number of time columns
 * @param method      estimation method
 * @param dest        output noise floor (n_freq floats)
 * @param percentile  percentile for SubtractPercentile (1..50, default 20)
 */
void estimateNoiseFloor(const float *data, int n_freq, int n_time,
			NoiseFloorMethod method, float *dest,
			int percentile = 20);

/*
 * Apply noise floor correction to spectrogram data (in-place).
 *
 * @param data        spectrogram data in dB (column-major), modified in-place
 * @param n_freq      number of frequency bins per column
 * @param n_time      number of time columns
 * @param method      correction method (determines subtract vs normalize)
 * @param noiseFloor  per-bin noise floor (n_freq floats from estimateNoiseFloor)
 */
void applyNoiseFloor(float *data, int n_freq, int n_time,
		     NoiseFloorMethod method, const float *noiseFloor);

/* Display name for UI combo box */
const char *noiseFloorMethodName(NoiseFloorMethod method);

static inline int noiseFloorMethodCount()
{
	return static_cast<int>(NoiseFloorMethod::NoiseFloorMethodCount);
}
