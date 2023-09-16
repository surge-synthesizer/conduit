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

#include "polymetric-delay.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <sstream>
#include <array>

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

struct TapPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolymetricDelayEditor &ed;
    int tapIdx;

    TapPanel(uicomm_t &p, ConduitPolymetricDelayEditor &e, int i);
    struct Content : juce::Component
    {
        ~Content()
        {
            for (auto &k : knobs)
                if (k)
                    k->setSource(nullptr);
        }
        void resized() override
        {
            auto bx = getLocalBounds().reduced(2).withWidth(getHeight() - 17);
            for (auto &k : knobs)
            {
                if (k)
                {
                    k->setBounds(bx);
                    bx = bx.translated(bx.getWidth() + 4, 0);
                }
            }
        }
        std::array<std::unique_ptr<jcmp::Knob>, 8> knobs;
    };
};

struct OutputPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolymetricDelayEditor &ed;
    int tapIdx;

    OutputPanel(uicomm_t &p, ConduitPolymetricDelayEditor &e);
    struct Content : juce::Component
    {
        ~Content()
        {
            for (auto &k : knobs)
                if (k)
                    k->setSource(nullptr);
        }
        void resized() override
        {
            auto bx = getLocalBounds().reduced(2).withHeight(getWidth() / 2);
            bx = bx.withWidth(bx.getHeight()).withHeight(bx.getHeight() + 15);
            for (auto &k : knobs)
            {
                if (k)
                {
                    k->setBounds(bx);
                    bx = bx.translated(0, bx.getHeight());
                }
            }
        }
        std::array<std::unique_ptr<jcmp::Knob>, 2> knobs;
    };
};

struct ConduitPolymetricDelayEditor : public jcmp::WindowPanel, shared::TooltipSupport
{
    uicomm_t &uic;
    using comms_t = sst::conduit::shared::EditorCommunicationsHandler<ConduitPolymetricDelay,
                                                                      ConduitPolymetricDelayEditor>;
    std::unique_ptr<comms_t> comms;

    ConduitPolymetricDelayEditor(uicomm_t &p) : uic(p)
    {
        comms = std::make_unique<comms_t>(p, *this);

        statusPanel = std::make_unique<jcmp::NamedPanel>("Status");
        addAndMakeVisible(*statusPanel);
        auto spi = std::make_unique<StatusPanel>(uic, *this);
        statusPanel->setContentAreaComponent(std::move(spi));

        outputPanel = std::make_unique<OutputPanel>(uic, *this);
        addAndMakeVisible(*outputPanel);

        for (int i = 0; i < ConduitPolymetricDelay::nTaps; ++i)
        {
            tapPanels[i] = std::make_unique<TapPanel>(uic, *this, i);
            addAndMakeVisible(*tapPanels[i]);
        }
        setSize(640, 400);

        comms->startProcessing();
    }

    ~ConduitPolymetricDelayEditor() { comms->stopProcessing(); }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override
    {
        auto tabH = 100, tabW = 520;

        auto b = getLocalBounds();
        // statusPanel->setBounds(sb);

        auto tabs = getLocalBounds().withWidth(tabW).withHeight(tabH);
        for (int i = 0; i < ConduitPolymetricDelay::nTaps; ++i)
        {
            tapPanels[i]->setBounds(tabs);
            tabs = tabs.translated(0, tabH);
        }

        auto sb = getLocalBounds().withTrimmedLeft(tabW).withHeight(getHeight() / 2);
        statusPanel->setBounds(sb);
        outputPanel->setBounds(sb.translated(0, getHeight() / 2));
    }

    std::array<std::unique_ptr<TapPanel>, ConduitPolymetricDelay::nTaps> tapPanels;

    std::unique_ptr<jcmp::NamedPanel> statusPanel, outputPanel;
};

StatusPanel::StatusPanel(uicomm_t &p, ConduitPolymetricDelayEditor &e) : uic(p), ed(e)
{
    ed.comms->addIdleHandler("repaintStatus", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->repaint();
    });
}
StatusPanel::~StatusPanel() { ed.comms->removeIdleHandler("repaintStatus"); }

TapPanel::TapPanel(uicomm_t &p, ConduitPolymetricDelayEditor &e, int i)
    : jcmp::NamedPanel("Tap " + std::to_string(i + 1)), uic(p), ed(e), tapIdx(i)
{
    setTogglable(true);
    auto content = std::make_unique<Content>();

    int ki{0};
    for (auto p :
         {ConduitPolymetricDelay::pmDelayTimeNTaps, ConduitPolymetricDelay::pmDelayTimeEveryM,
          ConduitPolymetricDelay::pmDelayTimeFineSeconds, ConduitPolymetricDelay::pmDelayModRate,
          ConduitPolymetricDelay::pmDelayModDepth, ConduitPolymetricDelay::pmTapLowCut,
          ConduitPolymetricDelay::pmTapHighCut, ConduitPolymetricDelay::pmTapLevel})
    {
        auto kb = std::make_unique<jcmp::Knob>();
        content->addAndMakeVisible(*kb);
        e.comms->attachContinuousToParam(kb.get(), p + tapIdx);
        content->knobs[ki] = std::move(kb);
        ki++;
    }

    setContentAreaComponent(std::move(content));
}

OutputPanel::OutputPanel(uicomm_t &p, ConduitPolymetricDelayEditor &e)
    : jcmp::NamedPanel("Output"), uic(p), ed(e)
{
    auto content = std::make_unique<Content>();

    int ki{0};
    for (auto p : {
             ConduitPolymetricDelay::pmMixLevel,
             ConduitPolymetricDelay::pmFeedbackLevel,
         })
    {
        auto kb = std::make_unique<jcmp::Knob>();
        content->addAndMakeVisible(*kb);
        e.comms->attachContinuousToParam(kb.get(), p);
        content->knobs[ki] = std::move(kb);
        ki++;
    }

    setContentAreaComponent(std::move(content));
}

} // namespace sst::conduit::polymetric_delay::editor

namespace sst::conduit::polymetric_delay
{
std::unique_ptr<juce::Component> ConduitPolymetricDelay::createEditor()
{
    auto innards =
        std::make_unique<sst::conduit::polymetric_delay::editor::ConduitPolymetricDelayEditor>(
            uiComms);
    auto editor = std::make_unique<conduit::shared::EditorBase>(desc.name, desc.id);
    editor->setContentComponent(std::move(innards));

    uiComms.refreshUIValues = true;

    return editor;
}
} // namespace sst::conduit::polymetric_delay