/*
 * Conduit - a project highlighting CLAP-first development
 *           and exercising the surge synth team libraries.
 *
 * Copyright 2023-2024 Paul Walker and authors in github
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

#include "clap-event-monitor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>

#include "sst/jucegui/accessibility/Ignored.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/data/Continuous.h"
#include "conduit-shared/editor-base.h"

namespace sst::conduit::clap_event_monitor::editor
{
namespace jcmp = sst::jucegui::components;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit::clap_event_monitor::ConduitClapEventMonitor;
using uicomm_t = cps_t::UICommunicationBundle;

struct ConduitClapEventMonitorEditor;

struct ConduitClapEventMonitorEditor : public sst::jucegui::accessibility::IgnoredComponent,
                                       shared::ToolTipMixIn<ConduitClapEventMonitorEditor>
{
    uicomm_t &uic;
    using comms_t =
        sst::conduit::shared::EditorCommunicationsHandler<ConduitClapEventMonitor,
                                                          ConduitClapEventMonitorEditor>;
    std::unique_ptr<comms_t> comms;

    ConduitClapEventMonitorEditor(uicomm_t &p) : uic(p)
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

        transportPanel = std::make_unique<jcmp::NamedPanel>("Transport");
        addAndMakeVisible(*transportPanel);

        auto ep = std::make_unique<EventPainter>(*this);
        eventPainterWeak = ep.get();
        evtPanel->setContentAreaComponent(std::move(ep));

        auto tp = std::make_unique<TransportPainter>(this);
        transportPanel->setContentAreaComponent(std::move(tp));

        setSize(800, 700);
    }

    ~ConduitClapEventMonitorEditor()
    {
        comms->removeIdleHandler("poll_events");
        comms->stopProcessing();
    }

    ConduitClapEventMonitorConfig::DataCopyForUI::evtCopy transportEvt;

    void pullEvents()
    {
        bool dorp{false};
        while (!uic.dataCopyForUI.eventBuf.empty())
        {
            auto ib = uic.dataCopyForUI.eventBuf.pop();
            if (ib.has_value())
            {
                if (ib->view()->space_id == CLAP_CORE_EVENT_SPACE_ID &&
                    ib->view()->type == CLAP_EVENT_TRANSPORT)
                {
                    transportEvt = *ib;
                    transportPanel->repaint();
                }
                else
                {
                    events.push_front(*ib);
                }
            }
            dorp = true;
        }
        if (dorp)
            eventPainterWeak->lb->updateContent();
    }

    struct TransportPainter : juce::Component
    {
        ConduitClapEventMonitorEditor *editor;

        TransportPainter(ConduitClapEventMonitorEditor *ed) : editor(ed) {}
        const clap_event_transport_t *t()
        {
            auto v = editor->transportEvt.view();
            if (v->space_id == CLAP_CORE_EVENT_SPACE_ID && v->type == CLAP_EVENT_TRANSPORT)
            {
                return reinterpret_cast<const clap_event_transport_t *>(v);
            }
            return nullptr;
        }
        void paint(juce::Graphics &g)
        {
            auto tp = t();
            if (!tp)
                return;

            auto xp{0}, yp{0};
            auto pt = [&xp, &yp, &g, this](const std::string lab, auto val) {
                auto w = getWidth() / 3;
                auto wl = w * 0.6;
                g.drawText(lab, xp, yp, wl, 20, juce::Justification::centredLeft);
                g.drawText(std::to_string(val), xp + wl, yp, w - wl, 20,
                           juce::Justification::centredLeft);
                yp += 20;
                if (yp > getHeight() - 20)
                {
                    yp = 0;
                    xp += w;
                }
            };
            g.setColour(juce::Colours::white);
            g.setFont(editor->fixedFace);

#define A(x) pt(#x, tp->x)
#define ASC(x) pt(#x "/CSF", 1.f * tp->x / CLAP_SECTIME_FACTOR)
#define F(x) pt(#x, (tp->flags & CLAP_TRANSPORT_##x) ? 1 : 0)

            A(song_pos_beats);
            A(song_pos_seconds);
            ASC(song_pos_beats);
            ASC(song_pos_seconds);
            A(tempo);
            A(tempo_inc);
            A(loop_start_beats);
            A(loop_end_beats);
            A(loop_start_seconds);
            A(loop_end_seconds);
            A(bar_start);
            ASC(bar_start);
            A(bar_number);
            A(tsig_num);
            A(tsig_denom);

            F(HAS_TEMPO);
            F(HAS_BEATS_TIMELINE);
            F(HAS_SECONDS_TIMELINE);
            F(HAS_TIME_SIGNATURE);
            F(IS_PLAYING);
            F(IS_RECORDING);
            F(IS_LOOP_ACTIVE);
            F(IS_WITHIN_PRE_ROLL);

#undef A
        }
    };

    struct EventPainter : juce::Component, juce::TableListBoxModel // a bit sloppy but that's OK
    {
        const ConduitClapEventMonitorEditor &editor;

        enum colid
        {
            SPACE = 1,
            TIME,
            TYPE,
            SIZE,
            LONGFORM
        };
        EventPainter(const ConduitClapEventMonitorEditor &e) : editor(e)
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
            case CLAP_EVENT_NOTE_ON:
            case CLAP_EVENT_NOTE_OFF:
            case CLAP_EVENT_NOTE_CHOKE:
            {
                std::ostringstream oss;
                oss << "CLAP_EVENT_NOTE_"
                    << (ev->type == CLAP_EVENT_NOTE_ON
                            ? "ON "
                            : (ev->type == CLAP_EVENT_NOTE_OFF ? "OFF" : "CHOKE"));
                auto nev = reinterpret_cast<const clap_event_note_t *>(ev);
                oss << " port=" << std::setw(2) << nev->port_index;
                oss << " chan=" << std::setw(2) << nev->channel;
                oss << " key=" << std::setw(3) << nev->key;
                oss << " nid=" << std::setw(6) << nev->note_id;
                oss << " vel=" << std::setw(3) << nev->velocity;

                return oss.str();
            }
            break;
            case CLAP_EVENT_NOTE_EXPRESSION:
            {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(3);
                oss << "CLAP_EVENT_NOTE_EXPRESSION_";
                auto nexp = reinterpret_cast<const clap_event_note_expression_t *>(ev);
                switch (nexp->expression_id)
                {
                case CLAP_NOTE_EXPRESSION_BRIGHTNESS:
                    oss << "BRIGHTNESS";
                    break;
                case CLAP_NOTE_EXPRESSION_EXPRESSION:
                    oss << "EXPRESSION";
                    break;
                case CLAP_NOTE_EXPRESSION_PAN:
                    oss << "PAN";
                    break;
                case CLAP_NOTE_EXPRESSION_PRESSURE:
                    oss << "PRESSURE";
                    break;
                case CLAP_NOTE_EXPRESSION_TUNING:
                    oss << "TUNING";
                    break;
                case CLAP_NOTE_EXPRESSION_VIBRATO:
                    oss << "VIBRATO";
                    break;
                case CLAP_NOTE_EXPRESSION_VOLUME:
                    oss << "VOLUME";
                    break;
                default:
                    oss << "UNKNOWN";
                }
                oss << " " << nexp->value;
                return oss.str();
            }
            break;
            case CLAP_EVENT_PARAM_VALUE:
                return "CLAP_EVENT_PARAM_VALUE";
            case CLAP_EVENT_PARAM_MOD:
                return "CLAP_EVENT_PARAM_MOD";
            case CLAP_EVENT_PARAM_GESTURE_BEGIN:
                return "CLAP_EVENT_PARAM_GESTURE_BEGIN";
            case CLAP_EVENT_PARAM_GESTURE_END:
                return "CLAP_EVENT_PARAM_GESTURE_END";
            case CLAP_EVENT_TRANSPORT:
                return "CLAP_EVENT_TRANSPORT";
            case CLAP_EVENT_MIDI:
            {
                std::ostringstream oss;
                auto mev = reinterpret_cast<const clap_event_midi_t *>(ev);

                oss << "CLAP_EVENT_MIDI ";

                oss << std::hex << "0x" << (int)mev->data[0] << " 0x" << (int)mev->data[1] << " 0x"
                    << (int)mev->data[2];
                return oss.str();
            }
            case CLAP_EVENT_MIDI_SYSEX:
                return "CLAP_EVENT_MIDI_SYSEX";
            case CLAP_EVENT_MIDI2:
                return "CLAP_EVENT_MIDI2";
            }

            return "Un-decoded event";
        }

        std::unique_ptr<juce::TableListBox> lb;
    };

    void resized() override
    {
        auto spl = 180;
        if (evtPanel)
            evtPanel->setBounds(getLocalBounds().withTrimmedTop(spl));
        if (transportPanel)
            transportPanel->setBounds(getLocalBounds().withHeight(spl));
    }
    std::unique_ptr<jcmp::NamedPanel> evtPanel, transportPanel;
    std::deque<ConduitClapEventMonitorConfig::DataCopyForUI::evtCopy> events;
    EventPainter *eventPainterWeak{nullptr};
    juce::Typeface::Ptr fixedFace{nullptr};
};
} // namespace sst::conduit::clap_event_monitor::editor

namespace sst::conduit::clap_event_monitor
{
std::unique_ptr<juce::Component> ConduitClapEventMonitor::createEditor()
{
    uiComms.refreshUIValues = true;
    auto innards =
        std::make_unique<sst::conduit::clap_event_monitor::editor::ConduitClapEventMonitorEditor>(
            uiComms);
    auto editor = std::make_unique<conduit::shared::EditorBase<ConduitClapEventMonitor>>(uiComms);
    innards->fixedFace = editor->loadFont("Anonymous_Pro/AnonymousPro-Regular.ttf");
    editor->setContentComponent(std::move(innards));

    return editor;
}

} // namespace sst::conduit::clap_event_monitor
