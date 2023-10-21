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

#include "mts-to-noteexpression.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "version.h"

#include "libMTSClient.h"

namespace sst::conduit::mts_to_noteexpression
{
const clap_plugin_descriptor *ConduitMTSToNoteExpressionConfig::getDescription()
{
    static const char *features[] = {CLAP_PLUGIN_FEATURE_NOTE_EFFECT, CLAP_PLUGIN_FEATURE_DELAY,
                                     nullptr};
    static clap_plugin_descriptor desc = {CLAP_VERSION,
                                          "org.surge-synth-team.conduit.mts-to-noteexpression",
                                          "Conduit MTS to Note Expression Adapter",
                                          "Surge Synth Team",
                                          "https://surge-synth-team.org",
                                          "",
                                          "",
                                          sst::conduit::build::FullVersionStr,
                                          "The Conduit ClapEventMonitor is a work in progress",
                                          &features[0]};
    return &desc;
}

ConduitMTSToNoteExpression::ConduitMTSToNoteExpression(const clap_host *host)
    : sst::conduit::shared::ClapBaseClass<ConduitMTSToNoteExpression,
                                          ConduitMTSToNoteExpressionConfig>(host)
{
    mtsClient = MTS_RegisterClient();

    auto autoFlag = CLAP_PARAM_IS_AUTOMATABLE;
    auto steppedFlag = autoFlag | CLAP_PARAM_IS_STEPPED;

    paramDescriptions.push_back(ParamDesc()
                                    .asBool()
                                    .withID(pmRetuneHeld)
                                    .withName("Retune Held Notes")
                                    .withGroupName("MTS NE")
                                    .withDefault(1)
                                    .withFlags(steppedFlag));

    paramDescriptions.push_back(ParamDesc()
                                    .asFloat()
                                    .withID(pmReleaseTuning)
                                    .withName("Tune Release Time")
                                    .withGroupName("MTS NE")
                                    .withRange(0, 10)
                                    .withDefault(2)
                                    .withLinearScaleFormatting("s")
                                    .withFlags(autoFlag));

    configureParams();
    attachParam(pmReleaseTuning, postNoteRelease);
    attachParam(pmRetuneHeld, retunHeld);

    clapJuceShim = std::make_unique<sst::clap_juce_shim::ClapJuceShim>(this);
    clapJuceShim->setResizable(true);

    uiComms.dataCopyForUI.mtsClient = mtsClient;
}

ConduitMTSToNoteExpression::~ConduitMTSToNoteExpression() {}

bool ConduitMTSToNoteExpression::notePortsInfo(uint32_t index, bool isInput,
                                               clap_note_port_info *info) const noexcept
{
    info->id = isInput ? 178 : 23;
    info->supported_dialects = CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_CLAP;
    info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
    strncpy(info->name, (std::string("Note ") + (isInput ? "In" : "Out")).c_str(),
            CLAP_NAME_SIZE - 1);
    return true;
}

float ConduitMTSToNoteExpression::retuningFor(int key, int channel) const
{
    return MTS_RetuningInSemitones(mtsClient, key, channel);
}

bool ConduitMTSToNoteExpression::tuningActive() { return true; }
bool ConduitMTSToNoteExpression::retuneHeldNotes() { return (*retunHeld > 0.5); }

clap_process_status ConduitMTSToNoteExpression::process(const clap_process *process) noexcept
{
    auto ev = process->in_events;
    auto ov = process->out_events;
    auto sz = ev->size(ev);

    // Generate top-of-block tuning messages for all our notes that are on
    for (int c = 0; c < 16; ++c)
    {
        for (int i = 0; i < 128; ++i)
        {
            if (tuningActive() && noteRemaining[c][i] != 0.f && retuneHeldNotes())
            {
                auto prior = sclTuning[c][i];
                sclTuning[c][i] = retuningFor(i, c);
                if (sclTuning[c][i] != prior)
                {
                    auto q = clap_event_note_expression();
                    q.header.size = sizeof(clap_event_note_expression);
                    q.header.type = (uint16_t)CLAP_EVENT_NOTE_EXPRESSION;
                    q.header.time = 0;
                    q.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                    q.header.flags = 0;
                    q.key = i;
                    q.channel = c;
                    q.port_index = 0;
                    q.expression_id = CLAP_NOTE_EXPRESSION_TUNING;

                    q.value = sclTuning[c][i];

                    ov->try_push(ov, reinterpret_cast<const clap_event_header *>(&q));
                }
            }
        }
    }

    if (lastUIUpdate == 0)
    {
        // if a paint happens through the copy it doesnt matter
        uiComms.dataCopyForUI.noteRemaining = noteRemaining;
        // since this will force a paint anyway on next idle
        uiComms.dataCopyForUI.noteRemainingUpdate++;
    }
    lastUIUpdate = (lastUIUpdate + 1) & 15;

    for (uint32_t i = 0; i < sz; ++i)
    {
        auto evt = ev->get(ev, i);
        switch (evt->type)
        {
        case CLAP_EVENT_PARAM_VALUE:
        {
            auto v = reinterpret_cast<const clap_event_param_value *>(evt);
            updateParamInPatch(v);
            return true;
        }
        break;
        case CLAP_EVENT_MIDI:
        case CLAP_EVENT_MIDI2:
        case CLAP_EVENT_MIDI_SYSEX:
        case CLAP_EVENT_NOTE_CHOKE:
            ov->try_push(ov, evt);
            break;
        case CLAP_EVENT_NOTE_ON:
        {
            auto nevt = reinterpret_cast<const clap_event_note *>(evt);
            assert(nevt->channel >= 0);
            assert(nevt->channel < 16);
            assert(nevt->key >= 0);
            assert(nevt->key < 128);
            noteRemaining[nevt->channel][nevt->key] = -1;

            auto q = clap_event_note_expression();
            q.header.size = sizeof(clap_event_note_expression);
            q.header.type = (uint16_t)CLAP_EVENT_NOTE_EXPRESSION;
            q.header.time = nevt->header.time;
            q.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            q.header.flags = 0;
            q.key = nevt->key;
            q.channel = nevt->channel;
            q.port_index = nevt->port_index;
            q.note_id = nevt->note_id;
            q.expression_id = CLAP_NOTE_EXPRESSION_TUNING;

            if (tuningActive())
            {
                sclTuning[nevt->channel][nevt->key] = retuningFor(nevt->key, nevt->channel);
            }
            q.value = sclTuning[nevt->channel][nevt->key];

            ov->try_push(ov, evt);
            ov->try_push(ov, &(q.header));
        }
        break;
        case CLAP_EVENT_NOTE_OFF:
        {
            auto nevt = reinterpret_cast<const clap_event_note *>(evt);
            assert(nevt->channel >= 0);
            assert(nevt->channel < 16);
            assert(nevt->key >= 0);
            assert(nevt->key < 128);
            noteRemaining[nevt->channel][nevt->key] = *postNoteRelease;
            ov->try_push(ov, evt);
        }
        break;
        case CLAP_EVENT_NOTE_EXPRESSION:
        {
            auto nevt = reinterpret_cast<const clap_event_note_expression *>(evt);

            auto oevt = clap_event_note_expression();
            memcpy(&oevt, evt, nevt->header.size);

            if (nevt->expression_id == CLAP_NOTE_EXPRESSION_TUNING)
            {
                if (tuningActive())
                {
                    sclTuning[nevt->channel][nevt->key] = retuningFor(nevt->key, nevt->channel);
                }
                oevt.value += sclTuning[nevt->channel][nevt->key];
            }

            ov->try_push(ov, &oevt.header);
        }
        break;
        }
    }

    // subtract block size seconds from everyone with remaining time and zero out some
    for (auto &c : noteRemaining)
        for (auto &n : c)
            if (n > 0.f)
            {
                n -= secondsPerSample * process->frames_count;
                if (n < 0)
                    n = 0.f;
            }

    return CLAP_PROCESS_CONTINUE;
}

} // namespace sst::conduit::mts_to_noteexpression
