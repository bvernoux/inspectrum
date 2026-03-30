/*
 *  Copyright (C) 2026, Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 *  Tile cache key and hash for spectrogram tile caching.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <QtGlobal>

class TileCacheKey
{
public:
	TileCacheKey(int fftSize, int zoomLevel, size_t sample, int overlap = 0)
		: fftSize(fftSize), zoomLevel(zoomLevel),
		  sample(sample), overlap(overlap) {}

	bool operator==(const TileCacheKey &k2) const {
		return fftSize == k2.fftSize &&
		       zoomLevel == k2.zoomLevel &&
		       sample == k2.sample &&
		       overlap == k2.overlap;
	}

	int fftSize;
	int zoomLevel;
	size_t sample;
	int overlap;
};

/*
 * FNV-1a 32-bit hash for QCache key lookup.
 * Constants: offset_basis = 2166136261, prime = 16777619
 * Ref: http://www.isthe.com/chongo/tech/comp/fnv/
 */
static constexpr uint FNV1A_BASIS = 2166136261u;
static constexpr uint FNV1A_PRIME = 16777619u;

inline uint qHash(const TileCacheKey &key, uint seed = 0)
{
	uint h = FNV1A_BASIS;
	h = (h ^ (uint)key.fftSize) * FNV1A_PRIME;
	h = (h ^ (uint)key.zoomLevel) * FNV1A_PRIME;
	h = (h ^ (uint)(key.sample & 0xFFFFFFFFu)) * FNV1A_PRIME;
	h = (h ^ (uint)((key.sample >> 16) >> 16)) * FNV1A_PRIME;
	h = (h ^ (uint)key.overlap) * FNV1A_PRIME;
	return h ^ seed;
}
