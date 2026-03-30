/*
 *  Colormap LUT library for inspectrum ng.
 *
 *  Viridis, Inferno, Magma: adapted from matplotlib (BSD license).
 *  Turbo: adapted from Google AI (Apache 2.0 license).
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include "colormaps.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline uint32_t packRGBA(int r, int g, int b)
{
	return 0xFF000000u
		| (((uint32_t)r & 0xFF) << 16)
		| (((uint32_t)g & 0xFF) << 8)
		| ((uint32_t)b & 0xFF);
}

static inline uint8_t clamp8(float v)
{
	if (v < 0.0f) return 0;
	if (v > 255.0f) return 255;
	return (uint8_t)(v + 0.5f);
}

/*
 * Interpolate between control points.
 * cp is an array of {position, R, G, B} where position is in [0..1]
 * and R,G,B are in [0..255].
 */
struct ColorCP {
	float pos;
	float r, g, b;
};

static void interpolateColormap(const ColorCP *cp, int ncp, uint32_t *dest)
{
	for (int i = 0; i < 256; i++) {
		float t = i / 255.0f;

		/* find bracketing control points */
		int lo = 0;
		for (int j = 1; j < ncp; j++) {
			if (cp[j].pos <= t)
				lo = j;
		}
		int hi = (lo + 1 < ncp) ? lo + 1 : lo;

		float span = cp[hi].pos - cp[lo].pos;
		float frac = (span > 0.0f) ? (t - cp[lo].pos) / span : 0.0f;

		float r = cp[lo].r + frac * (cp[hi].r - cp[lo].r);
		float g = cp[lo].g + frac * (cp[hi].g - cp[lo].g);
		float b = cp[lo].b + frac * (cp[hi].b - cp[lo].b);

		dest[i] = packRGBA(clamp8(r), clamp8(g), clamp8(b));
	}
}

/* ---- Default: existing inspectrum HSV sweep ---- */

static void generateDefault(uint32_t *dest)
{
	for (int i = 0; i < 256; i++) {
		float p = (float)i / 256.0f;
		/* HSV: hue = p*0.83 (blue->red), sat=1, val=1-p */
		float h = p * 0.83f;
		float s = 1.0f;
		float v = 1.0f - p;

		/* HSV to RGB */
		float c = v * s;
		float x = c * (1.0f - fabsf(fmodf(h * 6.0f, 2.0f) - 1.0f));
		float m = v - c;
		float r, g, b;
		int sect = (int)(h * 6.0f) % 6;
		switch (sect) {
		case 0: r = c; g = x; b = 0; break;
		case 1: r = x; g = c; b = 0; break;
		case 2: r = 0; g = c; b = x; break;
		case 3: r = 0; g = x; b = c; break;
		case 4: r = x; g = 0; b = c; break;
		default: r = c; g = 0; b = x; break;
		}
		dest[i] = packRGBA(clamp8((r + m) * 255.0f),
				   clamp8((g + m) * 255.0f),
				   clamp8((b + m) * 255.0f));
	}
}

/* ---- Viridis (matplotlib) ---- */
/* 16 control points sampled from the 256-entry Viridis LUT */

static const ColorCP viridis_cp[] = {
	{0.000f,  68,   1,  84},
	{0.067f,  72,  33, 115},
	{0.133f,  67,  62, 133},
	{0.200f,  56,  88, 140},
	{0.267f,  45, 112, 142},
	{0.333f,  37, 133, 142},
	{0.400f,  30, 155, 138},
	{0.467f,  34, 175, 127},
	{0.533f,  53, 194, 111},
	{0.600f,  94, 211,  82},
	{0.667f, 143, 223,  53},
	{0.733f, 190, 229,  28},
	{0.800f, 229, 228,  32},
	{0.867f, 253, 219,  36},
	{0.933f, 253, 195,  40},
	{1.000f, 253, 231,  37},
};

/* ---- Inferno (matplotlib) ---- */

static const ColorCP inferno_cp[] = {
	{0.000f,   0,   0,   4},
	{0.067f,  12,   7,  40},
	{0.133f,  40,  11,  84},
	{0.200f,  73,  10, 104},
	{0.267f, 106,  14, 104},
	{0.333f, 137,  25,  91},
	{0.400f, 166,  44,  70},
	{0.467f, 192,  68,  46},
	{0.533f, 213,  96,  24},
	{0.600f, 229, 130,   7},
	{0.667f, 239, 167,   8},
	{0.733f, 242, 205,  32},
	{0.800f, 236, 236,  80},
	{0.867f, 244, 250, 120},
	{0.933f, 252, 255, 164},
	{1.000f, 252, 255, 164},
};

/* ---- Magma (matplotlib) ---- */

static const ColorCP magma_cp[] = {
	{0.000f,   0,   0,   4},
	{0.067f,  10,   7,  36},
	{0.133f,  33,  12,  79},
	{0.200f,  63,  15, 114},
	{0.267f,  94,  17, 126},
	{0.333f, 127,  22, 122},
	{0.400f, 160,  34, 110},
	{0.467f, 192,  54,  91},
	{0.533f, 219,  81,  72},
	{0.600f, 237, 115,  63},
	{0.667f, 249, 153,  73},
	{0.733f, 254, 192, 100},
	{0.800f, 254, 226, 145},
	{0.867f, 252, 247, 194},
	{0.933f, 252, 253, 228},
	{1.000f, 252, 253, 255},
};

/* ---- Turbo (Google) ---- */
/*
 * Polynomial approximation from:
 * "Turbo, An Improved Rainbow Colormap for Visualization"
 * https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html
 */
static void generateTurbo(uint32_t *dest)
{
	for (int i = 0; i < 256; i++) {
		float x = i / 255.0f;

		float r = 0.13572138f
			+ x * (4.61539260f
			+ x * (-42.66032258f
			+ x * (132.13108234f
			+ x * (-152.94239396f + x * 59.28637943f))));

		float g = 0.09140261f
			+ x * (2.19418839f
			+ x * (4.84296658f
			+ x * (-14.18503333f + x * 7.56348072f)));

		float b = 0.10667330f
			+ x * (12.64194608f
			+ x * (-60.58204836f
			+ x * (110.36276771f
			+ x * (-89.90310912f + x * 27.34824973f))));

		dest[i] = packRGBA(clamp8(r * 255.0f),
				   clamp8(g * 255.0f),
				   clamp8(b * 255.0f));
	}
}

/* ---- Grayscale ---- */

static void generateGrayscale(uint32_t *dest)
{
	for (int i = 0; i < 256; i++) {
		/* index 0 = darkest (highest power), 255 = brightest (lowest power)
		 * The spectrogram maps: 0 = lowest power, 255 = highest power.
		 * For grayscale: bright = strong signal, dark = weak/noise. */
		int v = 255 - i;
		dest[i] = packRGBA(v, v, v);
	}
}

void generateColormap(ColormapType type, uint32_t *dest)
{
	switch (type) {
	case ColormapType::Default:
		generateDefault(dest);
		break;

	case ColormapType::Viridis:
		interpolateColormap(viridis_cp, 16, dest);
		break;

	case ColormapType::Inferno:
		interpolateColormap(inferno_cp, 16, dest);
		break;

	case ColormapType::Turbo:
		generateTurbo(dest);
		break;

	case ColormapType::Magma:
		interpolateColormap(magma_cp, 16, dest);
		break;

	case ColormapType::Grayscale:
		generateGrayscale(dest);
		break;

	default:
		generateDefault(dest);
		break;
	}
}

const char *colormapTypeName(ColormapType type)
{
	switch (type) {
	case ColormapType::Default:   return "Default";
	case ColormapType::Viridis:   return "Viridis";
	case ColormapType::Inferno:   return "Inferno";
	case ColormapType::Turbo:     return "Turbo";
	case ColormapType::Magma:     return "Magma";
	case ColormapType::Grayscale: return "Grayscale";
	default:                      return "Unknown";
	}
}
