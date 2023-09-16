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
                                    .asPercent()
                                    .withID(pmMixLevel)
                                    .withName("Mix Level")
                                    .withGroupName("Main")
                                    .withDefault(0.8)
                                    .withFlags(autoFlag));

    paramDescriptions.push_back(ParamDesc()
                                    .asPercent()
                                    .withID(pmFeedbackLevel)
                                    .withName("Feedback Level")
                                    .withGroupName("Main")
                                    .withDefault(0.2)
                                    .withFlags(autoFlag));

    for (int i = 0; i < nTaps; ++i)
    {
        auto gn = std::string("Tap ") + std::to_string(i + 1);
        auto tm = std::string(" (") + std::to_string(i + 1) + ")";
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
                                        .withFlags(autoFlag)
                                        .withID(pmDelayTimeFineSeconds + i)
                                        .withName("Fine Adjust" + tm)
                                        .withGroupName(gn)
                                        .withRange(-0.1, 0.1)
                                        .withDefault(0)
                                        .withLinearScaleFormatting("seconds"));
        paramDescriptions.push_back(ParamDesc()
                                        .asLog2SecondsRange(-3, 4)
                                        .withFlags(autoFlag)
                                        .withID(pmDelayModRate + i)
                                        .withName("Mod Rate" + tm)
                                        .withGroupName(gn)
                                        .withDefault(0));
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
    }

    configureParams();

    attachParam(pmMixLevel, mix);
    attachParam(pmFeedbackLevel, feedback);

    memset(delayBuffer, 0, sizeof(delayBuffer));

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

    handleEventsFromUIQueue(process->out_events);

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
        handleInboundEvent((const clap_event_header *)(process->transport));

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

        for (int c = 0; c < chans; ++c)
        {
            out[c][i] = in[c][i] * (*mix);
        }
    }
    return CLAP_PROCESS_CONTINUE;
}

void ConduitPolymetricDelay::handleInboundEvent(const clap_event_header_t *evt)
{
    if (handleParamBaseEvents(evt))
    {
        return;
    }

    // Other events just get dropped right now
    if (evt->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;

    if (evt->type == CLAP_EVENT_TRANSPORT)
    {
        auto tev = reinterpret_cast<const clap_event_transport_t *>(evt);
        uiComms.dataCopyForUI.tempo = tev->tempo;
        uiComms.dataCopyForUI.bar_start = tev->bar_start;
        uiComms.dataCopyForUI.bar_number = tev->bar_number;
        uiComms.dataCopyForUI.song_pos_beats = tev->song_pos_beats;
        uiComms.dataCopyForUI.tsig_num = tev->tsig_num;
        uiComms.dataCopyForUI.tsig_denom = tev->tsig_denom;

        uiComms.dataCopyForUI.isPlayingOrRecording =
            (tev->flags & CLAP_TRANSPORT_IS_PLAYING) || (tev->flags & CLAP_TRANSPORT_IS_RECORDING);
    }
}
} // namespace sst::conduit::polymetric_delay