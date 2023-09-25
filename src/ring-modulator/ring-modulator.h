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

#ifndef CONDUIT_SRC_RING_MODULATOR_RING_MODULATOR_H
#define CONDUIT_SRC_RING_MODULATOR_RING_MODULATOR_H

#include <clap/helpers/plugin.hh>
#include <atomic>
#include <array>
#include <unordered_map>
#include <memory>

#include <memory>
#include "sst/basic-blocks/params/ParamMetadata.h"
#include "sst/basic-blocks/dsp/QuadratureOscillators.h"
#include "sst/filters/HalfRateFilter.h"

#include "conduit-shared/clap-base-class.h"

namespace sst::conduit::ring_modulator
{

extern clap_plugin_descriptor desc;
static constexpr int nParams = 4;

struct ConduitRingModulatorConfig
{
    static constexpr int nParams{sst::conduit::ring_modulator::nParams};
    static constexpr bool baseClassProvidesMonoModSupport{true};
    using PatchExtension = sst::conduit::shared::EmptyPatchExtension;
    struct DataCopyForUI
    {
        std::atomic<uint32_t> updateCount{0};
        std::atomic<bool> isProcessing{false};
    };

    static clap_plugin_descriptor *getDescription() { return &desc; }
};

struct ConduitRingModulator
    : sst::conduit::shared::ClapBaseClass<ConduitRingModulator, ConduitRingModulatorConfig>
{
    ConduitRingModulator(const clap_host *host);
    ~ConduitRingModulator();

    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override
    {
        setSampleRate(sampleRate);
        return true;
    }

    enum paramIds : uint32_t
    {
        pmMixLevel = 842,

        pmSource = 712,
        pmInternalSourceFrequency = 1524,

        pmAlgo = 17
    };

    enum Algos : uint32_t
    {
        algoDigital = 0,
        algoAnalog = 1
    };

    enum Source : uint32_t
    {
        srcInternal = 0,
        srcSidechain = 1
    };

    float delayInSamples{1000};

    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override { return isInput ? 2 : 1; }
    bool audioPortsInfo(uint32_t index, bool isInput,
                        clap_audio_port_info *info) const noexcept override;

    /*
     * I have an unacceptably crude state dump and restore. If you want to
     * improve it, PRs welcome! But it's just like any other read-and-write-goop
     * from-a-stream api really.
     */

    clap_process_status process(const clap_process *process) noexcept override;
    void handleInboundEvent(const clap_event_header_t *evt);

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

  protected:
    bool implementsLatency() const noexcept override { return true; }
    uint32_t latencyGet() const noexcept override { return blockSize; }

  public:
    typedef std::unordered_map<int, int> PatchPluginExtension;

  protected:
    std::unique_ptr<juce::Component> createEditor() override;
    std::atomic<bool> refreshUIValues{false};

    sst::filters::HalfRate::HalfRateFilter hr_up, hr_scup, hr_down;
    sst::basic_blocks::dsp::QuadratureOscillator<float> internalSource;

    static constexpr int blockSize{4}, blockSizeOS{blockSize << 1};
    float inputBuf alignas(16)[2][blockSize];
    float inputOS alignas(16)[2][blockSizeOS];
    float sidechainBuf alignas(16)[2][blockSize];
    float sidechainBufOS alignas(16)[2][blockSizeOS];

    float outBuf[2][blockSize]{};
    float inMixBuf[2][blockSize]{};

    uint32_t pos{0};

    lag_t mix, freq;

    float *algo, *src;
};
} // namespace sst::conduit::ring_modulator

#endif // CONDUIT_POLYMETRIC_DELAY_H
