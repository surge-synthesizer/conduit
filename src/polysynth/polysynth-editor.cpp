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

#include "polysynth.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/ToggleButton.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/components/VSlider.h"
#include "sst/jucegui/components/HSlider.h"
#include "sst/jucegui/components/HSliderFilled.h"
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/components/VUMeter.h"

#include "sst/jucegui/component-adapters/DiscreteToReference.h"
#include "sst/jucegui/data/Continuous.h"
#include "conduit-shared/editor-base.h"
#include <sst/jucegui/layouts/LabeledGrid.h>

namespace sst::conduit::polysynth::editor
{
namespace jcmp = sst::jucegui::components;
namespace jcad = sst::jucegui::component_adapters;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit::polysynth::ConduitPolysynth;
using uicomm_t = cps_t::UICommunicationBundle;

struct ConduitPolysynthEditor;

template <typename editor_t, int lx, int ly> struct GridContentBase : juce::Component
{
    GridContentBase() { layout.setControlCellSize(60, 60); }
    ~GridContentBase() {}
    void resized() override
    {
        layout.resize(getLocalBounds());
        if (additionalResizeHandler)
        {
            additionalResizeHandler(this);
        }
    }
    std::function<void(GridContentBase<editor_t, lx, ly> *)> additionalResizeHandler{nullptr};

    template <typename T>
    T *addContinousT(editor_t &e, clap_id p, int x, int y, const std::string &label)
    {
        auto kb = std::make_unique<T>();
        addAndMakeVisible(*kb);
        this->layout.addComponent(*kb, x, y);
        e.comms->attachContinuousToParam(kb.get(), p);
        knobs[p] = std::move(kb);

        auto lb = this->layout.addLabel(label, x, y);
        addAndMakeVisible(*lb);
        labels.push_back(std::move(lb));

        return (T *)(knobs[p].get());
    }

    template <typename T>
    T *addDiscreteT(editor_t &e, clap_id p, int x, int y, const std::string &label)
    {
        auto kb = std::make_unique<T>();
        addAndMakeVisible(*kb);
        this->layout.addComponent(*kb, x, y);
        e.comms->attachDiscreteToParam(kb.get(), p);
        dknobs[p] = std::move(kb);

        auto lb = this->layout.addLabel(label, x, y);
        if (lb)
        {
            addAndMakeVisible(*lb);
            labels.push_back(std::move(lb));
        }
        return (T *)(dknobs[p].get());
    }

    jcmp::Knob *addKnob(editor_t &e, clap_id p, int x, int y, const std::string &label)
    {
        auto kb = addContinousT<jcmp::Knob>(e, p, x, y, label);
        kb->setDrawLabel(false);
        return kb;
    }

    jcmp::VSlider *addVSlider(editor_t &e, clap_id p, int x, int y, const std::string &label)
    {
        return addContinousT<jcmp::VSlider>(e, p, x, y, label);
    }

    jcmp::MultiSwitch *addMultiSwitch(editor_t &e, clap_id p, int x, int y,
                                      const std::string &label)
    {
        return addDiscreteT<jcmp::MultiSwitch>(e, p, x, y, label);
    }

    sst::jucegui::layouts::LabeledGrid<lx, ly> layout;
    std::unordered_map<uint32_t, std::unique_ptr<jcmp::ContinuousParamEditor>> knobs;
    std::unordered_map<uint32_t, std::unique_ptr<jcmp::DiscreteParamEditor>> dknobs;
    std::vector<std::unique_ptr<juce::Component>> labels;
};

struct SawPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    SawPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct PulsePanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    PulsePanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct SinPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    SinPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct NoisePanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    NoisePanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct AEGPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    AEGPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct FEGPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    FEGPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct LPFPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    LPFPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};
struct SVFPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    SVFPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct WSPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    WSPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct FilterRoutingPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    FilterRoutingPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct LFOPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    LFOPanel(uicomm_t &p, ConduitPolysynthEditor &e, int which);
};

struct ModMatrixPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    ModMatrixPanel(uicomm_t &p, ConduitPolysynthEditor &e);

    struct ModMatrixRow : juce::Component, sst::jucegui::data::Continuous
    {
        int row{-1};
        uicomm_t &uic;
        const ModMatrixPanel &panel;

        int lastDataUpdate{-1};

        ModMatrixRow(int row, const ModMatrixPanel &, uicomm_t &p, ConduitPolysynthEditor &e);
        ~ModMatrixRow();

        void paint(juce::Graphics &g) override
        {
            // g.fillAll(juce::Colour(row * 25, 100 + 100 * (row % 2), 250 - row * 25));
        }
        void resized() override
        {
            static constexpr int bw{80};
            static constexpr int tw{100};
            static constexpr int lw{15};

            auto bx = getLocalBounds().withWidth(bw);
            s1->setBounds(bx.reduced(1));
            bx = bx.translated(bw, 0);

            xLab->setBounds(bx.withWidth(lw));
            bx = bx.translated(lw, 0);

            s2->setBounds(bx.reduced(1));
            bx = bx.translated(bw, 0);

            toLab->setBounds(bx.withWidth(lw));
            bx = bx.translated(lw, 0);

            bx = bx.withWidth(tw);
            tgt->setBounds(bx.reduced(1));
            bx = bx.translated(tw, 0);

            atLab->setBounds(bx.withWidth(lw));
            bx = bx.translated(lw, 0);

            bx = bx.withWidth(getWidth() - bx.getX());
            depth->setBounds(bx.reduced(1));
        }

        std::string getLabel() const override { return "depth"; }
        float getMin() const override { return -1; }
        float getMax() const override { return 1; }
        float getValue() const override { return depthValue; }
        std::string getValueAsStringFor(float f) const override
        {
            return fmt::format("{:.2f}%", f * 100.f);
        }
        void setValueFromGUI(const float &f) override
        {
            depthValue = f;
            resendData();
        }
        void setValueFromModel(const float &f) override {}
        float getDefaultValue() const override { return 0.f; }

        std::unique_ptr<jcmp::MenuButton> s1, s2, tgt;
        std::unique_ptr<jcmp::Label> xLab, toLab, atLab;
        std::unique_ptr<jcmp::HSlider> depth;
        float depthValue{0.f};
        int32_t s1value, s2value, tgtvalue;

        void showSourceMenu(int which);
        void showTargetMenu();
        void resendData();
        void updateFromDataIfNeeded();
        void updateFromDataCopy();
        void updateLabels();
    };

    struct Content : juce::Component
    {
        void resized() override
        {
            auto bx = getLocalBounds().withHeight(getHeight() * 1.f / modRows.size());
            for (auto &b : modRows)
            {
                if (b)
                    b->setBounds(bx);
                bx = bx.translated(0, bx.getHeight());
            }
        }

        std::array<std::unique_ptr<ModMatrixRow>, polysynth::ModMatrixConfig::nModSlots> modRows;
    };

    std::map<std::string, std::map<std::string, int32_t>> sourceMenu;
    std::unordered_map<int32_t, std::string> sourceName;

    std::map<std::string, std::map<std::string, int32_t>> targetMenu;
    std::unordered_map<int32_t, std::string> targetName;
};

struct VoiceOutputPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    VoiceOutputPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct StatusPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    StatusPanel(uicomm_t &p, ConduitPolysynthEditor &e);
    ~StatusPanel();

    void updateStatus();

    struct Content : juce::Component
    {
        StatusPanel *panel{nullptr};
        Content(StatusPanel *p) : panel(p) {}
        void resized()
        {
            panel->mpeButton->widget->setBounds(0, 0, 200, 20);
            panel->voiceCountLabel->setBounds(0, 22, 200, 20);
            panel->vuMeter->setBounds(getWidth() - 30, 0, 30, getHeight());
        }
    };

    void pushMPEStatus()
    {
        auto ms = ConduitPolysynthConfig::SpecializedMessage::MPEConfig();
        ms.active = mpeActive;

        ConduitPolysynth::FromUI val;
        val.type = ConduitPolysynth::FromUI::SPECIALIZED;
        val.id = 0;
        val.specializedMessage.payload = ms;

        uic.fromUiQ.push(val);
    }
    std::unique_ptr<jcad::DiscreteToValueReference<jcmp::ToggleButton, bool>> mpeButton;
    bool mpeActive{false};

    std::unique_ptr<jcmp::VUMeter> vuMeter;
    std::unique_ptr<jcmp::Label> voiceCountLabel;
};

struct ModFXPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    ModFXPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct ReverbPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    ReverbPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct ConduitPolysynthEditor : public jcmp::WindowPanel,
                                shared::ToolTipMixIn<ConduitPolysynthEditor>
{
    uicomm_t &uic;
    using comms_t =
        sst::conduit::shared::EditorCommunicationsHandler<ConduitPolysynth, ConduitPolysynthEditor>;
    std::unique_ptr<comms_t> comms;

    ConduitPolysynthEditor(uicomm_t &p) : uic(p)
    {
        comms = std::make_unique<comms_t>(p, *this);

        sawPanel = std::make_unique<SawPanel>(uic, *this);
        addAndMakeVisible(*sawPanel);

        pulsePanel = std::make_unique<PulsePanel>(uic, *this);
        addAndMakeVisible(*pulsePanel);

        sinPanel = std::make_unique<SinPanel>(uic, *this);
        addAndMakeVisible(*sinPanel);

        noisePanel = std::make_unique<NoisePanel>(uic, *this);
        addAndMakeVisible(*noisePanel);

        aegPanel = std::make_unique<AEGPanel>(uic, *this);
        fegPanel = std::make_unique<FEGPanel>(uic, *this);

        addAndMakeVisible(*aegPanel);
        addAndMakeVisible(*fegPanel);

        lpfPanel = std::make_unique<LPFPanel>(uic, *this);
        svfPanel = std::make_unique<SVFPanel>(uic, *this);
        wsPanel = std::make_unique<WSPanel>(uic, *this);
        routingPanel = std::make_unique<FilterRoutingPanel>(uic, *this);
        addAndMakeVisible(*lpfPanel);
        addAndMakeVisible(*svfPanel);
        addAndMakeVisible(*wsPanel);
        addAndMakeVisible(*routingPanel);

        lfo1Panel = std::make_unique<LFOPanel>(uic, *this, 0);
        lfo2Panel = std::make_unique<LFOPanel>(uic, *this, 1);
        addAndMakeVisible(*lfo1Panel);
        addAndMakeVisible(*lfo2Panel);

        modMatrixPanel = std::make_unique<ModMatrixPanel>(uic, *this);
        addAndMakeVisible(*modMatrixPanel);

        outputPanel = std::make_unique<VoiceOutputPanel>(uic, *this);
        addAndMakeVisible(*outputPanel);

        statusPanel = std::make_unique<StatusPanel>(uic, *this);
        addAndMakeVisible(*statusPanel);

        modFXPanel = std::make_unique<ModFXPanel>(uic, *this);
        reverbPanel = std::make_unique<ReverbPanel>(uic, *this);
        addAndMakeVisible(*modFXPanel);
        addAndMakeVisible(*reverbPanel);

        setSize(958, 550);

        comms->startProcessing();
    }

    ~ConduitPolysynthEditor() { comms->stopProcessing(); }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override
    {
        static constexpr int oscWidth{320}, oscHeight{110};
        sawPanel->setBounds(0, 0, oscWidth, oscHeight);
        pulsePanel->setBounds(0, oscHeight, oscWidth, oscHeight);
        sinPanel->setBounds(0, 2 * oscHeight, oscWidth / 5 * 3, oscHeight);
        noisePanel->setBounds(oscWidth / 5 * 3, 2 * oscHeight, oscWidth / 5 * 2, oscHeight);

        static constexpr int envWidth{187}, envHeight{3 * oscHeight / 2};
        fegPanel->setBounds(oscWidth, 0, envWidth, envHeight);
        aegPanel->setBounds(oscWidth, envHeight, envWidth, envHeight);

        static constexpr int filterWidth{(int)(oscWidth / 5 * 3.5)};
        static constexpr int filterXPos{envWidth + oscWidth};
        lpfPanel->setBounds(filterXPos, 0, filterWidth, oscHeight);
        svfPanel->setBounds(filterXPos + filterWidth, 0, filterWidth, oscHeight);

        static constexpr int wsXPos = filterXPos;
        static constexpr int wsWidth{oscWidth / 5 * 3};
        static constexpr int rtWidth{oscWidth / 5 * 2};
        static constexpr int outWidth = rtWidth;
        wsPanel->setBounds(wsXPos, oscHeight, wsWidth, oscHeight);
        routingPanel->setBounds(wsXPos + wsWidth, oscHeight, rtWidth, oscHeight);
        outputPanel->setBounds(wsXPos + wsWidth + rtWidth, oscHeight, outWidth, oscHeight);

        static constexpr int lfoWidth{(oscWidth + envWidth) / 2};
        lfo1Panel->setBounds(0, 3 * oscHeight, lfoWidth, oscHeight);
        lfo2Panel->setBounds(lfoWidth, 3 * oscHeight, lfoWidth, oscHeight);

        // static constexpr int outputWidth{wsWidth};
        modMatrixPanel->setBounds(oscWidth + envWidth, 2 * oscHeight, rtWidth + wsWidth + outWidth,
                                  2 * oscHeight);

        static constexpr int fxYPos{4 * oscHeight};
        static constexpr int modFXWidth{oscWidth};
        static constexpr int revFXWidth{oscWidth / 5 * 4};
        modFXPanel->setBounds(0, fxYPos, modFXWidth, oscHeight);
        reverbPanel->setBounds(modFXWidth, fxYPos, revFXWidth, oscHeight);
        statusPanel->setBounds(modFXWidth + revFXWidth, fxYPos,
                               modMatrixPanel->getRight() - (modFXWidth + revFXWidth), oscHeight);
    }

    std::unique_ptr<jcmp::NamedPanel> sawPanel, pulsePanel, sinPanel, noisePanel;
    std::unique_ptr<jcmp::NamedPanel> aegPanel, fegPanel;
    std::unique_ptr<jcmp::NamedPanel> lfo1Panel, lfo2Panel;
    std::unique_ptr<jcmp::NamedPanel> lpfPanel, svfPanel, wsPanel, routingPanel;
    std::unique_ptr<jcmp::NamedPanel> modMatrixPanel;
    std::unique_ptr<jcmp::NamedPanel> outputPanel, statusPanel;

    std::unique_ptr<jcmp::NamedPanel> modFXPanel, reverbPanel;
};

SawPanel::SawPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                   sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Saw Osc"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 5, 1>>();

    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmSawActive);

    content->addKnob(e, ConduitPolysynth::pmSawUnisonCount, 0, 0, "Voices");
    content->addKnob(e, ConduitPolysynth::pmSawUnisonSpread, 1, 0, "Detune");
    content->addKnob(e, ConduitPolysynth::pmSawCoarse, 2, 0, "Coarse");
    content->addKnob(e, ConduitPolysynth::pmSawFine, 3, 0, "Fine");
    content->addKnob(e, ConduitPolysynth::pmSawLevel, 4, 0, "Level");

    setContentAreaComponent(std::move(content));
}

PulsePanel::PulsePanel(sst::conduit::polysynth::editor::uicomm_t &p,
                       sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Pulse Width Osc"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 5, 1>>();

    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmPWActive);

    content->addKnob(e, ConduitPolysynth::pmPWWidth, 0, 0, "Width");
    content->addKnob(e, ConduitPolysynth::pmPWFrequencyDiv, 1, 0, "Octave");
    content->addKnob(e, ConduitPolysynth::pmPWCoarse, 2, 0, "Coarse");
    content->addKnob(e, ConduitPolysynth::pmPWFine, 3, 0, "Fine");
    content->addKnob(e, ConduitPolysynth::pmPWLevel, 4, 0, "Level");

    setContentAreaComponent(std::move(content));
}

SinPanel::SinPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                   sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Sin Osc"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 3, 1>>();

    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmSinActive);

    content->addKnob(e, ConduitPolysynth::pmSinFrequencyDiv, 0, 0, "Octave");
    content->addKnob(e, ConduitPolysynth::pmSinCoarse, 1, 0, "Coarse");
    content->addKnob(e, ConduitPolysynth::pmSinLevel, 2, 0, "Level");

    setContentAreaComponent(std::move(content));
}

NoisePanel::NoisePanel(sst::conduit::polysynth::editor::uicomm_t &p,
                       sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Noise OSC"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 2, 1>>();

    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmNoiseActive);

    content->addKnob(e, ConduitPolysynth::pmNoiseColor, 0, 0, "Color");
    content->addKnob(e, ConduitPolysynth::pmNoiseLevel, 1, 0, "Level");

    setContentAreaComponent(std::move(content));
}

AEGPanel::AEGPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                   sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Amplitude EG"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 6, 1>>();
    content->layout.setControlCellSize(27, 120);
    content->layout.addColGapAfter(3);

    content->addVSlider(e, ConduitPolysynth::pmEnvA, 0, 0, "A");
    content->addVSlider(e, ConduitPolysynth::pmEnvD, 1, 0, "D");
    content->addVSlider(e, ConduitPolysynth::pmEnvS, 2, 0, "S");
    content->addVSlider(e, ConduitPolysynth::pmEnvR, 3, 0, "R");
    content->addVSlider(e, ConduitPolysynth::pmAegVelocitySens, 4, 0, "Vel");
    content->addVSlider(e, ConduitPolysynth::pmAegPreFilterGain, 5, 0, "Gain");

    setContentAreaComponent(std::move(content));
}

FEGPanel::FEGPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                   sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Filter EG"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 6, 1>>();
    content->layout.setControlCellSize(27, 120);
    content->layout.addColGapAfter(3);

    content->addVSlider(e, ConduitPolysynth::pmEnvA + ConduitPolysynth::offPmFeg, 0, 0, "A");
    content->addVSlider(e, ConduitPolysynth::pmEnvD + ConduitPolysynth::offPmFeg, 1, 0, "D");
    content->addVSlider(e, ConduitPolysynth::pmEnvS + ConduitPolysynth::offPmFeg, 2, 0, "S");
    content->addVSlider(e, ConduitPolysynth::pmEnvR + ConduitPolysynth::offPmFeg, 3, 0, "R");
    content->addVSlider(e, ConduitPolysynth::pmFegToLPFCutoff, 4, 0, "Adv");
    content->addVSlider(e, ConduitPolysynth::pmFegToSVFCutoff, 5, 0, "Mult");

    setContentAreaComponent(std::move(content));
}

LPFPanel::LPFPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                   sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Advanced Filter"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 4, 1>>();

    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmLPFActive);

    content->addMultiSwitch(e, ConduitPolysynth::pmLPFFilterMode, 0, 0, "");
    content->addKnob(e, ConduitPolysynth::pmLPFCutoff, 1, 0, "Cutoff");
    content->addKnob(e, ConduitPolysynth::pmLPFResonance, 2, 0, "Resonance");

    content->addVSlider(e, ConduitPolysynth::pmLPFKeytrack, 3, 0, "KeyTk");
    // bit of a hack
    content->layout.addColGapAfter(2, -12);

    setContentAreaComponent(std::move(content));
}

SVFPanel::SVFPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                   sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Multi-Mode Filter"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 4, 1>>();

    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmSVFActive);

    content->addMultiSwitch(e, ConduitPolysynth::pmSVFFilterMode, 0, 0, "");
    content->addKnob(e, ConduitPolysynth::pmSVFCutoff, 1, 0, "Cutoff");
    content->addKnob(e, ConduitPolysynth::pmSVFResonance, 2, 0, "Resonance");
    content->addVSlider(e, ConduitPolysynth::pmSVFKeytrack, 3, 0, "KeyTk");
    // bit of a hack
    content->layout.addColGapAfter(2, -12);
    setContentAreaComponent(std::move(content));
}

WSPanel::WSPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                 sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Waveshaper"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 3, 1>>();

    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmWSActive);

    content->addMultiSwitch(e, ConduitPolysynth::pmWSMode, 0, 0, "");
    content->addKnob(e, ConduitPolysynth::pmWSDrive, 1, 0, "Drive");
    content->addKnob(e, ConduitPolysynth::pmWSBias, 2, 0, "Bias");

    setContentAreaComponent(std::move(content));
}

FilterRoutingPanel::FilterRoutingPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                                       sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Filter Routing"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 2, 1>>();

    content->addMultiSwitch(e, ConduitPolysynth::pmFilterRouting, 0, 0, "");
    content->addKnob(e, ConduitPolysynth::pmFilterFeedback, 1, 0, "Feedback");

    setContentAreaComponent(std::move(content));
}

LFOPanel::LFOPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                   sst::conduit::polysynth::editor::ConduitPolysynthEditor &e, int which)
    : jcmp::NamedPanel("LFO " + std::to_string(which + 1)), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 4, 1>>();

    content->addMultiSwitch(e, ConduitPolysynth::pmLFOShape + which * ConduitPolysynth::offPmLFO2,
                            0, 0, "");
    auto rt = content->addKnob(e, ConduitPolysynth::pmLFORate + which * ConduitPolysynth::offPmLFO2,
                               1, 0, "Rate");
    rt->pathDrawMode = jcmp::Knob::ALWAYS_FROM_MIN;

    content->addKnob(e, ConduitPolysynth::pmLFOAmplitude + which * ConduitPolysynth::offPmLFO2, 2,
                     0, "Amp");
    content->addKnob(e, ConduitPolysynth::pmLFODeform + which * ConduitPolysynth::offPmLFO2, 3, 0,
                     "Deform");

    auto ts = std::make_unique<jcmp::ToggleButton>();
    ts->setGlyph(jcmp::GlyphPainter::GlyphType::METRONOME);
    content->addAndMakeVisible(*ts);
    e.comms->attachDiscreteToParam(ts.get(), ConduitPolysynth::pmLFOTempoSync +
                                                 which * ConduitPolysynth::offPmLFO2);
    content->dknobs[ConduitPolysynth::pmLFOTempoSync + which * ConduitPolysynth::offPmLFO2] =
        std::move(ts);

    content->additionalResizeHandler = [which](auto *ct) {
        auto pRt = ConduitPolysynth::pmLFORate + which * ConduitPolysynth::offPmLFO2;
        auto pTs = ConduitPolysynth::pmLFOTempoSync + which * ConduitPolysynth::offPmLFO2;

        const auto &pK = ct->knobs[pRt];
        const auto &pT = ct->dknobs[pTs];

        pT->setBounds(pK->getRight() - 6, pK->getY() - 2, 10, 10);
    };
    setContentAreaComponent(std::move(content));
}

ModMatrixPanel::ModMatrixPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                               sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Mod Matrix"), uic(p), ed(e)
{
    auto content = std::make_unique<Content>();

    auto v = uic.getAllParamDescriptions();
    for (auto &pd : v)
    {
        if (pd.flags & CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID)
        {
            targetMenu[pd.groupName][pd.name] = pd.id;
            targetName[pd.id] = pd.name;
        }
    }
    targetMenu[""]["Off"] = ConduitPolysynth::pmNoModTarget;
    targetName[ConduitPolysynth::pmNoModTarget] = "-";

    auto q = conduit::polysynth::ModMatrixConfig();
    auto s = q.sourceNames;
    for (auto &pd : s)
    {
        sourceMenu[pd.second.second][pd.second.first] = pd.first;
        sourceName[pd.first] = pd.second.first;
    }

#if DEBUG_MATRIX_MENUS
    for (const auto &[g, m] : targetMenu)
    {
        CNDOUT << "T -- " << g << std::endl;
        for (const auto &[n, id] : m)
        {
            CNDOUT << "T     |-- " << n << " (" << id << ")" << std::endl;
        }
    }

    for (const auto &[g, m] : sourceMenu)
    {
        CNDOUT << "S -- " << g << std::endl;
        for (const auto &[n, id] : m)
        {
            CNDOUT << "S     |-- " << n << " (" << id << ")" << std::endl;
        }
    }
#endif

    for (auto i = 0U; i < content->modRows.size(); ++i)
    {
        content->modRows[i] = std::make_unique<ModMatrixRow>(i, *this, p, e);
        content->addAndMakeVisible(*(content->modRows[i]));
    }

    setContentAreaComponent(std::move(content));
}

ModMatrixPanel::ModMatrixRow::ModMatrixRow(
    int row, const ModMatrixPanel &pan, sst::conduit::polysynth::editor::uicomm_t &p,
    sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : row(row), uic(p), panel(pan)
{
    xLab = std::make_unique<jcmp::Label>();
    xLab->setText("x");
    addAndMakeVisible(*xLab);

    toLab = std::make_unique<jcmp::Label>();
    toLab->setText("to");
    addAndMakeVisible(*toLab);

    atLab = std::make_unique<jcmp::Label>();
    atLab->setText("@");
    addAndMakeVisible(*atLab);

    s1 = std::make_unique<jcmp::MenuButton>();
    s1->setLabel("-");
    s1->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showSourceMenu(0);
    });
    addAndMakeVisible(*s1);

    s2 = std::make_unique<jcmp::MenuButton>();
    s2->setLabel("-");
    s2->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showSourceMenu(1);
    });
    addAndMakeVisible(*s2);

    tgt = std::make_unique<jcmp::MenuButton>();
    tgt->setLabel("-");
    tgt->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->showTargetMenu();
    });
    addAndMakeVisible(*tgt);

    depth = std::make_unique<jcmp::HSliderFilled>();
    depth->setSource(this);
    depth->setShowLabel(false);
    addAndMakeVisible(*depth);

    updateFromDataCopy();

    panel.ed.comms->addIdleHandler("row" + std::to_string(row),
                                   [this]() { updateFromDataIfNeeded(); });
}

ModMatrixPanel::ModMatrixRow::~ModMatrixRow()
{
    panel.ed.comms->removeIdleHandler("row" + std::to_string(row));
}

void ModMatrixPanel::ModMatrixRow::resendData()
{
    ConduitPolysynth::FromUI val;
    val.type = ConduitPolysynth::FromUI::SPECIALIZED;
    val.id = 0;
    ConduitPolysynthConfig::SpecializedMessage::ModRowMessage msg;

    msg.row = row;
    msg.s1 = s1value;
    msg.s2 = s2value;
    msg.tgt = tgtvalue;
    msg.depth = depthValue;

    val.specializedMessage.payload = msg;

    uic.fromUiQ.push(val);
}

void ModMatrixPanel::ModMatrixRow::updateFromDataIfNeeded()
{
    int32_t v = uic.dataCopyForUI.rescanMatrix;
    if (v != lastDataUpdate)
    {
        updateFromDataCopy();
        lastDataUpdate = v;
    }
}

void ModMatrixPanel::ModMatrixRow::updateFromDataCopy()
{
    auto &dc = uic.dataCopyForUI.modMatrixCopy[row];
    s1value = std::get<0>(dc);
    s2value = std::get<1>(dc);
    tgtvalue = std::get<2>(dc);
    depthValue = std::get<3>(dc);

    updateLabels();
}

void ModMatrixPanel::ModMatrixRow::updateLabels()
{
    s1->setLabel(panel.sourceName.at(s1value));
    s2->setLabel(panel.sourceName.at(s2value));
    tgt->setLabel(panel.targetName.at(tgtvalue));
    repaint();
}

void ModMatrixPanel::ModMatrixRow::showSourceMenu(int which)
{
    auto p = juce::PopupMenu();
    p.addSectionHeader((which == 0) ? "Set Source" : "Set Source Via");
    p.addSeparator();
    for (const auto &[name, group] : panel.sourceMenu)
    {
        if (name == "")
        {
            for (auto &[t, id] : group)
            {
                p.addItem(t, [setTo = id, which, w = juce::Component::SafePointer(this)]() {
                    if (w)
                    {
                        if (which)
                            w->s2value = setTo;
                        else
                            w->s1value = setTo;
                        w->updateLabels();
                        w->resendData();
                    }
                });
            }
        }
        else
        {
            auto sm = juce::PopupMenu();
            for (auto &[t, id] : group)
            {
                sm.addItem(t, [setTo = id, which, w = juce::Component::SafePointer(this)]() {
                    if (w)
                    {
                        if (which)
                            w->s2value = setTo;
                        else
                            w->s1value = setTo;
                        w->updateLabels();
                        w->resendData();
                    }
                });
            }
            p.addSubMenu(name, sm);
        }
    }
    // p.showMenuAsync(juce::PopupMenu::Options().withParentComponent(this));
    p.showMenuAsync(juce::PopupMenu::Options());
}

void ModMatrixPanel::ModMatrixRow::showTargetMenu()
{
    auto p = juce::PopupMenu();
    p.addSectionHeader("Select Target");
    p.addSeparator();
    for (const auto &[name, group] : panel.targetMenu)
    {
        if (name == "")
        {
            for (auto &[t, id] : group)
            {
                p.addItem(t, [setTo = id, w = juce::Component::SafePointer(this)]() {
                    if (w)
                    {
                        w->tgtvalue = setTo;
                        w->updateLabels();
                        w->resendData();
                    }
                });
            }
        }
        else
        {
            auto sm = juce::PopupMenu();
            for (auto &[t, id] : group)
            {
                sm.addItem(t, [setTo = id, w = juce::Component::SafePointer(this)]() {
                    if (w)
                    {
                        w->tgtvalue = setTo;
                        w->updateLabels();
                        w->resendData();
                    }
                });
            }
            p.addSubMenu(name, sm);
        }
    }
    // p.showMenuAsync(juce::PopupMenu::Options().withParentComponent(this));
    p.showMenuAsync(juce::PopupMenu::Options());
}

VoiceOutputPanel::VoiceOutputPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                                   sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Voice Output"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 2, 1>>();
    content->addKnob(e, ConduitPolysynth::pmVoicePan, 0, 0, "Pan");
    content->addKnob(e, ConduitPolysynth::pmVoiceLevel, 1, 0, "Level");
    setContentAreaComponent(std::move(content));
}

StatusPanel::StatusPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                         sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Global"), uic(p), ed(e)
{
    auto content = std::make_unique<Content>(this);

    mpeButton =
        std::make_unique<jcad::DiscreteToValueReference<jcmp::ToggleButton, bool>>(mpeActive);
    mpeButton->widget->setLabel("MPE Mode");
    mpeButton->onValueChanged = [w = juce::Component::SafePointer(this)](auto v) {
        if (w)
            w->pushMPEStatus();
    };
    content->addAndMakeVisible(*(mpeButton->widget));

    vuMeter = std::make_unique<jcmp::VUMeter>();
    vuMeter->direction = jcmp::VUMeter::VERTICAL;
    content->addAndMakeVisible(*vuMeter);

    voiceCountLabel = std::make_unique<jcmp::Label>();
    voiceCountLabel->setText("Voices: 0");
    content->addAndMakeVisible(*voiceCountLabel);

    setContentAreaComponent(std::move(content));

    ed.comms->addIdleHandler("status", [this]() { updateStatus(); });
}

StatusPanel::~StatusPanel() { ed.comms->removeIdleHandler("status"); }

void StatusPanel::updateStatus()
{
    vuMeter->setLevels(uic.dataCopyForUI.mainVU[0], uic.dataCopyForUI.mainVU[1]);
    voiceCountLabel->setText("Voices : " + std::to_string(uic.dataCopyForUI.polyphony));
    repaint();
}

ModFXPanel::ModFXPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                       sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Modulation Effect"), uic(p), ed(e)
{
    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmModFXActive);

    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 5, 1>>();
    content->addMultiSwitch(e, ConduitPolysynth::pmModFXType, 0, 0, "");
    auto ms = content->addMultiSwitch(e, ConduitPolysynth::pmModFXPreset, 1, 0, "");
    ms->direction = jcmp::MultiSwitch::HORIZONTAL;
    content->layout.setColspanAt(1, 2);

    auto rt = content->addKnob(e, ConduitPolysynth::pmModFXRate, 3, 0, "Rate");
    rt->pathDrawMode = jcmp::Knob::PathDrawMode::ALWAYS_FROM_MIN;

    auto ts = std::make_unique<jcmp::ToggleButton>();
    ts->setGlyph(jcmp::GlyphPainter::GlyphType::METRONOME);
    content->addAndMakeVisible(*ts);
    e.comms->attachDiscreteToParam(ts.get(), ConduitPolysynth::pmModFXRateTemposync);
    content->dknobs[ConduitPolysynth::pmModFXRateTemposync] = std::move(ts);

    content->additionalResizeHandler = [](auto *ct) {
        auto pRt = ConduitPolysynth::pmModFXRate;
        auto pTs = ConduitPolysynth::pmModFXRateTemposync;

        const auto &pK = ct->knobs[pRt];
        const auto &pT = ct->dknobs[pTs];

        pT->setBounds(pK->getRight() - 6, pK->getY() - 2, 10, 10);
    };

    content->addKnob(e, ConduitPolysynth::pmModFXMix, 4, 0, "Mix");

    setContentAreaComponent(std::move(content));
}

ReverbPanel::ReverbPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                         sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Reverb Effect"), uic(p), ed(e)
{
    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmRevFXActive);

    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 4, 1>>();
    auto ms = content->addMultiSwitch(e, ConduitPolysynth::pmRevFXPreset, 0, 0, "");
    ms->direction = jcmp::MultiSwitch::HORIZONTAL;
    content->layout.setColspanAt(0, 2);

    auto dk = content->addKnob(e, ConduitPolysynth::pmRevFXTime, 2, 0, "Decay");
    dk->pathDrawMode = jcmp::Knob::ALWAYS_FROM_MIN;

    content->addKnob(e, ConduitPolysynth::pmRevFXMix, 3, 0, "Mix");
    setContentAreaComponent(std::move(content));
}

} // namespace sst::conduit::polysynth::editor
namespace sst::conduit::polysynth
{
std::unique_ptr<juce::Component> ConduitPolysynth::createEditor()
{
    uiComms.refreshUIValues = true;
    auto innards =
        std::make_unique<sst::conduit::polysynth::editor::ConduitPolysynthEditor>(uiComms);
    auto editor = std::make_unique<conduit::shared::EditorBase<ConduitPolysynth>>(uiComms);
    editor->setContentComponent(std::move(innards));

    return editor;
}
} // namespace sst::conduit::polysynth