//
// Created by Paul Walker on 9/6/23.
//

#include "polymetric-delay.h"

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
    : sst::conduit::shared::ClapBaseClass<ConduitPolymetricDelay, ConduitPolymetricDelayConfig>(&desc, host)
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

    memset(delayBuffer, 0, sizeof(delayBuffer));
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

clap_process_status ConduitPolymetricDelay::process(const clap_process *process) noexcept
{
    if (process->audio_outputs_count <= 0)
        return CLAP_PROCESS_SLEEP;
    if (process->audio_inputs_count <= 0)
        return CLAP_PROCESS_SLEEP;

    float **out = process->audio_outputs[0].data32;
    auto ochans = process->audio_outputs->channel_count;

    float ** const in = process->audio_inputs[0].data32;
    auto ichans = process->audio_inputs->channel_count;

    static int foo{0};
    if (foo == 100)
    {
        CNDOUT << CNDVAR(patch.params[0]) << std::endl;
        foo = 0;
    }
    foo++;

    auto chans = std::min(ochans, ichans);
    if (chans < 0)
        return CLAP_PROCESS_SLEEP;

    for (int i=0; i<process->frames_count; ++i)
    {
        for (int c=0; c<chans; ++c)
        {
            delayBuffer[c][wp] = in[c][i];
            auto rp = (wp + bufSize + (int)patch.params[0]) & (bufSize - 1);
            wp = (wp + 1) & (bufSize - 1);
            out[c][i] = in[c][i] * 0.7 + delayBuffer[c][rp] * 0.3;
        }
    }
    return CLAP_PROCESS_CONTINUE;
}
}