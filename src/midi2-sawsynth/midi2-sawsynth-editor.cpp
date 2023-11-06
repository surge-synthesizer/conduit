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

#include "midi2-sawsynth.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>

#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/TextPushButton.h"
#include "sst/jucegui/component-adapters/DiscreteToReference.h"

#include "sst/jucegui/data/Continuous.h"
#include "conduit-shared/editor-base.h"

#include <midi/midi2_channel_voice_message.h>
#include <midi/channel_voice_message.h>
#include <midi/universal_packet.h>

namespace sst::conduit::midi2_sawsynth::editor
{
namespace jcmp = sst::jucegui::components;
namespace jcad = sst::jucegui::component_adapters;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit::midi2_sawsynth::ConduitMIDI2SawSynth;
using uicomm_t = cps_t::UICommunicationBundle;

struct ConduitMIDI2SawSynthEditor;

struct ConduitMIDI2SawSynthEditor : public jcmp::WindowPanel,
                                    shared::ToolTipMixIn<ConduitMIDI2SawSynthEditor>
{
    uicomm_t &uic;
    using comms_t = sst::conduit::shared::EditorCommunicationsHandler<ConduitMIDI2SawSynth,
                                                                      ConduitMIDI2SawSynthEditor>;
    std::unique_ptr<comms_t> comms;

    struct Controls : juce::Component
    {
        ConduitMIDI2SawSynthEditor &editor;
        Controls(ConduitMIDI2SawSynthEditor &e) : editor(e)
        {
            noteOnly = std::make_unique<jcad::DiscreteToValueReference<jcmp::ToggleButton, bool>>(
                editor.noteOnly);
            noteOnly->widget->setLabel("Note Only");
            addAndMakeVisible(*noteOnly->widget);

            textB = std::make_unique<jcmp::TextPushButton>();
            textB->setLabel("Clear");
            addAndMakeVisible(*textB);
            textB->setOnCallback([this]() { editor.clearEvents(); });
        }
        void resized() override
        {
            noteOnly->widget->setBounds(0, 0, 200, 20);
            textB->setBounds(0, 22, 200, 20);
        }
        std::unique_ptr<jcad::DiscreteToValueReference<jcmp::ToggleButton, bool>> noteOnly;
        std::unique_ptr<jcmp::TextPushButton> textB;
    };

    ConduitMIDI2SawSynthEditor(uicomm_t &p) : uic(p)
    {
        comms = std::make_unique<comms_t>(p, *this);

        comms->startProcessing();

        comms->addIdleHandler("poll_events", [w = juce::Component::SafePointer(this)]() {
            if (w)
            {
                w->pullEvents();
            }
        });

        evtPanel = std::make_unique<jcmp::NamedPanel>("Events");
        addAndMakeVisible(*evtPanel);

        ctrlPanel = std::make_unique<jcmp::NamedPanel>("Controls");
        ctrlPanel->setContentAreaComponent(std::make_unique<Controls>(*this));
        addAndMakeVisible(*ctrlPanel);

        auto ep = std::make_unique<EventPainter>(*this);
        eventPainterWeak = ep.get();
        evtPanel->setContentAreaComponent(std::move(ep));

        setSize(600, 700);
    }

    ~ConduitMIDI2SawSynthEditor()
    {
        comms->removeIdleHandler("poll_events");
        comms->stopProcessing();
    }

    void pullEvents()
    {
        bool dorp{false};
        while (!uic.dataCopyForUI.eventBuf.empty())
        {
            auto ib = uic.dataCopyForUI.eventBuf.pop();
            if (ib.has_value())
            {
                if (includeEvent(*ib))
                {
                    events.push_front(*ib);
                }
            }
            dorp = true;
        }
        if (dorp)
            eventPainterWeak->lb->updateContent();
    }

    void clearEvents()
    {
        events.clear();
        eventPainterWeak->lb->updateContent();
    }

    bool includeEvent(const ConduitMIDI2SawSynthConfig::DataCopyForUI::evtCopy &ec)
    {
        if (!noteOnly)
            return true;

        auto evt = ec.view();

        if (evt->type != CLAP_EVENT_MIDI2)
            return true;

        auto m2e = reinterpret_cast<const clap_event_midi2 *>(evt);
        auto pk = midi::universal_packet(m2e->data[0], m2e->data[1], m2e->data[2], m2e->data[3]);

        return pk.type() == midi::packet_type::midi2_channel_voice &&
               (midi::is_note_on_message(pk) || midi::is_note_off_message(pk));
    }

    struct EventPainter : juce::Component, juce::TableListBoxModel // a bit sloppy but that's OK
    {
        const ConduitMIDI2SawSynthEditor &editor;

        enum colid
        {
            SPACE = 1,
            TIME,
            TYPE,
            SIZE,
            LONGFORM
        };
        EventPainter(const ConduitMIDI2SawSynthEditor &e) : editor(e)
        {
            lb = std::make_unique<juce::TableListBox>();
            lb->setModel(this);
            lb->getHeader().addColumn("Space", SPACE, 40);
            lb->getHeader().addColumn("Time", TIME, 40);
            lb->getHeader().addColumn("Type", TYPE, 40);
            lb->getHeader().addColumn("Size", SIZE, 40);
            lb->getHeader().addColumn("Information", LONGFORM, 600);

            addAndMakeVisible(*lb);
        }

        void resized() override { lb->setBounds(getLocalBounds()); }

        int getNumRows() override { return editor.events.size(); }

        void paintRowBackground(juce::Graphics &g, int rowNumber, int width, int height,
                                bool rowIsSelected) override
        {
            if (rowNumber % 2 == 0)
                g.fillAll(juce::Colour(0x20, 0x20, 0x30));
        }
        void paintCell(juce::Graphics &g, int rowNumber, int columnId, int width, int height,
                       bool rowIsSelected) override
        {
            std::string txt;
            auto ev = editor.events[rowNumber].view();
            switch (columnId)
            {
            case SPACE:
                txt = std::to_string(ev->space_id);
                break;
            case TIME:
                txt = std::to_string(ev->time);
                break;
            case TYPE:
                txt = std::to_string(ev->type);
                break;
            case SIZE:
                txt = std::to_string(ev->size);
                break;
            case LONGFORM:
                txt = textSummary(ev);
                break;
            }
            g.setFont(juce::Font(editor.fixedFace).withHeight(10));
            g.setColour(juce::Colours::white);
            g.drawText(txt, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }

        std::string textSummary(const clap_event_header_t *ev)
        {
            if (ev->space_id != CLAP_CORE_EVENT_SPACE_ID)
            {
                return "Non-core event";
            }

            switch (ev->type)
            {
            case CLAP_EVENT_MIDI2:
            {
                std::ostringstream oss;
                oss << "CLAP_EVENT_MIDI2 ";
                auto m2e = reinterpret_cast<const clap_event_midi2 *>(ev);
                auto pk =
                    midi::universal_packet(m2e->data[0], m2e->data[1], m2e->data[2], m2e->data[3]);

                if (pk.type() == midi::packet_type::midi2_channel_voice)
                {
                    if (midi::is_note_on_message(pk))
                    {
                        oss << "note on "
                            << " nr=" << (int)midi::get_note_nr(pk)
                            << " pt=" << midi::get_note_pitch(pk).as_float()
                            << " vel=" << midi::get_note_velocity(pk).as_float();

                        for (int i = 0; i < 4; ++i)
                            oss << std::hex << " " << (uint32_t)m2e->data[i] << std::dec;
                    }
                    else if (midi::is_note_off_message(pk))
                    {
                        oss << "note off "
                            << " nr=" << (int)midi::get_note_nr(pk)
                            << " pt=" << midi::get_note_pitch(pk).as_float()
                            << " vel=" << midi::get_note_velocity(pk).as_float();

                        for (int i = 0; i < 4; ++i)
                            oss << std::hex << " " << (uint32_t)m2e->data[i] << std::dec;
                    }
                    else if (midi::is_channel_pressure_message(pk))
                    {
                        oss << "channel pressure "
                            << midi::get_channel_pressure_value(pk).as_float();
                    }
                    else if (midi::is_poly_pressure_message(pk))
                    {
                        oss << "poly pressure " << midi::get_poly_pressure_value(pk).as_float()
                            << std::dec << " n=" << (int)midi::get_note_nr(pk) << " "
                            << " " << std::hex << m2e->data[0] << " " << m2e->data[1] << " "
                            << m2e->data[2] << " " << m2e->data[3] << " ";
                    }
                    else if (midi::is_channel_pitch_bend_message(pk))
                    {
                        oss << "channel bend " << midi::get_channel_pitch_bend_value(pk).as_float()
                            << std::endl;
                    }
                    else if (midi::is_control_change_message(pk))
                    {
                        oss << "control change " << (int)midi::get_controller_nr(pk) << " "
                            << midi::get_controller_value(pk).as_float() << std::endl;
                    }
                    else if (midi::is_midi2_registered_per_note_controller_message(pk))
                    {
                        oss << "registered per note controller n=" << std::dec
                            << (int)midi::get_note_nr(pk) << " ct=" << std::hex
                            << (int)midi::get_midi2_per_note_controller_index(pk)
                            << " val=" << midi::get_controller_value(pk).as_float() << std::endl;
                    }
                    else if (midi::is_midi2_per_note_pitch_bend_message(pk))
                    {
                        oss << "note pitch bend n=" << std::dec << (int)midi::get_note_nr(pk)
                            << " val=" << midi::get_controller_value(pk).as_float() << std::endl;
                    }
                    else if (midi::is_midi2_registered_controller_message(pk))
                    {
                        oss << "registered controller"
                            << " ct=" << std::hex
                            << (int)midi::get_midi2_per_note_controller_index(pk);
                        if (midi::get_midi2_per_note_controller_index(pk) == 0x7)
                        {
                            oss << " pitch bend config=";
                            auto cf = midi::pitch_7_25(m2e->data[1]);
                            oss << cf.as_float() << " cents";
                        }
                    }
                    else
                    {
                        oss << "Unknown " << std::hex << "type=" << (int)pk.type()
                            << " group=" << (int)pk.group() << " stat=" << (int)pk.status() << " "
                            << m2e->data[0] << " " << m2e->data[1] << " " << m2e->data[2] << " "
                            << m2e->data[3] << " ";
                    }
                }
                else
                {
                    oss << "unknown pktype=" << (int)pk.type() << std::endl;
                }
                return oss.str();
            }
            default:
            {
                return "NON-MIDI2 Event. Why dod I get a non-MIDI2 event? That's an error";
            }
            }

            return "Un-decoded event";
        }

        std::unique_ptr<juce::TableListBox> lb;
    };

    void resized() override
    {
        if (evtPanel)
            evtPanel->setBounds(getLocalBounds().withTrimmedTop(120));
        if (ctrlPanel)
            ctrlPanel->setBounds(getLocalBounds().withHeight(120));
    }
    std::unique_ptr<jcmp::NamedPanel> evtPanel, ctrlPanel;
    std::deque<ConduitMIDI2SawSynthConfig::DataCopyForUI::evtCopy> events;
    EventPainter *eventPainterWeak{nullptr};
    juce::Typeface::Ptr fixedFace{nullptr};
    bool noteOnly{true};
};
} // namespace sst::conduit::midi2_sawsynth::editor

namespace sst::conduit::midi2_sawsynth
{
std::unique_ptr<juce::Component> ConduitMIDI2SawSynth::createEditor()
{
    uiComms.refreshUIValues = true;
    auto innards =
        std::make_unique<sst::conduit::midi2_sawsynth::editor::ConduitMIDI2SawSynthEditor>(uiComms);
    auto editor = std::make_unique<conduit::shared::EditorBase<ConduitMIDI2SawSynth>>(uiComms);
    innards->fixedFace = editor->loadFont("Anonymous_Pro/AnonymousPro-Regular.ttf");
    editor->setContentComponent(std::move(innards));

    return editor;
}

} // namespace sst::conduit::midi2_sawsynth
