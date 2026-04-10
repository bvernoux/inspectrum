/*
 *  Copyright (C) 2026, Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 *  Noise floor estimation and subtraction for inspectrum ng.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include "noisefloor.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

/*
 * nth_element partial sort to find the value at a given rank.
 * Caller provides a scratch buffer to avoid per-call allocation.
 */
static float findPercentile(float *tmp, int count, float pct)
{
	if (count <= 0)
		return 0.0f;
	if (count == 1)
		return tmp[0];

	float rank = pct * 0.01f * (count - 1);
	int lo = (int)rank;
	float frac = rank - lo;

	std::nth_element(tmp, tmp + lo, tmp + count);
	float vlo = tmp[lo];

	if (frac > 0.0f) {
		/* After nth_element for lo, all elements [lo+1..count) are
		 * >= vlo.  The (lo+1)th value is the min of that partition.
		 * Use linear scan for small remainders, nth_element for large. */
		int remaining = count - lo - 1;
		float vhi;
		if (remaining <= 64)
			vhi = *std::min_element(tmp + lo + 1, tmp + count);
		else {
			std::nth_element(tmp + lo + 1, tmp + lo + 1, tmp + count);
			vhi = tmp[lo + 1];
		}
		return vlo + frac * (vhi - vlo);
	}
	return vlo;
}

void estimateNoiseFloor(const float *data, int n_freq, int n_time,
			NoiseFloorMethod method, float *dest,
			int percentile)
{
	if (n_freq <= 0 || n_time <= 0)
		return;

	float pct;
	switch (method) {
	case NoiseFloorMethod::SubtractMedian:
		pct = 50.0f;
		break;
	case NoiseFloorMethod::SubtractPercentile:
		pct = (float)std::max(1, std::min(percentile, 50));
		break;
	case NoiseFloorMethod::Normalize:
		pct = 50.0f; /* median for normalize mode too */
		break;
	default:
		memset(dest, 0, n_freq * sizeof(float));
		return;
	}

	/* Single scratch buffer reused across all frequency bins
	 * (avoids n_freq heap allocations). */
	std::vector<float> binValues(n_time);

	/* Gather values for each frequency bin across all time columns.
	 * Data layout is column-major: data[col * n_freq + bin]. */
	for (int f = 0; f < n_freq; f++) {
		for (int t = 0; t < n_time; t++)
			binValues[t] = data[(size_t)t * n_freq + f];

		dest[f] = findPercentile(binValues.data(), n_time, pct);
	}
}

void applyNoiseFloor(float *data, int n_freq, int n_time,
		     NoiseFloorMethod method, const float *noiseFloor)
{
	if (n_freq <= 0 || n_time <= 0)
		return;

	switch (method) {
	case NoiseFloorMethod::SubtractMedian:
	case NoiseFloorMethod::SubtractPercentile:
		/*
		 * Subtract in dB domain.
		 * Result: 0 dB = noise floor, positive = signal above noise.
		 */
		for (int t = 0; t < n_time; t++) {
			float *col = data + (size_t)t * n_freq;
			for (int f = 0; f < n_freq; f++)
				col[f] -= noiseFloor[f];
		}
		break;

	case NoiseFloorMethod::Normalize:
		/*
		 * Normalize: divide in linear domain.
		 * In dB: S_out = S_in - noise (same as subtract, but
		 * conceptually represents S_linear / noise_linear).
		 * The difference is in how the power range is interpreted:
		 * result centered on 0 dB = noise floor.
		 */
		for (int t = 0; t < n_time; t++) {
			float *col = data + (size_t)t * n_freq;
			for (int f = 0; f < n_freq; f++)
				col[f] -= noiseFloor[f];
		}
		break;

	default:
		break;
	}
}

const char *noiseFloorMethodName(NoiseFloorMethod method)
{
	switch (method) {
	case NoiseFloorMethod::Off:                 return "Off";
	case NoiseFloorMethod::SubtractMedian:      return "Subtract (median)";
	case NoiseFloorMethod::SubtractPercentile:  return "Subtract (percentile)";
	case NoiseFloorMethod::Normalize:           return "Normalize";
	default:                                    return "Unknown";
	}
}
