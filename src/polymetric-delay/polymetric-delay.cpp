/*
 * Conduit - a series of demonstration and fun plugins
 *
 * Copyright 2023 Paul Walker and authors in github
 *
 * This file you are viewing now is released under the
 * MIT license, but the assembled program which results
 * from compiling it has GPL3 dependencies, so the total
 * program is a GPL3 program. More details to come.
 *
 * Basically before I give this to folks, document this bit and
 * replace these headers
 *
 */

#include "polymetric-delay.h"
#include "juce_gui_basics/juce_gui_basics.h"

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
                               "0.1.0",
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
                                    .withID(pmDelayInSamples)
                                    .withName("Delay in Samples")
                                    .withGroupName("Delay")
                                    .withRange(20, 48000)
                                    .withDefault(4800)
                                    .withLinearScaleFormatting("samples")
                                    .withFlags(autoFlag));

    paramDescriptions.push_back(ParamDesc()
                                    .asPercent()
                                    .withID(pmMixLevel)
                                    .withName("Mix Level")
                                    .withGroupName("Delay")
                                    .withDefault(0.8)
                                    .withFlags(autoFlag));

    paramDescriptions.push_back(ParamDesc()
                                    .asPercent()
                                    .withID(pmFeedbackLevel)
                                    .withName("Feedback Level")
                                    .withGroupName("Delay")
                                    .withDefault(0.2)
                                    .withFlags(autoFlag));

    configureParams();

    attachParam(pmDelayInSamples, sampleTime);
    attachParam(pmMixLevel, mix);
    attachParam(pmFeedbackLevel, feedback);

    memset(delayBuffer, 0, sizeof(delayBuffer));

    clapJuceShim = std::make_unique<sst::clap_juce_shim::ClapJuceShim>(this);
    clapJuceShim->setResizable(true);
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
            auto rp = (wp + bufSize + (int)(*sampleTime)) & (bufSize - 1);
            out[c][i] = in[c][i] * (1.0 - *mix) + delayBuffer[c][rp] * (*mix);

            delayBuffer[c][wp] = in[c][i] + delayBuffer[c][rp] * (*feedback);
        }
        wp = (wp + 1) & (bufSize - 1);
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
}
} // namespace sst::conduit::polymetric_delay