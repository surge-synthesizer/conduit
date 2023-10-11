/*
 * Conduit - a project highlighting CLAP-first development
 *           and exercising the surge synth team libraries.
 *
 * Copyright 2023 Paul Walker and authors in github
 *
 * This file you are viewing now is released under the
 * MIT license as described in LICENSE.md
 *
 * The assembled program which results from compiling this
 * project has GPL3 dependencies, so if you distribute
 * a binary, the combined work would be a GPL3 product.
 *
 * Roughly, that means you are welcome to copy the code and
 * ideas in the src/ directory, but perhaps not code from elsewhere
 * if you are closed source or non-GPL3. And if you do copy this code
 * you will need to replace some of the dependencies. Please see
 * the discussion in README.md for further information on what this may
 * mean for you.
 */

#include "voice.h"
#include "polysynth.h"
#include <cmath>
#include <algorithm>

#include "libMTSClient.h"

#include "sst/basic-blocks/dsp/CorrelatedNoise.h"
#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/FastMath.h"
#include "sst/basic-blocks/dsp/PanLaws.h"

#include "sst/filters/FilterConfiguration.h"

namespace sst::conduit::polysynth
{
float pival =
    3.14159265358979323846; // I always forget what you need for M_PI to work on all platforms

void PolysynthVoice::recalcPitch()
{
    if (mtsClient && MTS_HasMaster(mtsClient))
    {
        baseFreq = MTS_NoteToFrequency(mtsClient, key, channel);
    }
    else
    {
        baseFreq = baseFrequencyByMidiKey[std::clamp(key, 0, 127)];
    }

    if (sawActive)
    {
        for (int i = 0; i < sawUnison; ++i)
        {
            auto uf =
                baseFreq *
                synth.twoToXTable.twoToThe(
                    ((sawUnisonDetune.value() * sawUniVoiceDetune[i] + sawFine.value()) / 100 +
                     sawCoarse.value() + pitchNoteExpressionValue + pitchBendWheel) /
                    12.0);
            sawOsc[i].setFrequency(uf, srInv);
        }
    }

    static constexpr float mul[7] = {0.125, 0.25, 0.5, 1, 2, 4, 8};

    if (pulseActive)
    {
        auto po = std::clamp((int)std::round(pulseOctave.value()) + 3, 0, 6);
        auto sbf = baseFreq * mul[po];
        auto pf = sbf * synth.twoToXTable.twoToThe((pulseCoarse.value() + pulseFine.value() * 0.01 +
                                                    pitchNoteExpressionValue + pitchBendWheel) /
                                                   12.0);
        pulseOsc.setFrequency(pf, srInv);
        pulseOsc.setPulseWidth(pulseWidth.value());
    }

    if (sinActive)
    {
        auto po = std::clamp((int)std::round(sinOctave.value()) + 3, 0, 6);
        auto sbf = baseFreq * mul[po];
        auto pf = sbf * synth.twoToXTable.twoToThe(
                            (sinCoarse.value() + pitchNoteExpressionValue + pitchBendWheel) / 12.0);
        sinOsc.setRate(2.0 * M_PI * pf * srInv);
    }
}

void PolysynthVoice::recalcFilter()
{
    if (svfActive)
    {
        auto co = svfCutoff.value();
        auto rm = svfResonance.value();
        svfImpl.setCoeff(co, rm, srInv);
    }

    if (lpfActive)
    {
        sst::filters::FilterCoefficientMaker coefMaker;
        coefMaker.setSampleRateAndBlockSize(samplerate, blockSize);
        coefMaker.MakeCoeffs(lpfCutoff.value() - 60, lpfResonance.value(), qfType, qfSubType,
                             nullptr, false);
        coefMaker.updateState(qfState);
    }
}

static __m128 wsNoOp(sst::waveshapers::QuadWaveshaperState *__restrict, __m128 in, __m128 drive)
{
    return in;
}

static __m128 qfNoOp(sst::filters::QuadFilterUnitState *__restrict, __m128 in) { return in; }

void PolysynthVoice::processBlock()
{
    static constexpr float vScale{0.2};
    aeg.processBlock(aegValues.attack.value(), aegValues.decay.value(), aegValues.sustain.value(),
                     aegValues.release.value(), 0, 0, 0, gated);
    feg.processBlock(fegValues.attack.value(), fegValues.decay.value(), fegValues.sustain.value(),
                     fegValues.release.value(), 0, 0, 0, gated);
    lfos[0].process_block(lfoData[0].rate.value(), lfoData[0].deform.value(), lfoData[0].shape);
    lfos[1].process_block(lfoData[1].rate.value(), lfoData[1].deform.value(), lfoData[1].shape);

    *svfCutoff.internalMod = 0;
    *lpfCutoff.internalMod = 0;

    for (auto &r : routings)
    {
        if (r.target)
            *(r.target) = 0;
    }
    for (auto &r : routings)
    {
        if (r.source && r.target)
        {
            auto s = *(r.source);
            s *= (r.via ? *(r.via) : 1);
            *(r.target) += s * (*r.depth) * r.range;
        }
    }

    *svfCutoff.internalMod +=
        feg.outBlock0 * fegToSvfCutoff.value() + svfKeytrack.value() * (key - 69);
    *lpfCutoff.internalMod +=
        feg.outBlock0 * fegToLPFCutoff.value() + lpfKeytrack.value() * (key - 69);

    recalcFilter();
    recalcPitch();

    memset(outputOS, 0, sizeof(outputOS));

    if (sawActive)
    {
        sawLevel_lipol.newValue(sawLevel.value());
        for (auto s = 0U; s < blockSizeOS; ++s)
        {
            float L{0}, R{0};
            auto sl = sawLevel_lipol.v;
            sl = sl * sl * sl;
            for (int i = 0; i < sawUnison; ++i)
            {
                auto out = sawOsc[i].step();
                L += vScale * sl * sawUniLevelNorm[i] * sawUniPanL[i] * out;
                R += vScale * sl * sawUniLevelNorm[i] * sawUniPanR[i] * out;
            }

            outputOS[0][s] += L;
            outputOS[1][s] += R;
            sawLevel_lipol.process();
        }
    }

    if (pulseActive)
    {
        pulseLevel_lipol.newValue(pulseLevel.value());
        for (auto s = 0U; s < blockSizeOS; ++s)
        {
            auto sl = pulseLevel_lipol.v;
            sl = sl * sl * sl;
            auto V = vScale * sl * pulseOsc.step();

            outputOS[0][s] += V;
            outputOS[1][s] += V;
            pulseLevel_lipol.process();
        }
    }

    if (sinActive)
    {
        sinLevel_lipol.newValue(sinLevel.value());
        for (auto s = 0U; s < blockSizeOS; ++s)
        {
            sinOsc.step();
            auto sl = sinLevel_lipol.v;
            sl = sl * sl * sl;
            auto V = vScale * sl * sinOsc.u;

            outputOS[0][s] += V;
            outputOS[1][s] += V;
            sinLevel_lipol.process();
        }
    }

    if (noiseActive)
    {
        noiseLevel_lipol.newValue(noiseLevel.value());
        for (auto s = 0U; s < blockSizeOS; ++s)
        {
            auto sl = noiseLevel_lipol.v;
            sl = sl * sl * sl;

            auto V = vScale * sl *
                     sst::basic_blocks::dsp::correlated_noise_o2mk2_supplied_value(
                         w0, w1, noiseColor.value(), urd(gen));
            outputOS[0][s] += V;
            outputOS[1][s] += V;

            noiseLevel_lipol.process();
        }
    }

    // Filter stage
    aegPFG_lipol.set_target(synth.dbToLinear(aegPFG.value()));
    aegPFG_lipol.multiply_2_blocks(outputOS[0], outputOS[1]);

    wsDrive_lipol.newValue(synth.dbToLinear(wsDrive.value()));
    wsBias_lipol.newValue(wsBias.value() * (wsActive ? 1.f : 0.f));
    filterFeedback_lipol.newValue(filterFeedback.value());
    if (anyFilterStepActive)
    {

#define PACK                                                                                       \
    auto output = _mm_set_ps(0, 0, outputOS[1][s], outputOS[0][s]);                                \
    auto drive = _mm_set1_ps(wsDrive_lipol.v);                                                     \
    wsDrive_lipol.process();                                                                       \
    auto bias = _mm_set1_ps(wsBias_lipol.v);                                                       \
    wsBias_lipol.process();                                                                        \
    auto fback = _mm_set1_ps(filterFeedback_lipol.v);                                              \
    filterFeedback_lipol.process();                                                                \
    output = _mm_add_ps(output, filterFeedbackSignal)

#define UNPACK                                                                                     \
    filterFeedbackSignal = _mm_mul_ps(output, fback);                                              \
    float outArr alignas(16)[4];                                                                   \
    _mm_store_ps(outArr, output);                                                                  \
    outputOS[0][s] = outArr[0];                                                                    \
    outputOS[1][s] = outArr[1]

        switch (filterRouting)
        {
        case LowWSMulti:
            for (auto s = 0U; s < blockSizeOS; ++s)
            {
                PACK;
                output = qfPtr(&qfState, output);
                output = wsPtr(&wsState, _mm_add_ps(output, bias), drive);
                output = svfFilterOp(svfImpl, output);
                UNPACK;
            }
            break;
        case MultiWSLow:
            for (auto s = 0U; s < blockSizeOS; ++s)
            {
                PACK;
                output = svfFilterOp(svfImpl, output);
                output = wsPtr(&wsState, _mm_add_ps(output, bias), drive);
                output = qfPtr(&qfState, output);
                UNPACK;
            }
            break;
        case WSLowMulti:
            for (auto s = 0U; s < blockSizeOS; ++s)
            {
                PACK;
                output = wsPtr(&wsState, _mm_add_ps(output, bias), drive);
                output = qfPtr(&qfState, output);
                output = svfFilterOp(svfImpl, output);
                UNPACK;
            }
            break;
        case LowMultiWS:
            for (auto s = 0U; s < blockSizeOS; ++s)
            {
                PACK;
                output = qfPtr(&qfState, output);
                output = svfFilterOp(svfImpl, output);
                output = wsPtr(&wsState, _mm_add_ps(output, bias), drive);
                UNPACK;
            }
            break;
        case WSPar:
            for (auto s = 0U; s < blockSizeOS; ++s)
            {
                PACK;
                output = wsPtr(&wsState, _mm_add_ps(output, bias), drive);

                auto outputQ = qfPtr(&qfState, output);
                auto outputS = svfFilterOp(svfImpl, output);
                const auto half = _mm_set1_ps(0.5f);
                output = _mm_mul_ps(half, _mm_add_ps(outputQ, outputS));
                UNPACK;
            }
            break;
        case ParWS:
            for (auto s = 0U; s < blockSizeOS; ++s)
            {
                PACK;
                auto outputQ = qfPtr(&qfState, output);
                auto outputS = svfFilterOp(svfImpl, output);
                const auto half = _mm_set1_ps(0.5f);
                output = _mm_mul_ps(half, _mm_add_ps(outputQ, outputS));
                output = wsPtr(&wsState, _mm_add_ps(output, bias), drive);

                UNPACK;
            }
            break;
        }
    }
    sst::basic_blocks::mechanics::scale_by<blockSizeOS>(aeg.outputCache, outputOS[0]);
    sst::basic_blocks::mechanics::scale_by<blockSizeOS>(aeg.outputCache, outputOS[1]);

    auto olv = outputLevel.value();
    auto velSen = velocitySens.value();
    auto velAtten = velocity * velSen + (1 - velSen);
    olv *= velAtten;

    olv = olv * olv * olv;

    outputLevel_lipol.set_target(olv);
    outputLevel_lipol.multiply_2_blocks(outputOS[0], outputOS[1]);

    auto opv = outputPan.value();
    if (opv != 0.f)
    {
        sst::basic_blocks::dsp::pan_laws::panmatrix_t panMatrix;
        sst::basic_blocks::dsp::pan_laws::stereoTruePanning((opv + 1) * 0.5, panMatrix);
        for (auto s = 0U; s < blockSizeOS; ++s)
        {
            auto l = panMatrix[0] * outputOS[0][s] + panMatrix[2] * outputOS[1][s];
            auto r = panMatrix[1] * outputOS[1][s] + panMatrix[3] * outputOS[0][s];

            outputOS[0][s] = l;
            outputOS[1][s] = r;
        }
    }
}

void PolysynthVoice::start(int16_t porti, int16_t channeli, int16_t keyi, int32_t noteidi,
                           double veli)
{
    portid = porti;
    channel = channeli;
    key = keyi;
    note_id = noteidi;
    velocity = veli;

    pitchBendWheel = 0;
    filterFeedbackSignal = _mm_setzero_ps();

    sawUnison = static_cast<int>(*synth.paramToValue.at(ConduitPolysynth::pmSawUnisonCount));

    sawActive = static_cast<bool>(*synth.paramToValue.at(ConduitPolysynth::pmSawActive));
    pulseActive = static_cast<bool>(*synth.paramToValue.at(ConduitPolysynth::pmPWActive));
    sinActive = static_cast<bool>(*synth.paramToValue.at(ConduitPolysynth::pmSinActive));
    noiseActive = static_cast<bool>(*synth.paramToValue.at(ConduitPolysynth::pmNoiseActive));

    svfActive = static_cast<bool>(*synth.paramToValue.at(ConduitPolysynth::pmSVFActive));
    if (svfActive)
    {
        svfMode = static_cast<int>(*synth.paramToValue.at(ConduitPolysynth::pmSVFFilterMode));
        switch (svfMode)
        {
        case StereoSimperSVF::LP:
            svfFilterOp = StereoSimperSVF::stepSSE<StereoSimperSVF::LP>;
            break;
        case StereoSimperSVF::HP:
            svfFilterOp = StereoSimperSVF::stepSSE<StereoSimperSVF::HP>;
            break;
        case StereoSimperSVF::BP:
            svfFilterOp = StereoSimperSVF::stepSSE<StereoSimperSVF::BP>;
            break;
        case StereoSimperSVF::NOTCH:
            svfFilterOp = StereoSimperSVF::stepSSE<StereoSimperSVF::NOTCH>;
            break;
        case StereoSimperSVF::PEAK:
            svfFilterOp = StereoSimperSVF::stepSSE<StereoSimperSVF::PEAK>;
            break;
        case StereoSimperSVF::ALL:
            svfFilterOp = StereoSimperSVF::stepSSE<StereoSimperSVF::ALL>;
            break;
        }
    }
    else
    {
        svfFilterOp = [](auto &a, auto b) { return b; };
    }

    gated = true;
    active = true;
    srInv = 1.0 / samplerate;

    svfImpl.init();

    aeg.attackFrom(0.f, aegValues.attack.value(), 0, false);
    feg.attackFrom(0.f, fegValues.attack.value(), 0, false);
    if (sawUnison == 1)
    {
        sawUniVoiceDetune[0] = 0;
        sawUniPanL[0] = 1;
        sawUniPanR[0] = 1;
        sawUniLevelNorm[0] = 1.0;
    }
    else
    {
        for (int i = 0; i < sawUnison; ++i)
        {
            float dI = 1.0 * i / (sawUnison - 1);
            sawUniVoiceDetune[i] = 2 * dI - 1;
            sawUniPanL[i] = std::cos(0.5 * pival * dI);
            sawUniPanR[i] = std::sin(0.5 * pival * dI);

            sawUniLevelNorm[i] = 1.0 / sqrt(sawUnison);
        }
    }

    for (auto &o : sawOsc)
        o.retrigger();

    recalcPitch();
    recalcFilter();

    wsActive = static_cast<bool>(*synth.paramToValue.at(ConduitPolysynth::pmWSActive));

    if (wsActive)
    {
        float R[sst::waveshapers::n_waveshaper_registers];
        auto wsTypeEnum =
            static_cast<Waveshapers>(*synth.paramToValue.at(ConduitPolysynth::pmWSMode));

        auto type = sst::waveshapers::WaveshaperType::wst_ojd;
        switch (wsTypeEnum)
        {
        case Soft:
            type = sst::waveshapers::WaveshaperType::wst_soft;
            break;
        case OJD:
            type = sst::waveshapers::WaveshaperType::wst_ojd;
            break;
        case Digital:
            type = sst::waveshapers::WaveshaperType::wst_digital;
            break;
        case FullWaveRect:
            type = sst::waveshapers::WaveshaperType::wst_fwrectify;
            break;
        case WestcoastFold:
            type = sst::waveshapers::WaveshaperType::wst_westfold;
            break;
        case Fuzz:
            type = sst::waveshapers::WaveshaperType::wst_fuzz;
            break;
        }
        sst::waveshapers::initializeWaveshaperRegister(type, R);

        for (int i = 0; i < sst::waveshapers::n_waveshaper_registers; ++i)
        {
            wsState.R[i] = _mm_set1_ps(R[i]);
        }
        wsState.init = _mm_cmpneq_ps(_mm_setzero_ps(), _mm_setzero_ps());
        wsPtr = sst::waveshapers::GetQuadWaveshaper(type);
    }
    else
    {
        wsPtr = wsNoOp;
    }

    lpfActive = static_cast<bool>(*synth.paramToValue.at(ConduitPolysynth::pmLPFActive));

    if (lpfActive)
    {
        qfState = sst::filters::QuadFilterUnitState{};
        for (int i = 0; i < 4; ++i)
        {
            memset(delayBufferData[i], 0, sizeof(delayBufferData[i]));
            qfState.DB[i] = delayBufferData[i];
            qfState.active[i] = (int)0xffffffff;
            qfState.WP[i] = 0;
        }

        auto lpfTypeEnum =
            static_cast<LPFTypes>(*synth.paramToValue.at(ConduitPolysynth::pmLPFFilterMode));

        switch (lpfTypeEnum)
        {
        case OBXD:
            qfType = sst::filters::FilterType::fut_obxd_4pole;
            qfSubType = (sst::filters::FilterSubType)3; // 24dv
            break;
        case Vintage:
            qfType = sst::filters::FilterType::fut_vintageladder;
            qfSubType = (sst::filters::FilterSubType)0;
            break;
        case K35:
            qfType = sst::filters::FilterType::fut_k35_lp;
            qfSubType = (sst::filters::FilterSubType)2; // medium saturation
            break;
        case Comb:
            qfType = sst::filters::FilterType::fut_comb_pos;
            qfSubType = (sst::filters::FilterSubType)1;
            break;
        case CutWarp:
            qfType = sst::filters::FilterType::fut_cutoffwarp_lp;
            qfSubType = sst::filters::FilterSubType::st_cutoffwarp_ojd3;
            break;
        case ResWarp:
            qfType = sst::filters::FilterType::fut_resonancewarp_lp;
            qfSubType = sst::filters::FilterSubType::st_resonancewarp_tanh4;
            break;
        }

        qfPtr = sst::filters::GetCompensatedQFPtrFilterUnit<true>(qfType, qfSubType);
    }
    else
    {
        qfPtr = qfNoOp;
    }

    filterRouting =
        static_cast<FilterRouting>(*synth.paramToValue.at(ConduitPolysynth::pmFilterRouting));

    anyFilterStepActive = wsActive || svfActive || lpfActive;

    auto l1shp = static_cast<int>(*synth.paramToValue.at(ConduitPolysynth::pmLFOShape));
    if (l1shp > 1) l1shp ++;
    lfoData[0].shape = (lfo_t::Shape)l1shp;
    lfos[0].attack(lfoData[0].shape);

    auto l2shp = static_cast<int>(*synth.paramToValue.at(ConduitPolysynth::pmLFOShape + ConduitPolysynth::offPmLFO2));
    if (l2shp > 1) l2shp ++;
    lfoData[1].shape = (lfo_t::Shape)l2shp;
    lfos[1].attack(lfoData[1].shape);

    // This obviously can be built at start time more intelligently
    int idx{0};
    for (auto &r : synth.patch.extension.modMatrixConfig->routings)
    {
        if (idx >= routings.size())
        {
            assert(false);
            continue;
        }
        routings[idx] = {};
        auto &rt = routings[idx];

        if (r.source != ModMatrixConfig::NONE && r.target != ConduitPolysynth::pmNoModTarget)
        {
            auto pmd = synth.paramDescriptionMap.at(r.target);
            rt.range = pmd.maxVal - pmd.minVal;
            auto tp = internalMods.find(r.target);

            switch (r.source)
            {
            case ModMatrixConfig::LFO1:
                rt.source = &(lfos[0].lastTarget);
                break;

            case ModMatrixConfig::LFO2:
                rt.source = &(lfos[1].lastTarget);
                break;

            case ModMatrixConfig::AEG:
                rt.source = &(aeg.outBlock0);
                break;

            case ModMatrixConfig::FEG:
                rt.source = &(feg.outBlock0);
                break;

            case ModMatrixConfig::PolyAT:
                rt.source = &polyphonicAT;
                break;

            case ModMatrixConfig::ChannelAT:
                rt.source = &channelPressure;
                break;

            case ModMatrixConfig::Midi_CC15:
                rt.source = &midi1CC[21];
                break;

            case ModMatrixConfig::ModWheel:
                rt.source = &midi1CC[1];
                break;

            case ModMatrixConfig::PitchBend:
                rt.source = &pitchBendWheel;
                break;

            default:
                break;
            }

            rt.target = &(tp->second);
            rt.depth = &(r.depth);
        }
        idx ++;
    }
}

void PolysynthVoice::release() { gated = false; }

void PolysynthVoice::StereoSimperSVF::setCoeff(float key, float res, float srInv)
{
    auto co = 440.0 * pow(2.0, (key - 69.0) / 12);
    co = std::clamp(co, 10.0, 25000.0); // just to be safe/lazy
    res = std::clamp(res, 0.01f, 0.99f);
    g = _mm_set1_ps(sst::basic_blocks::dsp::fasttan(pival * co * srInv));
    k = _mm_set1_ps(2.0 - 2.0 * res);
    gk = _mm_add_ps(g, k);
    a1 = _mm_div_ps(oneSSE, _mm_add_ps(oneSSE, _mm_mul_ps(g, gk)));
    a2 = _mm_mul_ps(g, a1);
    a3 = _mm_mul_ps(g, a2);
    ak = _mm_mul_ps(gk, a1);
}

template <int FilterMode>
void PolysynthVoice::StereoSimperSVF::step(StereoSimperSVF &that, float &L, float &R)
{
    auto vin = _mm_set_ps(0, 0, R, L);
    auto res = stepSSE<FilterMode>(that, vin);
    float r4 alignas(16)[4];
    _mm_store_ps(r4, res);
    L = r4[0];
    R = r4[1];
}

template <int FilterMode>
__m128 PolysynthVoice::StereoSimperSVF::stepSSE(StereoSimperSVF &that, __m128 vin)
{
    // auto v3 = vin[c] - ic2eq[c];
    auto v3 = _mm_sub_ps(vin, that.ic2eq);
    // auto v0 = a1 * v3 - ak * ic1eq[c];
    auto v0 = _mm_sub_ps(_mm_mul_ps(that.a1, v3), _mm_mul_ps(that.ak, that.ic1eq));
    // auto v1 = a2 * v3 + a1 * ic1eq[c];
    auto v1 = _mm_add_ps(_mm_mul_ps(that.a2, v3), _mm_mul_ps(that.a1, that.ic1eq));

    // auto v2 = a3 * v3 + a2 * ic1eq[c] + ic2eq[c];
    auto v2 = _mm_add_ps(_mm_add_ps(_mm_mul_ps(that.a3, v3), _mm_mul_ps(that.a2, that.ic1eq)),
                         that.ic2eq);

    // ic1eq[c] = 2 * v1 - ic1eq[c];
    that.ic1eq = _mm_sub_ps(_mm_mul_ps(that.twoSSE, v1), that.ic1eq);
    // ic2eq[c] = 2 * v2 - ic2eq[c];
    that.ic2eq = _mm_sub_ps(_mm_mul_ps(that.twoSSE, v2), that.ic2eq);

    __m128 res;

    switch (FilterMode)
    {
    case LP:
        res = v2;
        break;
    case BP:
        res = v1;
        break;
    case HP:
        res = v0;
        break;
    case NOTCH:
        res = _mm_add_ps(v2, v0);
        break;
    case PEAK:
        res = _mm_sub_ps(v2, v0);
        break;
    case ALL:
        res = _mm_sub_ps(_mm_add_ps(v2, v0), _mm_mul_ps(that.k, v1));
        break;
    default:
        res = _mm_setzero_ps();
    }

    return res;
}

void PolysynthVoice::StereoSimperSVF::init()
{
    ic1eq = _mm_setzero_ps();
    ic2eq = _mm_setzero_ps();
}

void PolysynthVoice::attachTo(sst::conduit::polysynth::ConduitPolysynth &p)
{
    auto attach = [this, &p](clap_id parm, ModulatedValue &toThat) {
        p.attachParam(parm, toThat.base);
        externalMods[parm] = 0;
        internalMods[parm] = 0;
        toThat.internalMod = &(internalMods[parm]);
        toThat.externalMod = &(externalMods[parm]);
    };
    attach(ConduitPolysynth::pmSawUnisonSpread, sawUnisonDetune);
    attach(ConduitPolysynth::pmSawCoarse, sawCoarse);
    attach(ConduitPolysynth::pmSawFine, sawFine);
    attach(ConduitPolysynth::pmSawLevel, sawLevel);

    attach(ConduitPolysynth::pmPWWidth, pulseWidth);
    attach(ConduitPolysynth::pmPWFrequencyDiv, pulseOctave);
    attach(ConduitPolysynth::pmPWCoarse, pulseCoarse);
    attach(ConduitPolysynth::pmPWFine, pulseFine);
    attach(ConduitPolysynth::pmPWLevel, pulseLevel);

    attach(ConduitPolysynth::pmSinFrequencyDiv, sinOctave);
    attach(ConduitPolysynth::pmSinCoarse, sinCoarse);
    attach(ConduitPolysynth::pmSinLevel, sinLevel);

    attach(ConduitPolysynth::pmNoiseColor, noiseColor);
    attach(ConduitPolysynth::pmNoiseLevel, noiseLevel);

    attach(ConduitPolysynth::pmSVFCutoff, svfCutoff);
    attach(ConduitPolysynth::pmSVFResonance, svfResonance);
    attach(ConduitPolysynth::pmSVFKeytrack, svfKeytrack);

    attach(ConduitPolysynth::pmLPFCutoff, lpfCutoff);
    attach(ConduitPolysynth::pmLPFResonance, lpfResonance);
    attach(ConduitPolysynth::pmLPFKeytrack, lpfKeytrack);

    attach(ConduitPolysynth::pmEnvA, aegValues.attack);
    attach(ConduitPolysynth::pmEnvD, aegValues.decay);
    attach(ConduitPolysynth::pmEnvS, aegValues.sustain);
    attach(ConduitPolysynth::pmEnvR, aegValues.release);

    attach(ConduitPolysynth::pmAegPreFilterGain, aegPFG);

    attach(ConduitPolysynth::pmEnvA + ConduitPolysynth::offPmFeg, fegValues.attack);
    attach(ConduitPolysynth::pmEnvD + ConduitPolysynth::offPmFeg, fegValues.decay);
    attach(ConduitPolysynth::pmEnvS + ConduitPolysynth::offPmFeg, fegValues.sustain);
    attach(ConduitPolysynth::pmEnvR + ConduitPolysynth::offPmFeg, fegValues.release);

    attach(ConduitPolysynth::pmFegToSVFCutoff, fegToSvfCutoff);
    attach(ConduitPolysynth::pmFegToLPFCutoff, fegToLPFCutoff);

    attach(ConduitPolysynth::pmWSDrive, wsDrive);
    attach(ConduitPolysynth::pmWSBias, wsBias);

    attach(ConduitPolysynth::pmFilterFeedback, filterFeedback);

    attach(ConduitPolysynth::pmVoiceLevel, outputLevel);
    attach(ConduitPolysynth::pmVoicePan, outputPan);

    attach(ConduitPolysynth::pmLFORate, lfoData[0].rate);
    attach(ConduitPolysynth::pmLFODeform, lfoData[0].deform);
    attach(ConduitPolysynth::pmLFOAmplitude, lfoData[0].amplitude);

    attach(ConduitPolysynth::pmLFORate + ConduitPolysynth::offPmLFO2, lfoData[1].rate);
    attach(ConduitPolysynth::pmLFODeform + ConduitPolysynth::offPmLFO2, lfoData[1].deform);
    attach(ConduitPolysynth::pmLFOAmplitude + ConduitPolysynth::offPmLFO2, lfoData[1].amplitude);


    attach(ConduitPolysynth::pmAegVelocitySens, velocitySens);

    mtsClient = p.mtsClient;
}

void PolysynthVoice::applyExternalMod(clap_id param, float value)
{
    auto emit = externalMods.find(param);
    if (emit != externalMods.end())
    {
        emit->second = value;
    }
}

void PolysynthVoice::receiveNoteExpression(int expression, double value)
{
    switch (expression)
    {
    case CLAP_NOTE_EXPRESSION_TUNING:
    {
        pitchNoteExpressionValue = value;
        recalcPitch();
    }
    break;
    }
}


} // namespace sst::conduit::polysynth