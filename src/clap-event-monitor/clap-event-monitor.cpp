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

#include "clap-event-monitor.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "version.h"

namespace sst::conduit::clap_event_monitor
{
const char *features[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_DELAY, nullptr};
clap_plugin_descriptor desc = {CLAP_VERSION,
                               "org.surge-synth-team.conduit.clap-event-monitor",
                               "Conduit Clap Event Monitor",
                               "Surge Synth Team",
                               "https://surge-synth-team.org",
                               "",
                               "",
                               sst::conduit::build::FullVersionStr,
                               "The Conduit ClapEventMonitor is a work in progress",
                               features};

ConduitClapEventMonitor::ConduitClapEventMonitor(const clap_host *host)
    : sst::conduit::shared::ClapBaseClass<ConduitClapEventMonitor, ConduitClapEventMonitorConfig>(
          &desc, host)
{
    auto autoFlag = CLAP_PARAM_IS_AUTOMATABLE;
    auto steppedFlag = autoFlag | CLAP_PARAM_IS_STEPPED;
    auto modFlag = autoFlag | CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_MODULATABLE_PER_CHANNEL |
                   CLAP_PARAM_IS_MODULATABLE_PER_KEY | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID;

    paramDescriptions.push_back(ParamDesc()
                                    .asInt()
                                    .withID(pmStepped)
                                    .withName("Stepped Param")
                                    .withGroupName("Monitor")
                                    .withRange(-4, 17)
                                    .withDefault(2)
                                    .withLinearScaleFormatting("things")
                                    .withFlags(steppedFlag));

    paramDescriptions.push_back(ParamDesc()
                                    .asPercentBipolar()
                                    .withID(pmAuto)
                                    .withName("Automatable Param")
                                    .withGroupName("Monitor")
                                    .withFlags(autoFlag));

    paramDescriptions.push_back(ParamDesc()
                                    .asPercentBipolar()
                                    .withID(pmMod)
                                    .withName("Modulatable Param")
                                    .withGroupName("Monitor")
                                    .withFlags(modFlag));

    configureParams();

    clapJuceShim = std::make_unique<sst::clap_juce_shim::ClapJuceShim>(this);
    clapJuceShim->setResizable(true);
}

ConduitClapEventMonitor::~ConduitClapEventMonitor() {}

bool ConduitClapEventMonitor::audioPortsInfo(uint32_t index, bool isInput,
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

bool ConduitClapEventMonitor::notePortsInfo(uint32_t index, bool isInput,
                                            clap_note_port_info *info) const noexcept
{
    info->id = isInput ? 178 : 23;
    info->supported_dialects = CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_CLAP;
    info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
    strncpy(info->name, (std::string("Note ") + (isInput ? "In" : "Out")).c_str(),
            CLAP_NAME_SIZE - 1);
    return true;
}
clap_process_status ConduitClapEventMonitor::process(const clap_process *process) noexcept
{
    auto ev = process->in_events;
    auto ov = process->out_events;
    auto sz = ev->size(ev);

    for (auto i = 0U; i < sz; ++i)
    {
        auto et = ev->get(ev, i);
        uiComms.dataCopyForUI.writeEventTo(et);

        ov->try_push(ov, et);
    }

    return CLAP_PROCESS_CONTINUE;
}

} // namespace sst::conduit::clap_event_monitor
