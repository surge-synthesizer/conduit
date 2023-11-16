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

#ifndef CONDUIT_SRC_MULTIOUT_SYNTH_MULTIOUT_SYNTH_H
#define CONDUIT_SRC_MULTIOUT_SYNTH_MULTIOUT_SYNTH_H

#include <clap/helpers/plugin.hh>
#include <atomic>
#include <array>
#include <unordered_map>

#include "sst/basic-blocks/dsp/QuadratureOscillators.h"
#include "sst/basic-blocks/modulators/DAHDEnvelope.h"
#include "sst/cpputils/ring_buffer.h"

#include "conduit-shared/clap-base-class.h"

namespace sst::conduit::multiout_synth
{
static constexpr int nOuts = 4;
static constexpr int nParams = 3 * nOuts;

struct ConduitMultiOutSynthConfig
{
    static constexpr int nParams{sst::conduit::multiout_synth::nParams};
    static constexpr bool baseClassProvidesMonoModSupport{true};
    static constexpr bool usesSpecializedMessages{false};
    using PatchExtension = sst::conduit::shared::EmptyPatchExtension;
    struct DataCopyForUI
    {
        std::atomic<uint32_t> updateCount{0};
        std::atomic<bool> isProcessing{false};
    };

    static const clap_plugin_descriptor *getDescription();
};

struct ConduitMultiOutSynth
    : sst::conduit::shared::ClapBaseClass<ConduitMultiOutSynth, ConduitMultiOutSynthConfig>
{
    static constexpr int blockSize{8};
    ConduitMultiOutSynth(const clap_host *host);
    ~ConduitMultiOutSynth();

    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override
    {
        setSampleRate(sampleRate);
        return true;
    }

    enum paramIds : uint32_t
    {
        pmFreq0 = 8675309,
        pmFreq1,
        pmFreq2,
        pmFreq3,

        pmTime0 = 7421,
        pmTime1,
        pmTime2,
        pmTime3,

        pmMute0 = 8824,
        pmMute1,
        pmMute2,
        pmMute3,

    };

    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override { return isInput ? 0 : nOuts; }
    bool audioPortsInfo(uint32_t index, bool isInput,
                        clap_audio_port_info *info) const noexcept override;

    bool implementsNotePorts() const noexcept override { return true; }
    uint32_t notePortsCount(bool isInput) const noexcept override { return isInput ? 1 : 0; }
    bool notePortsInfo(uint32_t index, bool isInput,
                       clap_note_port_info *info) const noexcept override;

    clap_process_status process(const clap_process *process) noexcept override;

    bool startProcessing() noexcept override
    {
        uiComms.dataCopyForUI.isProcessing = true;
        uiComms.dataCopyForUI.updateCount++;
        return true;
    }
    void stopProcessing() noexcept override
    {
        uiComms.dataCopyForUI.isProcessing = false;
        uiComms.dataCopyForUI.updateCount++;
    }

    typedef std::unordered_map<int, int> PatchPluginExtension;

  protected:
    std::unique_ptr<juce::Component> createEditor() override;
    std::atomic<bool> refreshUIValues{false};

    struct Chan
    {
        Chan(ConduitMultiOutSynth *s) : env(s) {}
        int chan{0};
        float timeSinceTrigger{0};
        sst::basic_blocks::dsp::QuadratureOscillator<float> osc;
        sst::basic_blocks::modulators::DAHDEnvelope<ConduitMultiOutSynth, blockSize> env;
        float *freq;
        float *time;
        float *mute;
    };
    std::array<Chan, nOuts> chans;

  public:
    inline float envelope_rate_linear_nowrap(float f)
    {
        return blockSize * sampleRateInv * pow(2.f, -f);
    }

  public:
    uint64_t samplePos{0};
};
} // namespace sst::conduit::multiout_synth

#endif // CONDUIT_POLYMETRIC_DELAY_H
