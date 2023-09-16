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

#include "ring-modulator.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/data/Continuous.h"
#include "conduit-shared/editor-base.h"

namespace sst::conduit::ring_modulator::editor
{
namespace jcmp = sst::jucegui::components;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit::ring_modulator::ConduitRingModulator;
using uicomm_t = cps_t::UICommunicationBundle;

struct ConduitRingModulatorEditor;

struct ControlsPanel : juce::Component
{
    uicomm_t &uic;

    ControlsPanel(uicomm_t &p, ConduitRingModulatorEditor &e);
    ~ControlsPanel() { mix->setSource(nullptr); }

    void resized() override
    {
        auto b = getLocalBounds().reduced(5);
        auto ks = std::min(b.getWidth(), b.getHeight());

        int yp = 0;
        auto bx = b.withHeight(ks).withWidth(ks - 14).reduced(4);
        mix->setBounds(bx);
    }

    std::unique_ptr<jcmp::Knob> mix;
};

struct ConduitRingModulatorEditor : public jcmp::WindowPanel, shared::TooltipSupport
{
    uicomm_t &uic;
    using comms_t = sst::conduit::shared::EditorCommunicationsHandler<ConduitRingModulator,
                                                                      ConduitRingModulatorEditor>;
    std::unique_ptr<comms_t> comms;

    ConduitRingModulatorEditor(uicomm_t &p) : uic(p)
    {
        comms = std::make_unique<comms_t>(p, *this);

        ctrlPanel = std::make_unique<jcmp::NamedPanel>("Controls");
        addAndMakeVisible(*ctrlPanel);

        auto oct = std::make_unique<ControlsPanel>(uic, *this);
        ctrlPanel->setContentAreaComponent(std::move(oct));

        setSize(600, 200);

        comms->startProcessing();
    }

    ~ConduitRingModulatorEditor() { comms->stopProcessing(); }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override { ctrlPanel->setBounds(getLocalBounds()); }

    std::unique_ptr<jcmp::NamedPanel> ctrlPanel;
};

ControlsPanel::ControlsPanel(sst::conduit::ring_modulator::editor::uicomm_t &p,
                             sst::conduit::ring_modulator::editor::ConduitRingModulatorEditor &e)
    : uic(p)
{
    mix = std::make_unique<jcmp::Knob>();
    addAndMakeVisible(*mix);
    e.comms->attachContinuousToParam(mix.get(), cps_t::paramIds::pmMixLevel);
}
} // namespace sst::conduit::ring_modulator::editor

namespace sst::conduit::ring_modulator
{
std::unique_ptr<juce::Component> ConduitRingModulator::createEditor()
{
    uiComms.refreshUIValues = true;
    auto innards =
        std::make_unique<sst::conduit::ring_modulator::editor::ConduitRingModulatorEditor>(uiComms);
    auto editor = std::make_unique<conduit::shared::EditorBase>(desc.name, desc.id);
    editor->setContentComponent(std::move(innards));

    return editor;
}
} // namespace sst::conduit::ring_modulator