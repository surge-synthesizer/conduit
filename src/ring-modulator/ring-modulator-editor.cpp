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
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/data/Continuous.h"
#include "conduit-shared/editor-base.h"

namespace sst::conduit::ring_modulator::editor
{
namespace jcmp = sst::jucegui::components;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit::ring_modulator::ConduitRingModulator;
using uicomm_t = cps_t::UICommunicationBundle;

struct ConduitRingModulatorEditor;

struct RMPanel : juce::Component
{
    uicomm_t &uic;

    RMPanel(uicomm_t &p, ConduitRingModulatorEditor &e);

    void resized() override
    {
        auto sz = 110;
        auto bx = getLocalBounds().reduced(13, 0).withWidth(sz).withHeight(sz);
        mix->setBounds(bx);
        auto mlBx = bx.translated(0, sz).withHeight(20);
        mixLabel->setBounds(mlBx);

        mlBx = mlBx.translated(0, 20).withHeight(30);

        model->setBounds(mlBx);
    }

    std::unique_ptr<jcmp::Knob> mix;
    std::unique_ptr<jcmp::MultiSwitch> model;
    std::unique_ptr<jcmp::Label> mixLabel;
};

struct SourcePanel : juce::Component
{
    uicomm_t &uic;

    SourcePanel(uicomm_t &p, ConduitRingModulatorEditor &e);
    ~SourcePanel() {}

    void resized() override
    {
        auto sz = 110;
        auto bx = getLocalBounds().reduced(13, 0).withWidth(sz).withHeight(sz);
        freq->setBounds(bx);
        auto mlBx = bx.translated(0, sz).withHeight(20);
        freqLabel->setBounds(mlBx);

        mlBx = mlBx.translated(0, 20).withHeight(30);
        src->setBounds(mlBx);
    }

    std::unique_ptr<jcmp::Knob> freq;
    std::unique_ptr<jcmp::MultiSwitch> src;
    std::unique_ptr<jcmp::Label> mixLabel, freqLabel;
};

struct ConduitRingModulatorEditor : public jcmp::WindowPanel,
                                    shared::ToolTipMixIn<ConduitRingModulatorEditor>
{
    uicomm_t &uic;
    using comms_t = sst::conduit::shared::EditorCommunicationsHandler<ConduitRingModulator,
                                                                      ConduitRingModulatorEditor>;
    std::unique_ptr<comms_t> comms;

    ConduitRingModulatorEditor(uicomm_t &p) : uic(p)
    {
        comms = std::make_unique<comms_t>(p, *this);

        ctrlPanel = std::make_unique<jcmp::NamedPanel>("Ring Modulation");
        addAndMakeVisible(*ctrlPanel);

        auto oct = std::make_unique<RMPanel>(uic, *this);
        ctrlPanel->setContentAreaComponent(std::move(oct));

        sourcePanel = std::make_unique<jcmp::NamedPanel>("Modulator Source");
        auto soct = std::make_unique<SourcePanel>(uic, *this);
        sourcePanel->setContentAreaComponent(std::move(soct));
        addAndMakeVisible(*sourcePanel);

        setSize(300, 200);

        comms->startProcessing();
    }

    ~ConduitRingModulatorEditor() { comms->stopProcessing(); }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override
    {
        auto cpW = 150;
        ctrlPanel->setBounds(getLocalBounds().withWidth(cpW));
        sourcePanel->setBounds(getLocalBounds().withTrimmedLeft(cpW));
    }

    std::unique_ptr<jcmp::NamedPanel> ctrlPanel, sourcePanel;
};

RMPanel::RMPanel(sst::conduit::ring_modulator::editor::uicomm_t &p,
                 sst::conduit::ring_modulator::editor::ConduitRingModulatorEditor &e)
    : uic(p)
{
    mix = std::make_unique<jcmp::Knob>();
    addAndMakeVisible(*mix);
    e.comms->attachContinuousToParam(mix.get(), cps_t::paramIds::pmMixLevel);

    mixLabel = std::make_unique<jcmp::Label>();
    mixLabel->setText("Mix");
    addAndMakeVisible(*mixLabel);

    model = std::make_unique<jcmp::MultiSwitch>(jcmp::MultiSwitch::HORIZONTAL);

    addAndMakeVisible(*model);
    e.comms->attachDiscreteToParam(model.get(), cps_t::paramIds::pmAlgo);
}

SourcePanel::SourcePanel(sst::conduit::ring_modulator::editor::uicomm_t &p,
                         sst::conduit::ring_modulator::editor::ConduitRingModulatorEditor &e)
    : uic(p)
{
    freq = std::make_unique<jcmp::Knob>();
    freq->pathDrawMode = jucegui::components::Knob::ALWAYS_FROM_MIN;
    addAndMakeVisible(*freq);
    e.comms->attachContinuousToParam(freq.get(), cps_t::paramIds::pmInternalSourceFrequency);

    freqLabel = std::make_unique<jcmp::Label>();
    freqLabel->setText("Frequency");
    addAndMakeVisible(*freqLabel);

    src = std::make_unique<jcmp::MultiSwitch>(jcmp::MultiSwitch::HORIZONTAL);
    addAndMakeVisible(*src);
    e.comms->attachDiscreteToParam(src.get(), cps_t::paramIds::pmSource);
}
} // namespace sst::conduit::ring_modulator::editor

namespace sst::conduit::ring_modulator
{
std::unique_ptr<juce::Component> ConduitRingModulator::createEditor()
{
    uiComms.refreshUIValues = true;
    auto innards =
        std::make_unique<sst::conduit::ring_modulator::editor::ConduitRingModulatorEditor>(uiComms);
    auto editor = std::make_unique<conduit::shared::EditorBase<ConduitRingModulator>>(uiComms);
    editor->setContentComponent(std::move(innards));

    return editor;
}

} // namespace sst::conduit::ring_modulator