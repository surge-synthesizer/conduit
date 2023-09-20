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

#ifndef CONDUIT_SRC_CHORD_MEMORY_CHORD_MEMORY_H
#define CONDUIT_SRC_CHORD_MEMORY_CHORD_MEMORY_H

#include <clap/helpers/plugin.hh>
#include <atomic>
#include <array>
#include <unordered_map>
#include <memory>

#include <memory>
#include "sst/basic-blocks/params/ParamMetadata.h"
#include "conduit-shared/clap-base-class.h"

namespace sst::conduit::chord_memory
{

extern clap_plugin_descriptor desc;
static constexpr int nParams = 1;

struct ConduitChordMemoryConfig
{
    static constexpr int nParams{sst::conduit::chord_memory::nParams};
    using PatchExtension = sst::conduit::shared::EmptyPatchExtension;
    struct DataCopyForUI
    {
        std::atomic<uint32_t> updateCount{0};
        std::atomic<bool> isProcessing{false};
    };

    static clap_plugin_descriptor *getDescription() { return &desc; }
};

struct ConduitChordMemory
    : sst::conduit::shared::ClapBaseClass<ConduitChordMemory, ConduitChordMemoryConfig>
{
    ConduitChordMemory(const clap_host *host);
    ~ConduitChordMemory();

    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override
    {
        setSampleRate(sampleRate);
        return true;
    }

    enum paramIds : uint32_t
    {
        pmKeyShift = 7241
    };

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
    std::unique_ptr<juce::Component> createEditor() override;
    std::atomic<bool> refreshUIValues{false};

  public:
    float *keyShift;
};
} // namespace sst::conduit::chord_memory

#endif // CONDUIT_POLYMETRIC_DELAY_H
