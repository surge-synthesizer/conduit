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

#include "chord-memory.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "version.h"

namespace sst::conduit::chord_memory
{
const clap_plugin_descriptor *ConduitChordMemoryConfig::getDescription()
{
    const char *features[] = {CLAP_PLUGIN_FEATURE_NOTE_EFFECT, CLAP_PLUGIN_FEATURE_DELAY, nullptr};
    static clap_plugin_descriptor desc = {CLAP_VERSION,
                                          "org.surge-synth-team.conduit.chord-memory",
                                          "Conduit Chord Memory",
                                          "Surge Synth Team",
                                          "https://surge-synth-team.org",
                                          "",
                                          "",
                                          sst::conduit::build::FullVersionStr,
                                          "The Conduit Chord Memory is a work in progress",
                                          features};
    return &desc;
}

ConduitChordMemory::ConduitChordMemory(const clap_host *host)
    : sst::conduit::shared::ClapBaseClass<ConduitChordMemory, ConduitChordMemoryConfig>(host)
{
    auto autoFlag = CLAP_PARAM_IS_AUTOMATABLE;
    auto steppedFlag = autoFlag | CLAP_PARAM_IS_STEPPED;

    paramDescriptions.push_back(ParamDesc()
                                    .asInt()
                                    .withID(pmKeyShift)
                                    .withName("Key Shift")
                                    .withGroupName("Retune")
                                    .withRange(-24, 24)
                                    .withDefault(7)
                                    .withLinearScaleFormatting("keys")
                                    .withFlags(steppedFlag));

    configureParams();

    attachParam(pmKeyShift, keyShift);

    clapJuceShim = std::make_unique<sst::clap_juce_shim::ClapJuceShim>(this);
    clapJuceShim->setResizable(true);
}

ConduitChordMemory::~ConduitChordMemory() {}

bool ConduitChordMemory::notePortsInfo(uint32_t index, bool isInput,
                                       clap_note_port_info *info) const noexcept
{
    if (isInput)
    {
        info->id = 172;
        info->supported_dialects =
            CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI_MPE;
        info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
        strncpy(info->name, "NoteInput", CLAP_NAME_SIZE - 1);
        return true;
    }
    else
    {
        info->id = 2321;
        info->supported_dialects =
            CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI_MPE;
        info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
        strncpy(info->name, "NoteOutput", CLAP_NAME_SIZE - 1);
        return true;
    }
    return false;
}

clap_process_status ConduitChordMemory::process(const clap_process *process) noexcept
{
    handleEventsFromUIQueue(process->out_events);

    auto ev = process->in_events;
    auto ov = process->out_events;
    auto sz = ev->size(ev);

    if (sz == 0)
        return CLAP_PROCESS_CONTINUE;

    // We know the input list is sorted
    for (auto i = 0U; i < sz; ++i)
    {
        auto evt = ev->get(ev, i);

        if (handleParamBaseEvents(evt))
        {
        }
        else if (evt->space_id == CLAP_CORE_EVENT_SPACE_ID)
        {
            switch (evt->type)
            {
            case CLAP_EVENT_MIDI:
            {
                auto mevt = reinterpret_cast<const clap_event_midi *>(evt);

                auto msg = mevt->data[0] & 0xF0;
                auto chan = mevt->data[0] & 0x0F;

                if (msg == 0x90 || msg == 0x80)
                {
                    handleMIDI1NoteChange(ov, mevt, chan, mevt->data[1], mevt->data[2] / 127.0,
                                          msg == 0x90);
                    /*
                    clap_event_midi mextra;
                    memcpy(&mextra, mevt, sizeof(clap_event_midi));
                    mextra.data[1] += iks;
                    ov->try_push(ov, (const clap_event_header *)(&mextra));
                     */
                }
                else
                {
                    ov->try_push(ov, evt);
                }
            }
            break;

            case CLAP_EVENT_NOTE_ON:
            case CLAP_EVENT_NOTE_OFF:
            {
                auto nevt = reinterpret_cast<const clap_event_note *>(evt);
                handleClapNoteChange(ov, nevt, nevt->channel, nevt->key, nevt->velocity,
                                     evt->type == CLAP_EVENT_NOTE_ON);
            }
            break;

            default:
                ov->try_push(ov, evt);
                break;
            }
        }
        else
        {
            ov->try_push(ov, evt);
        }
    }

    return CLAP_PROCESS_CONTINUE;
}

void ConduitChordMemory::handleMIDI1NoteChange(const clap_output_events *ov,
                                               const clap_event_midi *mevt, int16_t channel,
                                               int16_t key, double vel, bool on)
{
    auto ks = (int)std::round(*keyShift);
    auto nk = std::clamp(key + ks, 0, 127);
    if (updateNoteOnOffData(channel, key, on))
    {
        ov->try_push(ov, (const clap_event_header *)mevt);
    }
    if (updateNoteOnOffData(channel, nk, on))
    {
        clap_event_midi mextra;
        memcpy(&mextra, mevt, sizeof(clap_event_midi));
        mextra.data[1] = nk;
        ov->try_push(ov, (const clap_event_header *)(&mextra));
    }
}

void ConduitChordMemory::handleClapNoteChange(const clap_output_events *ov,
                                              const clap_event_note *nevt, int16_t channel,
                                              int16_t key, double vel, bool on)
{
    auto ks = (int)std::round(*keyShift);
    auto nk = std::clamp(key + ks, 0, 127);
    if (updateNoteOnOffData(nevt->channel, nevt->key, on))
    {
        ov->try_push(ov, (const clap_event_header *)nevt);
    }
    if (updateNoteOnOffData(nevt->channel, nk, on))
    {
        clap_event_note mextra;
        memcpy(&mextra, nevt, sizeof(clap_event_note));
        mextra.key = nk;
        ov->try_push(ov, (const clap_event_header *)(&mextra));
    }
}

bool ConduitChordMemory::updateNoteOnOffData(int16_t channel, int16_t key, bool isOn)
{
    activeNotes[channel][key] += isOn ? 1 : -1;
    auto an = activeNotes[channel][key];
    CNDOUT << "After note " << (isOn ? "on" : "off") << " at " << channel << " " << key
           << " resulting an=" << an << std::endl;
    return isOn ? an == 1 : an == 0;
}

bool ConduitChordMemoryConfig::PatchExtension::toXml(TiXmlElement &el)
{
    TiXmlElement cn("companionNotes");
    for (auto i = 0U; i < companionNotes.size(); ++i)
    {
        TiXmlElement nt("note");
        nt.SetAttribute("n", i);
        nt.SetAttribute("b", companionNotes[i].to_string());
        cn.InsertEndChild(nt);
    }
    el.InsertEndChild(cn);
    return true;
}

bool ConduitChordMemoryConfig::PatchExtension::fromXml(TiXmlElement *) { return true; }

} // namespace sst::conduit::chord_memory
