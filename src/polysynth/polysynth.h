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

#ifndef CONDUIT_SRC_POLYSYNTH_POLYSYNTH_H
#define CONDUIT_SRC_POLYSYNTH_POLYSYNTH_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <optional>
#include "conduit-shared/debug-helpers.h"

/*
 * ConduitPolysynth is the core synthesizer class. It uses the clap-helpers C++ plugin extensions
 * to present the CLAP C API as a C++ object model.
 *
 * The core features here are
 *
 * - Hold the CLAP description static object
 * - Advertise parameters and ports
 * - Provide an event handler which responds to events and returns sound
 * - Do voice management. Which is really not very sophisticated (it's just an array of 64
 *   voice objects and we choose the next free one, and if you ask for a 65th voice, nothing
 *   happens).
 * - Provide the API points to delegate UI creation to a separate editor object,
 *   coded in clap-saw-demo-editor
 *
 * This demo is coded to be relatively familiar and close to programming styles form other
 * formats where the editor and synth collaborate closely; as described in clap-saw-demo-editor
 * this object also holds the two queues the editor and synth use to communicate; and holds the
 * bundle of atomic values to which the editor holds a const &.
 */

#include <atomic>
#include <array>
#include <unordered_map>
#include <memory>
#include <memory>

#include <clap/helpers/plugin.hh>

#include <readerwriterqueue.h>

#include "sst/basic-blocks/params/ParamMetadata.h"

#include "conduit-shared/clap-base-class.h"
#include "saw-voice.h"

namespace sst::conduit::polysynth
{
/*
 * This static (defined in the cpp file) allows us to present a name, feature set,
 * url etc... and is consumed by clap-saw-demo-pluginentry.cpp
 */
extern clap_plugin_descriptor desc;
static constexpr int nParams{10};

struct ConduitPolysynthConfig
{
    static constexpr int nParams{sst::conduit::polysynth::nParams};
    using PatchExtension = sst::conduit::shared::EmptyPatchExtension;
    struct DataCopyForUI
    {
        std::atomic<uint32_t> updateCount{0};
        std::atomic<bool> isProcessing{false};
        std::atomic<int> polyphony{0};
    };

    static clap_plugin_descriptor *getDescription() { return &desc; }
};

struct ConduitPolysynth
    : sst::conduit::shared::ClapBaseClass<ConduitPolysynth, ConduitPolysynthConfig>
{
    static constexpr int max_voices = 64;
    ConduitPolysynth(const clap_host *host);
    ~ConduitPolysynth();

    /*
     * Activate makes sure sampleRate is distributed through
     * the data structures, in this case by stamping the sampleRate
     * onto each pre-allocated voice object.
     */
    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override
    {
        for (auto &v : voices)
            v.sampleRate = sampleRate;
        return true;
    }

    /*
     * Parameter Handling:
     *
     * Each parameter gets a unique ID which is returned by 'paramsInfo' when the plugin
     * is instantiated (or the plugin asks the host to reset). To avoid accidental bugs where
     * I confuse creation index with param IDs, I am using arbitrary numbers for each
     * parameter id.
     *
     * The implementation of paramsInfo contains the setup of these params.
     *
     * The actual synth has a very simple model to update parameter values. It
     * contains a map from these IDs to a double * which the constructor sets up
     * as references to members.
     *
     * One difference from CSD is rather than put all the config in paramsInfo
     * we set it up here as a data structure
     */
    enum paramIds : uint32_t
    {
        pmUnisonCount = 1378,
        pmUnisonSpread = 2391,
        pmOscDetune = 8675309,

        pmAmpAttack = 2874,
        pmAmpRelease = 728,
        pmAmpIsGate = 1942,

        pmPreFilterVCA = 87612,

        pmCutoff = 17,
        pmResonance = 94,
        pmFilterMode = 14255
    };

  public:
    // Convert 0-1 linear into 0-4s exponential
    float scaleTimeParamToSeconds(float param);

    /*
     * Many CLAP plugins will want input and output audio and note ports, although
     * the spec doesn't require this. Here as a simple synth we set up a single s
     * stereo output and a single midi / clap_note input.
     */
    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override { return isInput ? 0 : 1; }
    bool audioPortsInfo(uint32_t index, bool isInput,
                        clap_audio_port_info *info) const noexcept override;

    bool implementsNotePorts() const noexcept override { return true; }
    uint32_t notePortsCount(bool isInput) const noexcept override { return isInput ? 1 : 0; }
    bool notePortsInfo(uint32_t index, bool isInput,
                       clap_note_port_info *info) const noexcept override;

    /*
     * VoiceInfo is an optional (currently draft) extension where you advertise
     * polyphony information. Crucially here though it allows you to advertise that
     * you can support overlapping notes, which in conjunction with CLAP_DIALECT_NOTE
     * and the Bitwig voice stack modulator lets you stack this little puppy!
     */
    bool implementsVoiceInfo() const noexcept override { return true; }
    bool voiceInfoGet(clap_voice_info *info) noexcept override
    {
        info->voice_capacity = max_voices;
        info->voice_count = max_voices;
        info->flags = CLAP_VOICE_INFO_SUPPORTS_OVERLAPPING_NOTES;
        return true;
    }

    /*
     * I have an unacceptably crude state dump and restore. If you want to
     * improve it, PRs welcome! But it's just like any other read-and-write-goop
     * from-a-stream api really.
     */
    /*bool implementsState() const noexcept override { return true; }
    bool stateSave(const clap_ostream *) noexcept override;
    bool stateLoad(const clap_istream *) noexcept override;
     */

    /*
     * process is the meat of the operation. It does obvious things like trigger
     * voices but also handles all the polyphonic modulation and so on. Please see the
     * comments in the cpp file to understand it and the helper functions we have
     * delegated to.
     */
    clap_process_status process(const clap_process *process) noexcept override;
    void handleInboundEvent(const clap_event_header_t *evt);
    void pushParamsToVoices();
    void handleNoteOn(int port_index, int channel, int key, int noteid);
    void handleNoteOff(int port_index, int channel, int key);
    void activateVoice(SawDemoVoice &v, int port_index, int channel, int key, int noteid);

    /*
     * In addition to ::process, the plugin should implement ::paramsFlush. ::paramsFlush will be
     * called when processing isn't active (no audio being generated, etc...) but the host or UI
     * wants to update a value - usually a parameter value. In effect it looks like a version
     * of process with no audio buffers.
     */
    void paramsFlush(const clap_input_events *in, const clap_output_events *out) noexcept override;

    /*
     * start and stop processing are called when you start and stop obviously.
     * We update an atomic bool so our UI can go ahead and draw processing state
     * and also flush param changes when there is no processing queue.
     */
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

    uint32_t getAsVst3SupportedNodeExpressions() override {
        return AS_VST3_NOTE_EXPRESSION_ALL;
    }

  protected:
    std::unique_ptr<juce::Component> createEditor() override;

  private:
    // These items are ONLY read and written on the audio thread, so they
    // are safe to be non-atomic doubles. We keep a map to locate them
    // for parameter updates.
    float *unisonCount, *unisonSpread, *oscDetune, *cutoff, *resonance, *ampAttack, *ampRelease,
        *ampIsGate, *preFilterVCA, *filterMode;

    typedef std::unordered_map<int, int> PatchPluginExtension;

    // "Voice Management" is "randomly pick a voice to kill and put it in stolen voices"
    std::array<SawDemoVoice, max_voices> voices;
    std::vector<std::tuple<int, int, int, int>> terminatedVoices; // that's PCK ID
};
} // namespace sst::conduit::polysynth

#endif
