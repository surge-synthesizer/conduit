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
#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>

#include "sst/jucegui/accessibility/Ignored.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/data/Continuous.h"
#include "conduit-shared/editor-base.h"

#include "libMTSClient.h"

namespace sst::conduit::mts_to_noteexpression::editor
{
namespace jcmp = sst::jucegui::components;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit::mts_to_noteexpression::ConduitMTSToNoteExpression;
using uicomm_t = cps_t::UICommunicationBundle;

struct ConduitMTSToNoteExpressionEditor;

struct ConduitMTSToNoteExpressionEditor : public sst::jucegui::accessibility::IgnoredComponent,
                                          shared::ToolTipMixIn<ConduitMTSToNoteExpressionEditor>
{
    uicomm_t &uic;
    using comms_t =
        sst::conduit::shared::EditorCommunicationsHandler<ConduitMTSToNoteExpression,
                                                          ConduitMTSToNoteExpressionEditor>;
    std::unique_ptr<comms_t> comms;

    ConduitMTSToNoteExpressionEditor(uicomm_t &p) : uic(p)
    {
        comms = std::make_unique<comms_t>(p, *this);

        comms->startProcessing();

        comms->addIdleHandler("poll_events", [w = juce::Component::SafePointer(this)]() {
            if (w)
            {
                w->onIdle();
            }
        });

        statusPanel = std::make_unique<jcmp::NamedPanel>("Status");
        statusPanel->setContentAreaComponent(std::make_unique<InfoComp>(this));
        addAndMakeVisible(*statusPanel);

        controlsPanel = std::make_unique<jcmp::NamedPanel>("Controls");
        addAndMakeVisible(*controlsPanel);

        notesPanel = std::make_unique<jcmp::NamedPanel>("Notes");
        notesPanel->setContentAreaComponent(std::make_unique<NotesComp>(this));
        addAndMakeVisible(*notesPanel);

        setSize(600, 400);
    }

    ~ConduitMTSToNoteExpressionEditor()
    {
        comms->removeIdleHandler("poll_events");
        comms->stopProcessing();
    }

    int lastNoteUpd{-1};
    void onIdle()
    {
        auto cl = uic.dataCopyForUI.mtsClient;
        auto sn = scaleName;
        if (cl && MTS_HasMaster(cl))
        {
            isConnected = true;
            scaleName = MTS_GetScaleName(cl);
        }
        else
        {
            isConnected = false;
            scaleName = "";
        }

        if (lastNoteUpd != uic.dataCopyForUI.noteRemainingUpdate)
        {
            lastNoteUpd = uic.dataCopyForUI.noteRemainingUpdate;
            repaint();
        }
        if (sn != scaleName)
        {
            repaint();
        }
    }

    struct InfoComp : juce::Component
    {
        ConduitMTSToNoteExpressionEditor *editor{nullptr};
        InfoComp(ConduitMTSToNoteExpressionEditor *that) : editor(that) {}

        void paint(juce::Graphics &g)
        {
            auto b = getLocalBounds().withHeight(20).withTrimmedLeft(3);
            auto ln = [&b, &g, this](auto s) {
                g.setColour(juce::Colours::white);
                g.setFont(juce::Font(editor->fixedFace).withHeight(12));
                g.drawText(s, b, juce::Justification::centredLeft);
                b = b.translated(0, 20);
            };
            ln(editor->isConnected ? "MTS Connected" : "No MTS Connection");
            ln("Scale: " + editor->scaleName);

            int ct{0};
            for (auto &c : editor->uic.dataCopyForUI.noteRemaining)
            {
                for (auto &m : c)
                {
                    if (m != 0.f)
                        ct++;
                }
            }
            ln("Active Voices : " + std::to_string(ct));
        }
    };

    struct NotesComp : juce::Component
    {
        ConduitMTSToNoteExpressionEditor *editor{nullptr};
        NotesComp(ConduitMTSToNoteExpressionEditor *that) : editor(that) {}

        void paint(juce::Graphics &g)
        {
            auto b = getLocalBounds().withHeight(20).withTrimmedLeft(3);
            auto ln = [&b, &g, this](auto s) {
                g.setColour(juce::Colours::white);
                g.setFont(juce::Font(editor->fixedFace).withHeight(12));
                g.drawText(s, b, juce::Justification::centredLeft);
                b = b.translated(0, 20);
                if (!getLocalBounds().contains(b))
                {
                    b = getLocalBounds().withHeight(20).withTrimmedLeft(b.getX() + 200);
                }
            };
            int ch{0};
            auto cl = editor->uic.dataCopyForUI.mtsClient;
            for (auto &c : editor->uic.dataCopyForUI.noteRemaining)
            {
                int nt{0};
                for (auto &m : c)
                {
                    if (m != 0.f)
                    {
                        auto fr = MTS_NoteToFrequency(cl, nt, ch);
                        auto m = fmt::format("ch={} k={} freq={:.2f}Hz", ch, nt, fr);
                        ln(m);
                    }
                    nt++;
                }
                ch++;
            }
        }
    };

    void resized() override
    {
        auto th = 90;
        auto sw = 250;
        if (statusPanel)
            statusPanel->setBounds(getLocalBounds().withHeight(th).withWidth(sw));

        if (controlsPanel)
            controlsPanel->setBounds(getLocalBounds().withHeight(th).withTrimmedLeft(sw));
        if (notesPanel)
            notesPanel->setBounds(getLocalBounds().withTrimmedTop(th));
    }
    std::unique_ptr<jcmp::NamedPanel> statusPanel, controlsPanel, notesPanel;
    juce::Typeface::Ptr fixedFace{nullptr};

    std::string scaleName{""};
    bool isConnected{false};
};
} // namespace sst::conduit::mts_to_noteexpression::editor

namespace sst::conduit::mts_to_noteexpression
{
std::unique_ptr<juce::Component> ConduitMTSToNoteExpression::createEditor()
{
    uiComms.refreshUIValues = true;
    auto innards = std::make_unique<
        sst::conduit::mts_to_noteexpression::editor::ConduitMTSToNoteExpressionEditor>(uiComms);
    auto editor =
        std::make_unique<conduit::shared::EditorBase<ConduitMTSToNoteExpression>>(uiComms);
    innards->fixedFace = editor->loadFont("Anonymous_Pro/AnonymousPro-Regular.ttf");
    editor->setContentComponent(std::move(innards));

    return editor;
}

} // namespace sst::conduit::mts_to_noteexpression
