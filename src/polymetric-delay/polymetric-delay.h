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

#ifndef CONDUIT_SRC_POLYMETRIC_DELAY_POLYMETRIC_DELAY_H
#define CONDUIT_SRC_POLYMETRIC_DELAY_POLYMETRIC_DELAY_H

#include <clap/helpers/plugin.hh>
#include <atomic>
#include <array>
#include <unordered_map>
#include <memory>

#include <memory>
#include "sst/basic-blocks/params/ParamMetadata.h"
#include "conduit-shared/clap-base-class.h"

namespace sst::conduit::polymetric_delay
{

extern clap_plugin_descriptor desc;
static constexpr int nParams = 3;

struct ConduitPolymetricDelayConfig
{
    static constexpr int nParams{sst::conduit::polymetric_delay::nParams};
    using PatchExtension = sst::conduit::shared::EmptyPatchExtension;
    struct DataCopyForUI
    {
        std::atomic<uint32_t> updateCount{0};
        std::atomic<bool> isProcessing{false};
    };

    static clap_plugin_descriptor *getDescription() { return &desc; }
};

struct ConduitPolymetricDelay
    : sst::conduit::shared::ClapBaseClass<ConduitPolymetricDelay, ConduitPolymetricDelayConfig>
{
    ConduitPolymetricDelay(const clap_host *host);
    ~ConduitPolymetricDelay();

    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override
    {
        return true;
    }

    enum paramIds : uint32_t
    {
        pmDelayInSamples = 8241,
        pmMixLevel = 81,
        pmFeedbackLevel = 999742
    };

    float delayInSamples{1000};

    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override { return 1; }
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

    struct DataCopyForUI
    {
    };

    typedef std::unordered_map<int, int> PatchPluginExtension;

  protected:
    static constexpr uint32_t bufSize{1 << 16};
    float delayBuffer[2][bufSize];
    uint32_t wp{0};

    std::unique_ptr<juce::Component> createEditor() override;
    std::atomic<bool> refreshUIValues{false};

  public:
    static constexpr uint32_t GUI_DEFAULT_W = 390, GUI_DEFAULT_H = 530;

    float *sampleTime, *mix, *feedback;
};
} // namespace sst::conduit::polymetric_delay

#endif // CONDUIT_POLYMETRIC_DELAY_H
