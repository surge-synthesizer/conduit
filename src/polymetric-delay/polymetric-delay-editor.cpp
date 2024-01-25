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

#include "polymetric-delay.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include <sstream>
#include <array>

#include "sst/jucegui/accessibility/Ignored.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/SevenSegmentControl.h"
#include "sst/jucegui/components/VUMeter.h"
#include "sst/jucegui/layouts/LabeledGrid.h"
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

            auto bx = getLocalBounds().withHeight(20);
            g.setColour(juce::Colours::white);
            g.setFont(14);
            g.drawText("Tempo", bx, juce::Justification::topLeft);
            bx = bx.translated(0, bx.getHeight());
            g.drawText(fmt::format("{:.1f} bpm", 1.0 * uic.dataCopyForUI.tempo), bx,
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
    ~TapPanel();
    struct Content : juce::Component
    {
        Content()
        {
            layout.addColGapAfter(1);
            layout.addColGapAfter(2);
        }

        ~Content() {}
        void resized() override
        {
            layout.resize(getLocalBounds());

            auto vuSpace = getLocalBounds()
                               .withTrimmedLeft(getWidth() - 20)
                               .translated(-5, 0)
                               .withTrimmedBottom(5);
            vuMeter->setBounds(vuSpace);
        }
        void updateVUMeter(TapPanel *p)
        {
            vuMeter->setLevels(p->uic.dataCopyForUI.tapVu[p->tapIdx][0],
                               p->uic.dataCopyForUI.tapVu[p->tapIdx][1]);
        }
        sst::jucegui::layouts::LabeledGrid<5, 2> layout;
        std::unordered_map<uint32_t, std::unique_ptr<jcmp::ContinuousParamEditor>> knobs;
        std::unordered_map<uint32_t, std::unique_ptr<jcmp::DiscreteParamEditor>> dknobs;
        std::vector<std::unique_ptr<juce::Component>> labels;
        std::unique_ptr<jcmp::VUMeter> vuMeter;
    };

    void updateName();
};

struct OutputPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolymetricDelayEditor &ed;
    int tapIdx;

    OutputPanel(uicomm_t &p, ConduitPolymetricDelayEditor &e);
    struct Content : juce::Component
    {
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

struct ConduitPolymetricDelayEditor : public sst::jucegui::accessibility::IgnoredComponent,
                                      shared::ToolTipMixIn<ConduitPolymetricDelayEditor>
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
        setSize(860, 380);

        comms->startProcessing();
    }

    ~ConduitPolymetricDelayEditor() { comms->stopProcessing(); }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override
    {
        auto tabH = getHeight() / 2, tabW = getWidth() - 100;

        auto tabs = getLocalBounds().withWidth(tabW / 2).withHeight(getHeight() / 2);
        for (int i = 0; i < ConduitPolymetricDelay::nTaps; ++i)
        {
            tapPanels[i]->setBounds(tabs);
            if (i == 0 || i == 2)
            {
                tabs = tabs.translated(tabW / 2, 0);
            }
            else
            {
                tabs = tabs.translated(-tabW / 2, tabH);
            }
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
    e.comms->attachDiscreteToParam(toggleButton.get(),
                                   ConduitPolymetricDelay::pmTapActive + tapIdx);

    auto content = std::make_unique<Content>();

    auto add7s = [this, &content, &e](auto p, auto x, auto y) {
        auto kb = std::make_unique<jcmp::SevenSegmentControl>();
        content->addAndMakeVisible(*kb);
        auto gc = content->layout.addComponent(*kb, x, y);
        gc->reduction = 4;
        gc->yPush = 4;
        gc->xPush = (x == 0 ? -2 : 4);

        e.comms->attachDiscreteToParam(kb.get(), p + tapIdx);
        content->dknobs[p] = std::move(kb);

        return (jcmp::SevenSegmentControl *)(content->knobs[p].get());
    };
    add7s(ConduitPolymetricDelay::pmDelayTimeNTaps, 0, 0);
    add7s(ConduitPolymetricDelay::pmDelayTimeEveryM, 1, 0);

    auto addKb = [this, &content, &e](auto p, auto x, auto y, const std::string &label) {
        auto kb = std::make_unique<jcmp::Knob>();
        kb->setDrawLabel(false);
        content->addAndMakeVisible(*kb);
        content->layout.addComponent(*kb, x, y);
        e.comms->attachContinuousToParam(kb.get(), p + tapIdx);
        content->knobs[p] = std::move(kb);

        auto lb = content->layout.addLabel(label, x, y);
        content->addAndMakeVisible(*lb);
        content->labels.push_back(std::move(lb));

        return (jcmp::Knob *)(content->knobs[p].get());
    };

    auto mr = addKb(ConduitPolymetricDelay::pmDelayModRate, 0, 1, "Mod Rate");
    mr->pathDrawMode = jucegui::components::Knob::ALWAYS_FROM_MIN;

    addKb(ConduitPolymetricDelay::pmDelayModDepth, 1, 1, "Mod Depth");

    auto kl = addKb(ConduitPolymetricDelay::pmTapLowCut, 2, 0, "Lo Cut");
    kl->pathDrawMode = jucegui::components::Knob::ALWAYS_FROM_MIN;
    auto kh = addKb(ConduitPolymetricDelay::pmTapHighCut, 2, 1, "Hi Cut");
    kh->pathDrawMode = jucegui::components::Knob::ALWAYS_FROM_MAX;

    addKb(ConduitPolymetricDelay::pmTapFeedback, 3, 0, "Feedback");
    addKb(ConduitPolymetricDelay::pmTapCrossFeedback, 3, 1, "CrossFeed");

    addKb(ConduitPolymetricDelay::pmTapLevel, 4, 0, "Level");
    addKb(ConduitPolymetricDelay::pmTapOutputPan, 4, 1, "Pan");

    content->vuMeter = std::make_unique<jcmp::VUMeter>();
    content->addAndMakeVisible(*(content->vuMeter));

    e.comms->addIdleHandler("tapIdle" + std::to_string(tapIdx), [this, c = content.get()]() {
        c->updateVUMeter(this);
        updateName();
    });

    setContentAreaComponent(std::move(content));
}

TapPanel::~TapPanel() { ed.comms->removeIdleHandler("tapIdle" + std::to_string(tapIdx)); }

void TapPanel::updateName()
{
    auto nIt =
        ed.comms->discreteDataTargets.find(ConduitPolymetricDelay::pmDelayTimeNTaps + tapIdx);
    auto mIt =
        ed.comms->discreteDataTargets.find(ConduitPolymetricDelay::pmDelayTimeEveryM + tapIdx);

    if (nIt == ed.comms->discreteDataTargets.end() || mIt == ed.comms->discreteDataTargets.end())
        return;

    auto n = nIt->second.second->getValue();
    auto m = mIt->second.second->getValue();

    auto name = fmt::format("Tap {} : {} taps per {} beats ({:.2f} beat delay)", tapIdx + 1, n, m,
                            1.f * m / n);
    if (name != getName())
        setName(name);
}

OutputPanel::OutputPanel(uicomm_t &p, ConduitPolymetricDelayEditor &e)
    : jcmp::NamedPanel("Output"), uic(p), ed(e)
{
    auto content = std::make_unique<Content>();

    int ki{0};
    for (auto p : {
             ConduitPolymetricDelay::pmDryLevel,
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
    auto editor = std::make_unique<conduit::shared::EditorBase<ConduitPolymetricDelay>>(uiComms);
    editor->setContentComponent(std::move(innards));

    uiComms.refreshUIValues = true;

    return editor;
}
} // namespace sst::conduit::polymetric_delay