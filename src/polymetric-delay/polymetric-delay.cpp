//
// Created by Paul Walker on 9/6/23.
//

#include "polymetric-delay.h"

namespace sst::conduit::polymetric_delay
{
const char *features[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SYNTHESIZER, nullptr};
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
    : sst::conduit::shared::ClapBaseClass<ConduitPolymetricDelay>(&desc, host),
      uiComms(*this)
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
                                    .withLinearScaleFormatting("samples"));
    configureParams();

}

ConduitPolymetricDelay::~ConduitPolymetricDelay()
{

}

bool ConduitPolymetricDelay::audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info *info) const noexcept
{
    static constexpr uint32_t inId{16}, outId{72};
    if (isInput)
    {
        info->id = inId;
        info->in_place_pair = outId;
        strncpy(info->name, "main input", sizeof(info->name));
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->channel_count = 2;
        info->port_type = CLAP_PORT_STEREO;
    }
    if (isInput)
    {
        info->id = outId;
        info->in_place_pair = inId;
        strncpy(info->name, "main output", sizeof(info->name));
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->channel_count = 2;
        info->port_type = CLAP_PORT_STEREO;
    }
    return true;
}

}