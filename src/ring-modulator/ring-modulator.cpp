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

#include "ring-modulator.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "version.h"

namespace sst::conduit::ring_modulator
{
const char *features[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_DELAY, nullptr};
clap_plugin_descriptor desc = {CLAP_VERSION,
                               "org.surge-synth-team.conduit.ring-modulator",
                               "Conduit Ring Modulator",
                               "Surge Synth Team",
                               "https://surge-synth-team.org",
                               "",
                               "",
                               sst::conduit::build::FullVersionStr,
                               "The Conduit RingModulator is a work in progress",
                               features};

ConduitRingModulator::ConduitRingModulator(const clap_host *host)
    : sst::conduit::shared::ClapBaseClass<ConduitRingModulator, ConduitRingModulatorConfig>(&desc,
                                                                                            host),
      hr_up(6, true), hr_scup(6, true), hr_down(6, true)
{
    auto autoFlag = CLAP_PARAM_IS_AUTOMATABLE;
    auto steppedFlag = autoFlag | CLAP_PARAM_IS_STEPPED;

    paramDescriptions.push_back(ParamDesc()
                                    .asPercent()
                                    .withID(pmMixLevel)
                                    .withName("Mix Level")
                                    .withGroupName("Ring Modulator")
                                    .withDefault(0.8)
                                    .withFlags(autoFlag));

    paramDescriptions.push_back(
        ParamDesc()
            .asInt()
            .withID(pmAlgo)
            .withName("Algorithm")
            .withGroupName("Ring Modulator")
            .withDefault(0)
            .withRange(0, 1)
            .withFlags(steppedFlag)
            .withUnorderedMapFormatting({{algoDigital, "Digital"}, {algoAnalog, "Analog"}}));

    paramDescriptions.push_back(
        ParamDesc()
            .asInt()
            .withID(pmSource)
            .withName("Modulator Source")
            .withGroupName("Ring Modulator")
            .withDefault(0)
            .withRange(0, 1)
            .withFlags(steppedFlag)
            .withUnorderedMapFormatting({{srcInternal, "Internal"}, {srcSidechain, "Sidechain"}}));

    paramDescriptions.push_back(ParamDesc()
                                    .asFloat()
                                    .withID(pmInternalSourceFrequency)
                                    .asAudibleFrequency()
                                    .withFlags(autoFlag)
                                    .withName("Source Frequency")
                                    .withGroupName("Ring Modulator"));
    configureParams();

    attachParam(pmMixLevel, mix);
    attachParam(pmInternalSourceFrequency, freq);

    attachParam(pmAlgo, algo);
    attachParam(pmSource, src);

    clapJuceShim = std::make_unique<sst::clap_juce_shim::ClapJuceShim>(this);
    clapJuceShim->setResizable(true);
}

ConduitRingModulator::~ConduitRingModulator() {}

bool ConduitRingModulator::audioPortsInfo(uint32_t index, bool isInput,
                                          clap_audio_port_info *info) const noexcept
{
    static constexpr uint32_t inId{16}, outId{72};
    if (isInput)
    {
        if (index == 0)
        {
            info->id = inId;
            info->in_place_pair = CLAP_INVALID_ID;
            strncpy(info->name, "main input", sizeof(info->name));
            info->flags = CLAP_AUDIO_PORT_IS_MAIN;
            info->channel_count = 2;
            info->port_type = CLAP_PORT_STEREO;
        }
        else
        {
            info->id = inId;
            info->in_place_pair = CLAP_INVALID_ID;
            strncpy(info->name, "ring sidechain", sizeof(info->name));
            info->flags = 0;
            info->channel_count = 2;
            info->port_type = CLAP_PORT_STEREO;
        }
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

clap_process_status ConduitRingModulator::process(const clap_process *process) noexcept
{
    if (process->audio_outputs_count <= 0)
        return CLAP_PROCESS_SLEEP;
    if (process->audio_inputs_count <= 0)
        return CLAP_PROCESS_SLEEP;

    float **out = process->audio_outputs[0].data32;
    auto ochans = process->audio_outputs->channel_count;

    float **const in = process->audio_inputs[0].data32;
    auto ichans = process->audio_inputs->channel_count;

    float **const sidechain = process->audio_inputs[1].data32;
    auto scchans = process->audio_inputs->channel_count;

    assert(ochans == 2 || ichans == 2 || scchans == 2);

    handleEventsFromUIQueue(process->out_events);

    auto chans = std::min({ochans, ichans, scchans});
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

        inputBuf[0][pos] = in[0][i];
        inputBuf[1][pos] = in[1][i];
        sidechainBuf[0][pos] = sidechain[0][i];
        sidechainBuf[1][pos] = sidechain[1][i];

        out[0][i] = outBuf[0][pos] * mix.v + inMixBuf[0][pos] * (1 - mix.v);
        out[1][i] = outBuf[1][pos] * mix.v + inMixBuf[1][pos] * (1 - mix.v);

        pos++;

        if (pos == blockSize)
        {
            memcpy(inMixBuf, inputBuf, sizeof(inMixBuf));
            hr_up.process_block_U2(inputBuf[0], inputBuf[1], inputOS[0], inputOS[1], blockSizeOS);

            if ((Source)(*src) == srcInternal)
            {
                static constexpr double mf0{8.17579891564};
                internalSource.setRate(2.0 * M_PI * note_to_pitch_ignoring_tuning(freq.v + 69) *
                                       mf0 * dsamplerate_inv * 0.5); // 0.5 for oversample

                for (int i = 0; i < blockSizeOS; ++i)
                {
                    internalSource.step();
                    inputOS[0][i] *= 2 * internalSource.u;
                    inputOS[1][i] *= 2 * internalSource.u;
                }
            }
            else
            {
                hr_scup.process_block_U2(sidechainBuf[0], sidechainBuf[1], sidechainBufOS[0],
                                         sidechainBufOS[1], blockSizeOS);
                for (int i = 0; i < blockSizeOS; ++i)
                {
                    inputOS[0][i] *= 4 * sidechainBufOS[0][i];
                    inputOS[1][i] *= 4 * sidechainBufOS[1][i];
                }
            }
            hr_down.process_block_D2(inputOS[0], inputOS[1], blockSizeOS, outBuf[0], outBuf[1]);
            pos = 0;
        }

        processLags();
    }
    return CLAP_PROCESS_CONTINUE;
}

void ConduitRingModulator::handleInboundEvent(const clap_event_header_t *evt)
{
    if (handleParamBaseEvents(evt))
    {
        return;
    }

    // Other events just get dropped right now
}
} // namespace sst::conduit::ring_modulator