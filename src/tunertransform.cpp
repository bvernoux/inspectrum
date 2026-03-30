/*
 *  Copyright (C) 2016, Mike Walters <mike@flomp.net>
 *  Copyright (C) 2026, Benjamin Vernoux <bvernoux@hydrasdr.com>
 *
 *  This file is part of inspectrum.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tunertransform.h"
#include <liquid/liquid.h>
#include "util.h"

TunerTransform::TunerTransform(std::shared_ptr<SampleSource<std::complex<float>>> src)
    : SampleBuffer(src), frequency(0), bandwidth(1.), taps{1.0f}
{
    nco = nco_crcf_create(LIQUID_NCO);
    rebuildFilter();
}

TunerTransform::~TunerTransform()
{
    if (nco)
        nco_crcf_destroy(nco);
    if (filter)
        firfilt_crcf_destroy(filter);
}

void TunerTransform::rebuildFilter()
{
    if (filter)
        firfilt_crcf_destroy(filter);
    filter = firfilt_crcf_create(taps.data(), taps.size());
    filterDirty = false;
}

void TunerTransform::work(void *input, void *output, int count, size_t sampleid)
{
    auto out = static_cast<std::complex<float>*>(output);

    /* grow mix buffer if needed (no realloc if already large enough) */
    if ((int)mixBuf.size() < count)
        mixBuf.resize(count);

    /* mix down using pre-allocated NCO */
    nco_crcf_set_phase(nco, fmodf(frequency * sampleid, Tau));
    nco_crcf_set_frequency(nco, frequency);
    nco_crcf_mix_block_down(nco,
                            static_cast<std::complex<float>*>(input),
                            mixBuf.data(),
                            count);

    /* rebuild filter if taps changed, otherwise just reset state */
    if (filterDirty)
        rebuildFilter();
    else
        firfilt_crcf_reset(filter);

    /* filter */
    for (int i = 0; i < count; i++) {
        firfilt_crcf_push(filter, mixBuf[i]);
        firfilt_crcf_execute(filter, &out[i]);
    }

    /* apply gain */
    if (gain != 1.0f) {
        for (int i = 0; i < count; i++)
            out[i] *= gain;
    }
}

void TunerTransform::setFrequency(float frequency)
{
    this->frequency = frequency;
}

void TunerTransform::setTaps(std::vector<float> taps)
{
    this->taps = taps;
    filterDirty = true;
}

void TunerTransform::setGain(float g)
{
    this->gain = g;
}

float TunerTransform::relativeBandwidth() {
    return bandwidth;
}

void TunerTransform::setRelativeBandwith(float bandwidth)
{
    this->bandwidth = bandwidth;
}

