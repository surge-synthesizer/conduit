//
// Created by Paul Walker on 9/6/23.
//

#ifndef CONDUIT_POLYMETRIC_DELAY_H
#define CONDUIT_POLYMETRIC_DELAY_H


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
static constexpr int nParams = 1;


struct ConduitPolymetricDelayConfig
{
    static constexpr int nParams{sst::conduit::polymetric_delay::nParams};
    using PatchExtension = sst::conduit::shared::EmptyPatchExtension;
    struct DataCopyForUI
    {
        std::atomic<uint32_t> updateCount{0};
        std::atomic<bool> isProcessing{false};
    };
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
        pmDelayInSamples = 8241
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
    //bool implementsState() const noexcept override { return true; }
    //bool stateSave(const clap_ostream *) noexcept override;
    //bool stateLoad(const clap_istream *) noexcept override;

    clap_process_status process(const clap_process *process) noexcept override;
    void paramsFlush(const clap_input_events *in, const clap_output_events *out) noexcept override {}
    void handleInboundEvent(const clap_event_header_t *evt) {}
    void handleEventsFromUIQueue(const clap_output_events_t *) {}


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

    static constexpr uint32_t bufSize{1<<16};
    float delayBuffer[2][bufSize];
    uint32_t wp{0};

    std::unique_ptr<juce::Component> createEditor() { return nullptr; }
    std::atomic<bool> refreshUIValues{false};

    // This is an API point the editor can call back to request the host to flush
    // bound by a lambda to the editor. For a technical template reason its implemented
    // (trivially) in clap-saw-demo.cpp not demo-editor
    void editorParamsFlush() { return; }

  public:
    static constexpr uint32_t GUI_DEFAULT_W = 390, GUI_DEFAULT_H = 530;
};
}

#endif // CONDUIT_POLYMETRIC_DELAY_H
