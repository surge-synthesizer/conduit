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

#include "polymetric-delay.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <sstream>

#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/data/Continuous.h"
#include "conduit-shared/editor-base.h"

namespace sst::conduit::polymetric_delay::editor
{
namespace jcmp = sst::jucegui::components;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit::polymetric_delay::ConduitPolymetricDelay;
using uicomm_t = cps_t::UICommunicationBundle;

struct ConduitPolymetricDelayEditor;

struct StatusPanel : juce::Component
{
    uicomm_t &uic;
    ConduitPolymetricDelayEditor &ed;

    StatusPanel(uicomm_t &p, ConduitPolymetricDelayEditor &e);
    ~StatusPanel();

    void paint(juce::Graphics &g) override
    {
        {
            std::ostringstream oss;
            oss << "status: bpm=" << uic.dataCopyForUI.tempo
                << " play=" << uic.dataCopyForUI.isPlayingOrRecording
                << " sig=" << uic.dataCopyForUI.tsig_num << "/" << uic.dataCopyForUI.tsig_denom
                << " pos=" << uic.dataCopyForUI.song_pos_beats
                << " bs=" << uic.dataCopyForUI.bar_start << " bn=" << uic.dataCopyForUI.bar_number;

            g.setColour(juce::Colours::white);
            g.setFont(14);
            g.drawText(oss.str(), getLocalBounds(), juce::Justification::topLeft);
        }

        {
            std::ostringstream oss;
            oss << "converted"
                << " pos=" << uic.dataCopyForUI.song_pos_beats / CLAP_BEATTIME_FACTOR
                << " bs=" << uic.dataCopyForUI.bar_start / CLAP_BEATTIME_FACTOR
                << " bn=" << uic.dataCopyForUI.bar_number;

            g.setColour(juce::Colours::white);
            g.setFont(14);
            g.drawText(oss.str(), getLocalBounds().withTrimmedTop(30),
                       juce::Justification::topLeft);
        }
    }
};

struct ControlsPanel : juce::Component
{
    uicomm_t &uic;

    ControlsPanel(uicomm_t &p, ConduitPolymetricDelayEditor &e);
    ~ControlsPanel()
    {
        time->setSource(nullptr);
        feedback->setSource(nullptr);
        mix->setSource(nullptr);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        auto ks = std::min(b.getWidth() / 3, b.getHeight());

        auto dw = 18;
        auto bx = b.withHeight(ks).withWidth(ks - dw).translated(dw / 2, 0);
        time->setBounds(bx);
        bx = bx.translated(ks, 0);
        feedback->setBounds(bx);
        bx = bx.translated(ks, 0);
        mix->setBounds(bx);
    }

    std::unique_ptr<jcmp::Knob> time, feedback, mix;
};

struct ConduitPolymetricDelayEditor : public jcmp::WindowPanel
{
    uicomm_t &uic;
    std::unique_ptr<sst::conduit::shared::EditorCommunicationsHandler<ConduitPolymetricDelay>>
        comms;

    ConduitPolymetricDelayEditor(uicomm_t &p) : uic(p)
    {
        comms = std::make_unique<
            sst::conduit::shared::EditorCommunicationsHandler<ConduitPolymetricDelay>>(p);

        ctrlPanel = std::make_unique<jcmp::NamedPanel>("Controls");
        addAndMakeVisible(*ctrlPanel);
        auto oct = std::make_unique<ControlsPanel>(uic, *this);
        ctrlPanel->setContentAreaComponent(std::move(oct));

        statusPanel = std::make_unique<jcmp::NamedPanel>("Status");
        addAndMakeVisible(*statusPanel);
        auto spi = std::make_unique<StatusPanel>(uic, *this);
        statusPanel->setContentAreaComponent(std::move(spi));

        setSize(600, 200);

        comms->startProcessing();
    }

    ~ConduitPolymetricDelayEditor() { comms->stopProcessing(); }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override
    {
        auto b = getLocalBounds();
        auto sb = b.withHeight(100);
        auto cb = b.withTrimmedTop(100);
        statusPanel->setBounds(sb);
        ctrlPanel->setBounds(cb);
    }

    std::unique_ptr<jcmp::NamedPanel> ctrlPanel, statusPanel;
};

ControlsPanel::ControlsPanel(uicomm_t &p, ConduitPolymetricDelayEditor &e) : uic(p)
{
    time = std::make_unique<jcmp::Knob>();
    addAndMakeVisible(*time);
    e.comms->attachContinuousToParam(time.get(), cps_t::paramIds::pmDelayInSamples);

    feedback = std::make_unique<jcmp::Knob>();
    addAndMakeVisible(*feedback);
    e.comms->attachContinuousToParam(feedback.get(), cps_t::paramIds::pmFeedbackLevel);

    mix = std::make_unique<jcmp::Knob>();
    addAndMakeVisible(*mix);
    e.comms->attachContinuousToParam(mix.get(), cps_t::paramIds::pmMixLevel);
}

StatusPanel::StatusPanel(uicomm_t &p, ConduitPolymetricDelayEditor &e) : uic(p), ed(e)
{
    ed.comms->addIdleHandler("repaintStatus", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->repaint();
    });
}
StatusPanel::~StatusPanel() { ed.comms->removeIdleHandler("repaintStatus"); }

} // namespace sst::conduit::polymetric_delay::editor

namespace sst::conduit::polymetric_delay
{
std::unique_ptr<juce::Component> ConduitPolymetricDelay::createEditor()
{
    uiComms.refreshUIValues = true;
    auto innards =
        std::make_unique<sst::conduit::polymetric_delay::editor::ConduitPolymetricDelayEditor>(
            uiComms);
    auto editor = std::make_unique<conduit::shared::EditorBase>(desc.name, desc.id);
    editor->setContentComponent(std::move(innards));

    return editor;
}
} // namespace sst::conduit::polymetric_delay