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

#ifndef CONDUIT_SRC_CLAP_EVENT_MONITOR_CLAP_EVENT_MONITOR_H
#define CONDUIT_SRC_CLAP_EVENT_MONITOR_CLAP_EVENT_MONITOR_H

#include <clap/helpers/plugin.hh>
#include <atomic>
#include <array>
#include <unordered_map>
#include <memory>

#include <memory>
#include "sst/basic-blocks/params/ParamMetadata.h"
#include "sst/cpputils/ring_buffer.h"

#include "conduit-shared/clap-base-class.h"

namespace sst::conduit::clap_event_monitor
{
static constexpr int nParams = 3;

struct ConduitClapEventMonitorConfig
{
    static constexpr int nParams{sst::conduit::clap_event_monitor::nParams};
    static constexpr bool baseClassProvidesMonoModSupport{true};
    static constexpr bool usesSpecializedMessages{false};
    using PatchExtension = sst::conduit::shared::EmptyPatchExtension;
    struct DataCopyForUI
    {
        std::atomic<uint32_t> updateCount{0};
        std::atomic<bool> isProcessing{false};

        std::atomic<uint64_t> processedSamples{0};
        static constexpr uint32_t maxEventSize{4096}, maxEvents{4096};

        struct evtCopy
        {
            unsigned char data[maxEventSize];
            void assign(const clap_event_header_t *e)
            {
                assert(e->size < maxEventSize);
                memcpy(data, e, std::max(e->size, maxEventSize));
            }

            const clap_event_header_t *view() const
            {
                return reinterpret_cast<const clap_event_header_t *>(data);
            }
        };
        sst::cpputils::SimpleRingBuffer<evtCopy, 4096> eventBuf;

        unsigned char eventData[maxEventSize * maxEvents];
        void writeEventTo(const clap_event_header_t *e)
        {
            evtCopy ec;
            ec.assign(e);
            eventBuf.push(ec);
        }

        // Read happens ui thread. Might be corrupted. Big queue. Copy soon.
        const clap_event_header_t *readEventFrom(int idx)
        {
            return (const clap_event_header_t *)(eventData + maxEventSize * idx);
        }
    };

    static const clap_plugin_descriptor *getDescription();
};

struct ConduitClapEventMonitor
    : sst::conduit::shared::ClapBaseClass<ConduitClapEventMonitor, ConduitClapEventMonitorConfig>
{
    ConduitClapEventMonitor(const clap_host *host);
    ~ConduitClapEventMonitor();

    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override
    {
        setSampleRate(sampleRate);
        return true;
    }

    enum paramIds : uint32_t
    {
        pmStepped = 24842,

        pmAuto = 912,
        pmMod = 2112
    };

    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override { return 1; }
    bool audioPortsInfo(uint32_t index, bool isInput,
                        clap_audio_port_info *info) const noexcept override;

    bool implementsNotePorts() const noexcept override { return true; }
    uint32_t notePortsCount(bool isInput) const noexcept override { return 1; }
    bool notePortsInfo(uint32_t index, bool isInput,
                       clap_note_port_info *info) const noexcept override;

    /*
     * I have an unacceptably crude state dump and restore. If you want to
     * improve it, PRs welcome! But it's just like any other read-and-write-goop
     * from-a-stream api really.
     */

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

    uint64_t samplePos{0};
};
} // namespace sst::conduit::clap_event_monitor

#endif // CONDUIT_POLYMETRIC_DELAY_H
