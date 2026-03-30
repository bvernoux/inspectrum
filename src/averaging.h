/*
 *  Copyright (C) 2026, Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 *  Extended averaging modes for inspectrum ng.
 *  Linear (mean), exponential, max hold, min hold.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#pragma once

enum class AveragingMode {
	Off = 0,
	Linear,
	Exponential,
	MaxHold,
	MinHold,
	AveragingModeCount
};

/*
 * Apply averaging across time columns of spectrogram tile data.
 *
 * Data layout is column-major: data[col * n_freq + bin], in dB.
 * The function converts to linear domain for processing and back to dB.
 *
 * @param data    input tile data in dB (column-major)
 * @param n_freq  number of frequency bins per column
 * @param n_time  number of time columns in tile
 * @param mode    averaging mode
 * @param dest    output (same dimensions, column-major, dB)
 * @param count   window size for Linear mode (1 = off)
 * @param alpha   decay factor for Exponential mode (0.01..0.99)
 */
void applyAveraging(const float *data, int n_freq, int n_time,
		    AveragingMode mode, float *dest,
		    int count = 1, float alpha = 0.1f);

/* Display name for UI combo box */
const char *averagingModeName(AveragingMode mode);

static inline int averagingModeCount()
{
	return static_cast<int>(AveragingMode::AveragingModeCount);
}
