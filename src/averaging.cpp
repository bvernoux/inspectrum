/*
 *  Copyright (C) 2026, Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 *  Extended averaging modes for inspectrum ng.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include "averaging.h"
#include "util.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <vector>

/*
 * Linear (mean) averaging with causal box filter.
 * This is the same algorithm as the existing getEnhancedTile()
 * but extracted into a standalone function.
 */
static void avgLinear(const float *data, int n_freq, int n_time,
		      float *dest, int count)
{
	if (count <= 1) {
		memcpy(dest, data, (size_t)n_freq * n_time * sizeof(float));
		return;
	}

	/* convert entire tile to linear domain */
	size_t total = (size_t)n_freq * n_time;
	std::vector<float> linear(total);
	for (size_t i = 0; i < total; i++)
		linear[i] = dBtoLinear(data[i]);

	/* causal box filter: running sum per frequency bin */
	std::vector<double> runSum(n_freq, 0.0);

	for (int x = 0; x < n_time; x++) {
		int leaveX = x - count;
		int win = std::min(x + 1, count);
		double invWin = 1.0 / win;
		const float *linCol = linear.data() + (size_t)x * n_freq;
		float *outCol = dest + (size_t)x * n_freq;
		const float *leaveCol = (leaveX >= 0)
			? linear.data() + (size_t)leaveX * n_freq
			: nullptr;

		if (leaveCol) {
			for (int y = 0; y < n_freq; y++) {
				runSum[y] += linCol[y] - leaveCol[y];
				outCol[y] = linearTodB((float)(runSum[y] * invWin));
			}
		} else {
			for (int y = 0; y < n_freq; y++) {
				runSum[y] += linCol[y];
				outCol[y] = linearTodB((float)(runSum[y] * invWin));
			}
		}
	}
}

/*
 * Exponential averaging: S_avg = alpha * S_new + (1-alpha) * S_old.
 * Operates in linear domain.
 */
static void avgExponential(const float *data, int n_freq, int n_time,
			   float *dest, float alpha)
{
	if (n_time <= 0)
		return;

	/* initialize accumulator with first column */
	std::vector<float> acc(n_freq);
	const float *col0 = data;
	for (int y = 0; y < n_freq; y++)
		acc[y] = dBtoLinear(col0[y]);

	/* first column output */
	float *out0 = dest;
	for (int y = 0; y < n_freq; y++)
		out0[y] = linearTodB(acc[y]);

	float oneMinusAlpha = 1.0f - alpha;

	for (int x = 1; x < n_time; x++) {
		const float *inCol = data + (size_t)x * n_freq;
		float *outCol = dest + (size_t)x * n_freq;

		for (int y = 0; y < n_freq; y++) {
			float newVal = dBtoLinear(inCol[y]);
			acc[y] = alpha * newVal + oneMinusAlpha * acc[y];
			outCol[y] = linearTodB(acc[y]);
		}
	}
}

/*
 * Transpose column-major [n_time][n_freq] to row-major [n_freq][n_time]
 * with dB-to-linear conversion in the same pass.
 */
static void transposeToLinear(const float *colMajor, int n_freq, int n_time,
			      float *rowMajor)
{
	for (int x = 0; x < n_time; x++) {
		const float *col = colMajor + (size_t)x * n_freq;
		for (int y = 0; y < n_freq; y++)
			rowMajor[(size_t)y * n_time + x] = dBtoLinear(col[y]);
	}
}

/*
 * Transpose row-major [n_freq][n_time] back to column-major [n_time][n_freq].
 * Values are already in dB.
 */
static void transposeFromdB(const float *rowMajor, int n_freq, int n_time,
			    float *colMajor)
{
	for (int x = 0; x < n_time; x++) {
		float *col = colMajor + (size_t)x * n_freq;
		for (int y = 0; y < n_freq; y++)
			col[y] = rowMajor[(size_t)y * n_time + x];
	}
}

/*
 * Max hold: sliding window maximum using monotonic deque.
 * O(n_freq * n_time) amortized - each element is pushed/popped at most once.
 * Transposes to row-major for sequential access, then transposes back.
 */
static void avgMaxHold(const float *data, int n_freq, int n_time,
		       float *dest, int count)
{
	size_t total = (size_t)n_freq * n_time;
	std::vector<float> tIn(total);  /* row-major linear */
	std::vector<float> tOut(total); /* row-major dB result */
	transposeToLinear(data, n_freq, n_time, tIn.data());

	std::deque<int> dq;
	for (int y = 0; y < n_freq; y++) {
		float *row = tIn.data() + (size_t)y * n_time;
		float *out = tOut.data() + (size_t)y * n_time;

		dq.clear();
		for (int x = 0; x < n_time; x++) {
			while (!dq.empty() && row[dq.back()] <= row[x])
				dq.pop_back();
			dq.push_back(x);
			if (dq.front() < x - count + 1)
				dq.pop_front();
			out[x] = linearTodB(row[dq.front()]);
		}
	}

	transposeFromdB(tOut.data(), n_freq, n_time, dest);
}

/*
 * Min hold: sliding window minimum using monotonic deque.
 * O(n_freq * n_time) amortized, fully sequential memory access.
 */
static void avgMinHold(const float *data, int n_freq, int n_time,
		       float *dest, int count)
{
	size_t total = (size_t)n_freq * n_time;
	std::vector<float> tIn(total);
	std::vector<float> tOut(total);
	transposeToLinear(data, n_freq, n_time, tIn.data());

	std::deque<int> dq;
	for (int y = 0; y < n_freq; y++) {
		float *row = tIn.data() + (size_t)y * n_time;
		float *out = tOut.data() + (size_t)y * n_time;

		dq.clear();
		for (int x = 0; x < n_time; x++) {
			while (!dq.empty() && row[dq.back()] >= row[x])
				dq.pop_back();
			dq.push_back(x);
			if (dq.front() < x - count + 1)
				dq.pop_front();
			out[x] = linearTodB(row[dq.front()]);
		}
	}

	transposeFromdB(tOut.data(), n_freq, n_time, dest);
}

void applyAveraging(const float *data, int n_freq, int n_time,
		    AveragingMode mode, float *dest,
		    int count, float alpha)
{
	if (n_freq <= 0 || n_time <= 0)
		return;

	switch (mode) {
	case AveragingMode::Off:
		memcpy(dest, data, (size_t)n_freq * n_time * sizeof(float));
		break;

	case AveragingMode::Linear:
		avgLinear(data, n_freq, n_time, dest, count);
		break;

	case AveragingMode::Exponential:
		avgExponential(data, n_freq, n_time, dest, alpha);
		break;

	case AveragingMode::MaxHold:
		avgMaxHold(data, n_freq, n_time, dest, count);
		break;

	case AveragingMode::MinHold:
		avgMinHold(data, n_freq, n_time, dest, count);
		break;

	default:
		memcpy(dest, data, (size_t)n_freq * n_time * sizeof(float));
		break;
	}
}

const char *averagingModeName(AveragingMode mode)
{
	switch (mode) {
	case AveragingMode::Off:          return "Off";
	case AveragingMode::Linear:       return "Linear";
	case AveragingMode::Exponential:  return "Exponential";
	case AveragingMode::MaxHold:      return "Max hold";
	case AveragingMode::MinHold:      return "Min hold";
	default:                          return "Unknown";
	}
}
