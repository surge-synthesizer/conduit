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
#include <readerwriterqueue.h>

#include <memory>
#include "sst/basic-blocks/params/ParamMetadata.h"
#include "conduit-shared/clap-base-class.h"

namespace sst::conduit::polymetric_delay
{

extern clap_plugin_descriptor desc;

struct ConduitPolymetricDelay
    : sst::conduit::shared::ClapBaseClass<ConduitPolymetricDelay>
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
    static constexpr int nParams = 11;

    float delayInSamples{1000};

    bool paramsValue(clap_id paramId, double *value) noexcept override
    {
        *value = delayInSamples;
        return true;
    }
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

    clap_process_status process(const clap_process *process) noexcept override
    { return CLAP_PROCESS_CONTINUE; }
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

  protected:

    std::unique_ptr<juce::Component> createEditor() { return nullptr; }
    std::atomic<bool> refreshUIValues{false};

    // This is an API point the editor can call back to request the host to flush
    // bound by a lambda to the editor. For a technical template reason its implemented
    // (trivially) in clap-saw-demo.cpp not demo-editor
    void editorParamsFlush() { return; }

  public:
    static constexpr uint32_t GUI_DEFAULT_W = 390, GUI_DEFAULT_H = 530;

    struct ToUI
    {
        enum MType
        {
            PARAM_VALUE = 0x31,
        } type;

        uint32_t id;  // param-id for PARAM_VALUE, key for noteon/noteoff
        double value; // value or unused
    };

    struct FromUI
    {
        enum MType
        {
            BEGIN_EDIT = 0xF9,
            END_EDIT,
            ADJUST_VALUE
        } type;
        uint32_t id;
        double value;
    };

    struct DataCopyForUI
    {
        std::atomic<uint32_t> updateCount{0};
        std::atomic<bool> isProcessing{false};
    };

    struct UICommunicationBundle
    {
        UICommunicationBundle(const ConduitPolymetricDelay &h) : cp(h) {}
        typedef moodycamel::ReaderWriterQueue<ToUI, 4096> SynthToUI_Queue_t;
        typedef moodycamel::ReaderWriterQueue<FromUI, 4096> UIToSynth_Queue_t;

        SynthToUI_Queue_t toUiQ;
        UIToSynth_Queue_t fromUiQ;
        DataCopyForUI dataCopyForUI;

        // todo make this std optional I guess
        ParamDesc getParameterDescription(paramIds id) const
        {
            // const so [] isn't an option
            auto fp = cp.paramDescriptionMap.find(id);
            if (fp == cp.paramDescriptionMap.end())
            {
                return ParamDesc();
            }
            return fp->second;
        }

      private:
        const ConduitPolymetricDelay &cp;
    } uiComms;
};
}

#endif // CONDUIT_POLYMETRIC_DELAY_H
