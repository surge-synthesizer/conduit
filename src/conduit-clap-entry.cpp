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

/*
 * This file provides the `clap_plugin_entry` entry point required in the DLL for all
 * clap plugins. It also provides the basic functions for the resulting factory class
 * which generates the plugin. In a single plugin case, this really is just plumbing
 * through to expose polysynth::ConduitPolysynthConfig::getDescription() and create a polysynth
 * plugin instance using the helper classes.
 *
 * For more information on this mechanism, see include/clap/entry.h
 */

#include <iostream>
#include <cmath>
#include <cstring>

#include <clap/plugin.h>
#include <clap/factory/plugin-factory.h>

#include "clapwrapper/vst3.h"
#include "clapwrapper/auv2.h"

#include "conduit-shared/debug-helpers.h"

#include "polysynth/polysynth.h"
#include "polymetric-delay/polymetric-delay.h"
#include "chord-memory/chord-memory.h"
#include "ring-modulator/ring-modulator.h"
#include "clap-event-monitor/clap-event-monitor.h"
#include "mts-to-noteexpression/mts-to-noteexpression.h"
#include "midi2-sawsynth/midi2-sawsynth.h"

namespace sst::conduit::pluginentry
{

uint32_t clap_get_plugin_count(const clap_plugin_factory *f) { return 7; }
const clap_plugin_descriptor *clap_get_plugin_descriptor(const clap_plugin_factory *f, uint32_t w)
{
    CNDOUT << CNDVAR(w) << std::endl;
    if (w == 0)
        return polysynth::ConduitPolysynthConfig::getDescription();
    if (w == 1)
        return polymetric_delay::ConduitPolymetricDelayConfig::getDescription();
    if (w == 2)
        return chord_memory::ConduitChordMemoryConfig::getDescription();
    if (w == 3)
        return ring_modulator::ConduitRingModulatorConfig::getDescription();
    if (w == 4)
        return clap_event_monitor::ConduitClapEventMonitorConfig::getDescription();
    if (w == 5)
        return mts_to_noteexpression::ConduitMTSToNoteExpressionConfig::getDescription();
    if (w == 6)
        return midi2_sawsynth::ConduitMIDI2SawSynthConfig::getDescription();

    CNDOUT << "Clap Plugin not found at " << w << std::endl;
    return nullptr;
}

static const clap_plugin *clap_create_plugin(const clap_plugin_factory *f, const clap_host *host,
                                             const char *plugin_id)
{
    if (strcmp(plugin_id, polysynth::ConduitPolysynthConfig::getDescription()->id) == 0)
    {
        auto p = new polysynth::ConduitPolysynth(host);
        return p->clapPlugin();
    }
    if (strcmp(plugin_id, polymetric_delay::ConduitPolymetricDelayConfig::getDescription()->id) ==
        0)
    {
        auto p = new polymetric_delay::ConduitPolymetricDelay(host);
        return p->clapPlugin();
    }
    if (strcmp(plugin_id, chord_memory::ConduitChordMemoryConfig::getDescription()->id) == 0)
    {
        auto p = new chord_memory::ConduitChordMemory(host);
        return p->clapPlugin();
    }
    if (strcmp(plugin_id, ring_modulator::ConduitRingModulatorConfig::getDescription()->id) == 0)
    {
        auto p = new ring_modulator::ConduitRingModulator(host);
        return p->clapPlugin();
    }
    if (strcmp(plugin_id,
               clap_event_monitor::ConduitClapEventMonitorConfig::getDescription()->id) == 0)
    {
        auto p = new clap_event_monitor::ConduitClapEventMonitor(host);
        return p->clapPlugin();
    }

    if (strcmp(plugin_id,
               mts_to_noteexpression::ConduitMTSToNoteExpressionConfig::getDescription()->id) == 0)
    {
        auto p = new mts_to_noteexpression::ConduitMTSToNoteExpression(host);
        return p->clapPlugin();
    }

    if (strcmp(plugin_id, midi2_sawsynth::ConduitMIDI2SawSynthConfig::getDescription()->id) == 0)
    {
        auto p = new midi2_sawsynth::ConduitMIDI2SawSynth(host);
        return p->clapPlugin();
    }
    CNDOUT << "No plugin found; returning nullptr" << std::endl;
    return nullptr;
}

static bool clap_get_auv2_info(const clap_plugin_factory_as_auv2 *factory, uint32_t index,
                               clap_plugin_info_as_auv2_t *info)
{
    auto desc = clap_get_plugin_descriptor(nullptr, index); // we don't use the factory above

    auto plugin_id = desc->id;

    info->au_type[0] = 0; // use the features to determine the type
    if (strcmp(plugin_id, polysynth::ConduitPolysynthConfig::getDescription()->id) == 0)
    {
        strncpy(info->au_subt, "PlyS", 5);
        return true;
    }
    if (strcmp(plugin_id, polymetric_delay::ConduitPolymetricDelayConfig::getDescription()->id) ==
        0)
    {
        strncpy(info->au_subt, "dLay", 5);
        return true;
    }
    if (strcmp(plugin_id, chord_memory::ConduitChordMemoryConfig::getDescription()->id) == 0)
    {
        strncpy(info->au_subt, "crMm", 5);
        return true;
    }
    if (strcmp(plugin_id, ring_modulator::ConduitRingModulatorConfig::getDescription()->id) == 0)
    {
        strncpy(info->au_subt, "rngM", 5);
        return true;
    }
    if (strcmp(plugin_id,
               clap_event_monitor::ConduitClapEventMonitorConfig::getDescription()->id) == 0)
    {
        strncpy(info->au_subt, "clEv", 5);
        return true;
    }
    if (strcmp(plugin_id,
               mts_to_noteexpression::ConduitMTSToNoteExpressionConfig::getDescription()->id) == 0)
    {
        strncpy(info->au_subt, "mtsN", 5);
        return true;
    }

    if (strcmp(plugin_id, midi2_sawsynth::ConduitMIDI2SawSynthConfig::getDescription()->id) == 0)
    {
        strncpy(info->au_subt, "m2sn", 5);
        return true;
    }

    return false;
}

const struct clap_plugin_factory conduit_clap_factory = {
    sst::conduit::pluginentry::clap_get_plugin_count,
    sst::conduit::pluginentry::clap_get_plugin_descriptor,
    sst::conduit::pluginentry::clap_create_plugin,
};

const struct clap_plugin_factory_as_auv2 conduit_auv2_factory = {
    "SSTx",             // manu
    "Surge Synth Team", // manu name
    sst::conduit::pluginentry::clap_get_auv2_info};

static const void *get_factory(const char *factory_id)
{
    if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID))
        return &conduit_clap_factory;

    if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_INFO_AUV2))
        return &conduit_auv2_factory;

    return nullptr;
}

// clap_init and clap_deinit are required to be fast, but we have nothing we need to do here
bool clap_init(const char *p) { return true; }
void clap_deinit() {}

} // namespace sst::conduit::pluginentry

extern "C"
{

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes" // other peoples errors are outside my scope
#endif

    // clang-format off
    const CLAP_EXPORT struct clap_plugin_entry clap_entry = {
        CLAP_VERSION,
        sst::conduit::pluginentry::clap_init,
        sst::conduit::pluginentry::clap_deinit,
        sst::conduit::pluginentry::get_factory
    };
    // clang-format on
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}
