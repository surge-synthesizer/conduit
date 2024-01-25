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

#include "multiout-synth.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>

#include "sst/jucegui/accessibility/Ignored.h"
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

namespace sst::conduit::multiout_synth::editor
{
namespace jcmp = sst::jucegui::components;
namespace jcad = sst::jucegui::component_adapters;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit::multiout_synth::ConduitMultiOutSynth;
using uicomm_t = cps_t::UICommunicationBundle;

struct ConduitMultiOutSynthEditor;

struct ConduitMultiOutSynthEditor : public sst::jucegui::accessibility::IgnoredComponent,
                                    shared::ToolTipMixIn<ConduitMultiOutSynthEditor>
{
    uicomm_t &uic;
    using comms_t = sst::conduit::shared::EditorCommunicationsHandler<ConduitMultiOutSynth,
                                                                      ConduitMultiOutSynthEditor>;
    std::unique_ptr<comms_t> comms;

    struct Controls : juce::Component
    {
        ConduitMultiOutSynthEditor &editor;
        Controls(ConduitMultiOutSynthEditor &e) : editor(e) {}
        void resized() override {}
        void paint(juce::Graphics &g) override
        {
            g.setFont(20);
            g.setColour(juce::Colours::white);
            g.drawText("Coming Soon", getLocalBounds(), juce::Justification::centred);
        }
    };

    ConduitMultiOutSynthEditor(uicomm_t &p) : uic(p)
    {
        comms = std::make_unique<comms_t>(p, *this);
        comms->startProcessing();

        ctrlPanel = std::make_unique<jcmp::NamedPanel>("Controls");
        ctrlPanel->setContentAreaComponent(std::make_unique<Controls>(*this));
        addAndMakeVisible(*ctrlPanel);

        setSize(400, 300);
    }

    ~ConduitMultiOutSynthEditor() { comms->stopProcessing(); }

    void resized() override { ctrlPanel->setBounds(getLocalBounds()); }
    std::unique_ptr<jcmp::NamedPanel> evtPanel, ctrlPanel;
    juce::Typeface::Ptr fixedFace{nullptr};
    bool noteOnly{true};
};
} // namespace sst::conduit::multiout_synth::editor

namespace sst::conduit::multiout_synth
{
std::unique_ptr<juce::Component> ConduitMultiOutSynth::createEditor()
{
    uiComms.refreshUIValues = true;
    auto innards =
        std::make_unique<sst::conduit::multiout_synth::editor::ConduitMultiOutSynthEditor>(uiComms);
    auto editor = std::make_unique<conduit::shared::EditorBase<ConduitMultiOutSynth>>(uiComms);
    innards->fixedFace = editor->loadFont("Anonymous_Pro/AnonymousPro-Regular.ttf");
    editor->setContentComponent(std::move(innards));

    return editor;
}

} // namespace sst::conduit::multiout_synth
