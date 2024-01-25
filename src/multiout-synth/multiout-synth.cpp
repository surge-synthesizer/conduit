/*
 * Conduit - a project highlighting CLAP-first development
 *           and exercising the surge synth team libraries.
 *
 * Copyright 2023-2024 Paul Walker and authors in github
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

#include "multiout-synth.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "version.h"

#include "sst/cpputils/constructors.h"
#include "sst/cpputils/iterators.h"

namespace sst::conduit::multiout_synth
{
const clap_plugin_descriptor *ConduitMultiOutSynthConfig::getDescription()
{
    static const char *features[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT,
                                     CLAP_PLUGIN_FEATURE_SYNTHESIZER, nullptr};
    static clap_plugin_descriptor desc = {CLAP_VERSION,
                                          "org.surge-synth-team.conduit.multiout_synth",
                                          "Conduit MultiOut Clock Synth",
                                          "Surge Synth Team",
                                          "https://surge-synth-team.org",
                                          "",
                                          "",
                                          sst::conduit::build::FullVersionStr,
                                          "The Conduit MultiOutSynth is a work in progress",
                                          &features[0]};
    return &desc;
}

ConduitMultiOutSynth::ConduitMultiOutSynth(const clap_host *host)
    : sst::conduit::shared::ClapBaseClass<ConduitMultiOutSynth, ConduitMultiOutSynthConfig>(host),
      chans{sst::cpputils::make_array<Chan, nOuts>(this)}
{
    auto autoFlag = CLAP_PARAM_IS_AUTOMATABLE;
    auto steppedFlag = autoFlag | CLAP_PARAM_IS_STEPPED;

    auto nts = std::vector<int>{60, 64, 67, 70};
    for (int i = 0; i < nOuts; ++i)
    {
        chans[i].chan = i;
        paramDescriptions.push_back(ParamDesc()
                                        .asFloat()
                                        .withID(pmFreq0 + i)
                                        .withName("Frequency " + std::to_string(i + 1))
                                        .withGroupName("Output " + std::to_string(i + 1))
                                        .withRange(48, 96)
                                        .withDefault(nts[i] + 12)
                                        .withSemitoneZeroAtMIDIZeroFormatting()
                                        .withFlags(autoFlag));

        paramDescriptions.push_back(ParamDesc()
                                        .asFloat()
                                        .withID(pmTime0 + i)
                                        .withName("Time Between Pulses " + std::to_string(i + 1))
                                        .withGroupName("Output " + std::to_string(i + 1))
                                        .withRange(0.1f, 2.f)
                                        .withDefault(0.5 + i * 0.278234)
                                        .withLinearScaleFormatting("seconds")
                                        .withFlags(autoFlag));

        paramDescriptions.push_back(ParamDesc()
                                        .asBool()
                                        .withID(pmMute0 + i)
                                        .withName("Mute " + std::to_string(i + 1))
                                        .withGroupName("Output " + std::to_string(i + 1))
                                        .withFlags(steppedFlag));
    }

    configureParams();

    for (int i = 0; i < nOuts; ++i)
    {
        attachParam(pmFreq0 + i, chans[i].freq);
        attachParam(pmTime0 + i, chans[i].time);
        attachParam(pmMute0 + i, chans[i].mute);
    }

    clapJuceShim = std::make_unique<sst::clap_juce_shim::ClapJuceShim>(this);
    clapJuceShim->setResizable(true);
}

ConduitMultiOutSynth::~ConduitMultiOutSynth() {}

bool ConduitMultiOutSynth::audioPortsInfo(uint32_t index, bool isInput,
                                          clap_audio_port_info *info) const noexcept
{
    if (!isInput)
    {
        info->id = 652 + index * 13;
        info->in_place_pair = CLAP_INVALID_ID;
        if (index == 0)
        {
            strncpy(info->name, "main output", sizeof(info->name));
            info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        }
        else
        {
            snprintf(info->name, sizeof(info->name) - 1, "aux output %d", index);
        }
        info->channel_count = 2;
        info->port_type = CLAP_PORT_STEREO;

        return true;
    }
    return false;
}

bool ConduitMultiOutSynth::notePortsInfo(uint32_t index, bool isInput,
                                         clap_note_port_info *info) const noexcept
{
    info->id = 8427;
    info->supported_dialects = CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_CLAP;
    info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
    strncpy(info->name, "MIDI In", CLAP_NAME_SIZE - 1);
    return true;
}

clap_process_status ConduitMultiOutSynth::process(const clap_process *process) noexcept
{
    auto ev = process->in_events;
    auto sz = ev->size(ev);

    // This pointer is the sentinel to our next event which we advance once an event is processed
    const clap_event_header_t *nextEvent{nullptr};
    uint32_t nextEventIndex{0};
    if (sz != 0)
    {
        nextEvent = ev->get(ev, nextEventIndex);
    }

    for (auto s = 0U; s < process->frames_count; ++s)
    {
        while (nextEvent && nextEvent->time == s)
        {
            handleParamBaseEvents(nextEvent);
            nextEventIndex++;
            if (nextEventIndex >= sz)
                nextEvent = nullptr;
            else
                nextEvent = ev->get(ev, nextEventIndex);
        }

        for (auto &c : chans)
        {
            c.env.process(0.0, 0.1, 0.1, 0.1, 0, 0, 0, true);
            c.timeSinceTrigger += sampleRateInv;
            if (c.timeSinceTrigger > *(c.time))
            {
                c.timeSinceTrigger -= *(c.time);
                c.env.attackFrom(0, 0.1, 0, true);

                c.osc.setRate(2.0 * M_PI * 440.0 * pow(2.f, (*(c.freq) - 69) / 12) *
                              dsamplerate_inv);
            }
            auto v = c.env.output * c.osc.u;
            c.osc.step();
            process->audio_outputs[c.chan].data32[0][s] = v;
            process->audio_outputs[c.chan].data32[1][s] = v;
        }
    }

    return CLAP_PROCESS_CONTINUE;
}

} // namespace sst::conduit::multiout_synth
