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

#ifndef CONDUIT_SRC_MTS_TO_NOTEEXPRESSION_MTS_TO_NOTEEXPRESSION_H
#define CONDUIT_SRC_MTS_TO_NOTEEXPRESSION_MTS_TO_NOTEEXPRESSION_H

#include <clap/helpers/plugin.hh>
#include <atomic>
#include <array>
#include <unordered_map>
#include <memory>

#include <memory>
#include "sst/basic-blocks/params/ParamMetadata.h"
#include "sst/cpputils/ring_buffer.h"

#include "conduit-shared/clap-base-class.h"

struct MTSClient;

namespace sst::conduit::mts_to_noteexpression
{

extern clap_plugin_descriptor desc;
static constexpr int nParams = 2;

struct ConduitMTSToNoteExpressionConfig
{
    static constexpr int nParams{sst::conduit::mts_to_noteexpression::nParams};
    static constexpr bool baseClassProvidesMonoModSupport{true};
    using PatchExtension = sst::conduit::shared::EmptyPatchExtension;
    struct DataCopyForUI
    {
        std::atomic<bool> isProcessing{false};
        std::atomic<int32_t> updateCount{0};
        MTSClient *mtsClient{nullptr};

        std::array<std::array<float, 128>, 16>
            noteRemaining; // -1 means still held, otherwise its the time
        std::atomic<int32_t> noteRemainingUpdate{0};
    };

    static clap_plugin_descriptor *getDescription() { return &desc; }
};

struct ConduitMTSToNoteExpression
    : sst::conduit::shared::ClapBaseClass<ConduitMTSToNoteExpression,
                                          ConduitMTSToNoteExpressionConfig>
{
    ConduitMTSToNoteExpression(const clap_host *host);
    ~ConduitMTSToNoteExpression();

    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override
    {
        setSampleRate(sampleRate);
        secondsPerSample = 1.0 / sampleRate;
        return true;
    }

    enum paramIds : uint32_t
    {
        pmRetuneHeld = 1024,
        pmReleaseTuning = 5027
    };

    bool implementsAudioPorts() const noexcept override { return false; }

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

    MTSClient *mtsClient{nullptr};
    char priorScaleName[CLAP_NAME_SIZE];
    std::array<std::array<float, 128>, 16>
        noteRemaining{}; // -1 means still held, otherwise its the time
    std::array<std::array<double, 128>, 16> sclTuning;

    int lastUIUpdate{0};

    float retuningFor(int key, int channel) const;
    bool tuningActive();
    bool retuneHeldNotes();

    float *postNoteRelease{nullptr};
    float *retunHeld{nullptr};
    double secondsPerSample{0};

  protected:
    std::unique_ptr<juce::Component> createEditor() override;
    std::atomic<bool> refreshUIValues{false};

    uint64_t samplePos{0};
};
} // namespace sst::conduit::mts_to_noteexpression

#endif // CONDUIT_POLYMETRIC_DELAY_H
