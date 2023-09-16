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

#include "polymetric-delay.h"
#include "version.h"
#include "sst/basic-blocks/dsp/PanLaws.h"

namespace sst::conduit::polymetric_delay
{
const char *features[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_DELAY, nullptr};
clap_plugin_descriptor desc = {CLAP_VERSION,
                               "org.surge-synth-team.conduit.polymetric-delay",
                               "Conduit Polymetric Delay",
                               "Surge Synth Team",
                               "https://surge-synth-team.org",
                               "",
                               "",
                               sst::conduit::build::FullVersionStr,
                               "The Conduit Polymetric Delay is a work in progress",
                               features};

ConduitPolymetricDelay::ConduitPolymetricDelay(const clap_host *host)
    : sst::conduit::shared::ClapBaseClass<ConduitPolymetricDelay, ConduitPolymetricDelayConfig>(
          &desc, host)
{
    auto autoFlag = CLAP_PARAM_IS_AUTOMATABLE;
    auto modFlag = autoFlag | CLAP_PARAM_IS_MODULATABLE;
    auto steppedFlag = autoFlag | CLAP_PARAM_IS_STEPPED;

    paramDescriptions.push_back(ParamDesc()
                                    .asFloat()
                                    .asCubicDecibelAttenuation()
                                    .withID(pmDryLevel)
                                    .withName("Dry Output Level")
                                    .withGroupName("Main")
                                    .withDefault(0.5)
                                    .withFlags(autoFlag));

    for (int i = 0; i < nTaps; ++i)
    {
        auto gn = std::string("Tap ") + std::to_string(i + 1);
        auto tm = std::string(" (Tap ") + std::to_string(i + 1) + ")";
        paramDescriptions.push_back(ParamDesc()
                                        .asInt()
                                        .withFlags(steppedFlag)
                                        .withID(pmDelayTimeNTaps + i)
                                        .withName("N Taps" + tm)
                                        .withGroupName(gn)
                                        .withRange(1, 64)
                                        .withDefault(3 + i)
                                        .withLinearScaleFormatting("taps"));
        paramDescriptions.push_back(ParamDesc()
                                        .asInt()
                                        .withFlags(steppedFlag)
                                        .withID(pmDelayTimeEveryM + i)
                                        .withName("Every M Beats" + tm)
                                        .withGroupName(gn)
                                        .withRange(1, 32)
                                        .withDefault(2 + i)
                                        .withLinearScaleFormatting("beats"));
        paramDescriptions.push_back(ParamDesc()
                                        .asFloat()
                                        .withRange(log2(0.05 / 440) * 12, log2(6000.f / 440) * 12)
                                        .withDefault(log2(1.0 / 440.0) * 12)
                                        .withSemitoneZeroAt400Formatting()
                                        .withFlags(autoFlag)
                                        .withID(pmDelayModRate + i)
                                        .withName("Mod Rate" + tm)
                                        .withGroupName(gn));
        paramDescriptions.push_back(ParamDesc()
                                        .asFloat()
                                        .withFlags(autoFlag)
                                        .withID(pmDelayModDepth + i)
                                        .withName("Mod Depth" + tm)
                                        .withGroupName(gn)
                                        .withRange(0, 1)
                                        .withDefault(0)
                                        .withLinearScaleFormatting("seconds"));
        paramDescriptions.push_back(ParamDesc()
                                        .asBool()
                                        .withFlags(steppedFlag)
                                        .withID(pmTapActive + i)
                                        .withName("Tap Active" + tm)
                                        .withGroupName(gn)
                                        .withDefault(1));
        paramDescriptions.push_back(ParamDesc()
                                        .asAudibleFrequency()
                                        .withFlags(autoFlag)
                                        .withID(pmTapLowCut + i)
                                        .withName("Lo Cut" + tm)
                                        .withGroupName(gn)
                                        .withDefault(-60));
        paramDescriptions.push_back(ParamDesc()
                                        .asAudibleFrequency()
                                        .withFlags(autoFlag)
                                        .withID(pmTapHighCut + i)
                                        .withName("Hi Cut" + tm)
                                        .withGroupName(gn)
                                        .withDefault(70));
        paramDescriptions.push_back(ParamDesc()
                                        .asFloat()
                                        .asCubicDecibelAttenuation()
                                        .withFlags(autoFlag)
                                        .withID(pmTapLevel + i)
                                        .withName("Level" + tm)
                                        .withGroupName(gn)
                                        .withDefault(1.0f - 0.1f * i));
        paramDescriptions.push_back(ParamDesc()
                                        .asFloat()
                                        .asCubicDecibelAttenuation()
                                        .withFlags(autoFlag)
                                        .withID(pmTapFeedback + i)
                                        .withName("Feedback" + tm)
                                        .withGroupName(gn)
                                        .withDefault(0.2));
        paramDescriptions.push_back(ParamDesc()
                                        .asFloat()
                                        .asCubicDecibelAttenuation()
                                        .withFlags(autoFlag)
                                        .withID(pmTapCrossFeedback + i)
                                        .withName("Cross Feedback " + tm)
                                        .withGroupName(gn)
                                        .withDefault(0.0));
        paramDescriptions.push_back(ParamDesc()
                                        .asFloat()
                                        .asPercentBipolar()
                                        .withFlags(autoFlag)
                                        .withID(pmTapOutputPan + i)
                                        .withName("Pan " + tm)
                                        .withGroupName(gn)
                                        .withDefault(0.0));
    }

    configureParams();

    attachParam(pmDryLevel, dryLev);

    for (int i = 0; i < nTaps; ++i)
    {
        attachParam(pmDelayTimeNTaps + i, tapData[i].ntaps);
        attachParam(pmDelayTimeEveryM + i, tapData[i].mbeats);
        attachParam(pmTapLevel + i, tapData[i].level);
        attachParam(pmTapFeedback + i, tapData[i].fblev);
        attachParam(pmTapCrossFeedback + i, tapData[i].crossfblev);
        attachParam(pmTapActive + i, tapData[i].active);
        attachParam(pmTapOutputPan + i, tapData[i].pan);

        attachParam(pmTapLowCut + i, tapData[i].locut);
        attachParam(pmTapHighCut + i, tapData[i].hicut);

        hp[i].storage = this;
        lp[i].storage = this;
    }

    clapJuceShim = std::make_unique<sst::clap_juce_shim::ClapJuceShim>(this);
    clapJuceShim->setResizable(false);
}

ConduitPolymetricDelay::~ConduitPolymetricDelay() {}

bool ConduitPolymetricDelay::audioPortsInfo(uint32_t index, bool isInput,
                                            clap_audio_port_info *info) const noexcept
{
    static constexpr uint32_t inId{16}, outId{72};
    if (isInput)
    {
        info->id = inId;
        info->in_place_pair = CLAP_INVALID_ID;
        strncpy(info->name, "main input", sizeof(info->name));
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->channel_count = 2;
        info->port_type = CLAP_PORT_STEREO;

        return true;
    }
    else
    {
        info->id = outId;
        info->in_place_pair = CLAP_INVALID_ID;
        strncpy(info->name, "main output", sizeof(info->name));
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->channel_count = 2;
        info->port_type = CLAP_PORT_STEREO;

        return true;
    }
    return false;
}

clap_process_status ConduitPolymetricDelay::process(const clap_process *process) noexcept
{
    if (process->audio_outputs_count <= 0)
        return CLAP_PROCESS_SLEEP;
    if (process->audio_inputs_count <= 0)
        return CLAP_PROCESS_SLEEP;

    float **out = process->audio_outputs[0].data32;
    auto ochans = process->audio_outputs->channel_count;

    float **const in = process->audio_inputs[0].data32;
    auto ichans = process->audio_inputs->channel_count;

    while (!uiComms.fromUiQ.empty())
    {
        auto r = *uiComms.fromUiQ.pop();
        generateOutputMessagesFromUI(r, process->out_events);
        if (r.type == FromUI::ADJUST_VALUE)
        {
            doValueUpdate(r.id, r.value);
            if (isTapParam(r.id, pmDelayTimeEveryM) || isTapParam(r.id, pmDelayTimeNTaps))
            {
                recalcTaps();
            }
        }
    }
    refreshUIIfNeeded();

    auto chans = std::min(ochans, ichans);
    if (chans < 2)
        return CLAP_PROCESS_SLEEP;

    auto ev = process->in_events;
    auto sz = ev->size(ev);

    // This pointer is the sentinel to our next event which we advance once an event is processed
    const clap_event_header_t *nextEvent{nullptr};
    uint32_t nextEventIndex{0};
    if (sz != 0)
    {
        nextEvent = ev->get(ev, nextEventIndex);
    }

    if (process->transport)
    {
        handleInboundEvent((const clap_event_header *)(process->transport));
    }

    float inMx[2]{0, 0}, outMx[2]{0, 0}, tapMx[nTaps][2]{};
    bool active[nTaps];
    for (int i = 0; i < nTaps; ++i)
    {
        active[i] = *(tapData[i].active) > 0.5;
    }

    for (int i = 0; i < process->frames_count; ++i)
    {
        while (nextEvent && nextEvent->time == i)
        {
            handleInboundEvent(nextEvent);
            nextEventIndex++;
            if (nextEventIndex >= sz)
                nextEvent = nullptr;
            else
                nextEvent = ev->get(ev, nextEventIndex);
        }

        if (slowProcess >= blockSize)
        {
            slowProcess = 0;
            inVU.process(inMx[0], inMx[1]);
            outVU.process(outMx[0], outMx[1]);
            inMx[0] = 0;
            inMx[1] = 0;
            outMx[0] = 0;
            outMx[1] = 0;

            for (int t = 0; t < nTaps; ++t)
            {
                tapOutVU[t].process(tapMx[t][0], tapMx[t][1]);

                tapMx[t][0] = 0;
                tapMx[t][1] = 0;

                // Recalc pan laws
                sst::basic_blocks::dsp::pan_laws::stereoEqualPower((tapData[t].pan.v + 1) * 0.5,
                                                                   tapPanMatrix[t]);

                setTapFilterFrequencies(t);
            }
        }
        slowProcess++;

        float totalTapOut[2]{};
        float totalTapFB[2]{};
        for (int tap = 0; tap < nTaps; ++tap)
        {
            if (!active[tap])
                continue;

            auto tl = tapData[tap].level.v;
            tl = tl * tl * tl;
            auto ftl = tapData[tap].fblev.v;
            ftl = ftl * ftl * ftl;
            auto cftl = tapData[tap].crossfblev.v;
            cftl = cftl * cftl * cftl;

            auto smpL = delayLine[0].read(baseTapSamples[tap]);
            auto smpR = delayLine[1].read(baseTapSamples[tap]);

            auto dL = smpL * tapPanMatrix[tap][0] + smpR * tapPanMatrix[tap][2];
            auto dR = smpR * tapPanMatrix[tap][1] + smpL * tapPanMatrix[tap][3];

            dL = dL * tl;
            dR = dR * tl;

            hp[tap].process_sample(dL, dR, dL, dR);
            lp[tap].process_sample(dL, dR, dL, dR);

            tapMx[tap][0] = std::max(tapMx[tap][0], std::abs(dL));
            tapMx[tap][1] = std::max(tapMx[tap][1], std::abs(dR));

            totalTapOut[0] += dL;
            totalTapOut[1] += dR;

            totalTapFB[0] += smpL * ftl + smpR * cftl;
            totalTapFB[1] += smpR * ftl + smpL * cftl;
        }

        auto dl = (*dryLev);
        dl = dl * dl * dl;
        for (int c = 0; c < chans; ++c)
        {
            out[c][i] = in[c][i] * dl + totalTapOut[c];

            delayLine[c].write(in[c][i] + totalTapFB[c]);
            inMx[c] = std::max(inMx[c], std::abs(in[c][i]));
            outMx[c] = std::max(outMx[c], std::abs(out[c][i]));
        }

        processLags();
    }

    for (int c = 0; c < 2; ++c)
    {
        uiComms.dataCopyForUI.inVu[c] = inVU.vu_peak[c];
        uiComms.dataCopyForUI.outVu[c] = outVU.vu_peak[c];
        for (int t = 0; t < nTaps; ++t)
        {
            uiComms.dataCopyForUI.tapVu[t][c] = tapOutVU[t].vu_peak[c];
        }
    }

    return CLAP_PROCESS_CONTINUE;
}

void ConduitPolymetricDelay::handleInboundEvent(const clap_event_header_t *evt)
{
    // Other events just get dropped right now
    if (evt->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;

    switch (evt->type)
    {
    case CLAP_EVENT_PARAM_VALUE:
    {
        auto v = reinterpret_cast<const clap_event_param_value *>(evt);
        updateParamInPatch(v);

        if (isTapParam(v->param_id, pmDelayTimeEveryM) || isTapParam(v->param_id, pmDelayTimeNTaps))
        {
            recalcTaps();
        }
    }
    break;

    case CLAP_EVENT_TRANSPORT:
    {
        auto tev = reinterpret_cast<const clap_event_transport_t *>(evt);
        auto ptempo = tempo;
        tempo = tev->tempo;
        if (ptempo != tempo)
        {
            recalcTaps();
        }

        uiComms.dataCopyForUI.tempo = tev->tempo;
        uiComms.dataCopyForUI.bar_start = tev->bar_start;
        uiComms.dataCopyForUI.bar_number = tev->bar_number;
        uiComms.dataCopyForUI.song_pos_beats = tev->song_pos_beats;
        uiComms.dataCopyForUI.tsig_num = tev->tsig_num;
        uiComms.dataCopyForUI.tsig_denom = tev->tsig_denom;

        uiComms.dataCopyForUI.isPlayingOrRecording =
            (tev->flags & CLAP_TRANSPORT_IS_PLAYING) || (tev->flags & CLAP_TRANSPORT_IS_RECORDING);
    }
    break;
    }
}

void ConduitPolymetricDelay::recalcTaps()
{
    // 120 beats per minute is
    // 2 beats per second
    // is 1/2 second per beat
    // is sr/2 samples per beat
    // 240 bpm is
    // 1/4 second per beat sr/4 sample per beat
    // so samples ber beat is sampleRate * 60 / tempo
    double spb = sampleRate * 60.0 / tempo;

    // CNDOUT << "Recalculating Taps" << std::endl;
    for (int i = 0; i < nTaps; ++i)
    {
        auto n = (int)(*tapData[i].ntaps);
        auto m = (int)(*tapData[i].mbeats);
        baseTapSamples[i] = 1.f * spb * m / n;
        // CNDOUT << CNDVAR(n) << CNDVAR(m) << CNDVAR(spb) << CNDVAR(baseTapSamples[i]) <<
        // std::endl;
    }
}

} // namespace sst::conduit::polymetric_delay