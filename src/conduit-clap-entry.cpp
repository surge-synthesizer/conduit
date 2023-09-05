/*
 * polysynth
 * https://github.com/surge-synthesizer/clap-saw-demo
 *
 * Copyright 2022 Paul Walker and others as listed in the git history
 *
 * Released under the MIT License. See LICENSE.md for full text.
 */

/*
 * This file provides the `clap_plugin_entry` entry point required in the DLL for all
 * clap plugins. It also provides the basic functions for the resulting factory class
 * which generates the plugin. In a single plugin case, this really is just plumbing
 * through to expose polysynth::desc and create a polysynth plugin instance using
 * the helper classes.
 *
 * For more information on this mechanism, see include/clap/entry.h
 */

#include <iostream>
#include <cmath>
#include <cstring>

#include <clap/plugin.h>
#include <clap/factory/plugin-factory.h>

#include "polysynth/polysynth.h"


namespace sst::conduit::pluginentry
{

uint32_t clap_get_plugin_count(const clap_plugin_factory *f) { return 1; }
const clap_plugin_descriptor *clap_get_plugin_descriptor(const clap_plugin_factory *f, uint32_t w)
{
    return &polysynth::desc;
}

static const clap_plugin *clap_create_plugin(const clap_plugin_factory *f, const clap_host *host,
                                             const char *plugin_id)
{
    if (strcmp(plugin_id, polysynth::desc.id))
    {
        std::cout << "Warning: CLAP asked for plugin_id '" << plugin_id
                  << "' and clap-saw-demo ID is '" << polysynth::desc.id << "'" << std::endl;
        return nullptr;
    }
    // I know it looks like a leak right? but the clap-plugin-helpers basically
    // take ownership and destroy the wrapper when the host destroys the
    // underlying plugin (look at Plugin<h, l>::clapDestroy if you don't believe me!)
    auto p = new polysynth::ConduitPolysynth(host);
    return p->clapPlugin();
}

const struct clap_plugin_factory conduit_polysynth_factory = {
    sst::conduit::pluginentry::clap_get_plugin_count,
    sst::conduit::pluginentry::clap_get_plugin_descriptor,
    sst::conduit::pluginentry::clap_create_plugin,
};
static const void *get_factory(const char *factory_id) { return (!strcmp(factory_id,CLAP_PLUGIN_FACTORY_ID)) ? &conduit_polysynth_factory : nullptr; }

// clap_init and clap_deinit are required to be fast, but we have nothing we need to do here
bool clap_init(const char *p) { return true; }
void clap_deinit() {}

} // namespace sst::conduit_polysynth::pluginentry

extern "C"
{
    // clang-format off
    const CLAP_EXPORT struct clap_plugin_entry clap_entry = {
        CLAP_VERSION,
        sst::conduit::pluginentry::clap_init,
        sst::conduit::pluginentry::clap_deinit,
        sst::conduit::pluginentry::get_factory
    };
    // clang-format on
}
