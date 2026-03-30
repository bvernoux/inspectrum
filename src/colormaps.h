/*
 *  Colormap LUT library for inspectrum ng.
 *  256-entry RGBA lookup tables for spectrogram rendering.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#pragma once

#include <cstdint>

enum class ColormapType {
	Default = 0,
	Viridis,
	Inferno,
	Turbo,
	Magma,
	Grayscale,
	ColormapTypeCount
};

/*
 * Generate a 256-entry RGBA colormap.
 *
 * @param type  colormap type
 * @param dest  output array (must hold 256 uint32_t values)
 *
 * Index 0 = lowest power (darkest), 255 = highest power (brightest).
 * Format: 0xAARRGGBB (QRgb / ARGB32).
 */
void generateColormap(ColormapType type, uint32_t *dest);

/* Display name for UI combo box */
const char *colormapTypeName(ColormapType type);

static inline int colormapTypeCount()
{
	return static_cast<int>(ColormapType::ColormapTypeCount);
}
