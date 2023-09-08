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
        auto bx = b.withHeight(ks).withWidth(ks).reduced(4);
        mix->setBounds(bx);
    }

    std::unique_ptr<jcmp::Knob> mix;
};

struct ConduitRingModulatorEditor : public jcmp::WindowPanel
{
    uicomm_t &uic;
    std::unique_ptr<sst::conduit::shared::EditorCommunicationsHandler<ConduitRingModulator>> comms;

    ConduitRingModulatorEditor(uicomm_t &p) : uic(p)
    {
        comms = std::make_unique<
            sst::conduit::shared::EditorCommunicationsHandler<ConduitRingModulator>>(p);

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