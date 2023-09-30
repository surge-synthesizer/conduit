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
#include "sst/jucegui/components/MultiSwitch.h"
#include "sst/jucegui/data/Continuous.h"
#include "conduit-shared/editor-base.h"
#include <sst/jucegui/layouts/LabeledGrid.h>

namespace sst::conduit::polysynth::editor
{
namespace jcmp = sst::jucegui::components;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit::polysynth::ConduitPolysynth;
using uicomm_t = cps_t::UICommunicationBundle;

struct ConduitPolysynthEditor;

template <typename editor_t, int lx, int ly> struct GridContentBase : juce::Component
{
    GridContentBase() { layout.setControlCellSize(60, 60); }
    ~GridContentBase()
    {
        for (auto &[p, k] : knobs)
            if (k)
                k->setSource(nullptr);

        for (auto &[p, k] : dknobs)
            if (k)
                k->setSource(nullptr);
    }
    void resized() override { layout.resize(getLocalBounds()); }

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
    };

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
    };
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
};

struct OutputPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    OutputPanel(uicomm_t &p, ConduitPolysynthEditor &e);
};

struct StatusPanel : jcmp::NamedPanel
{
    uicomm_t &uic;
    ConduitPolysynthEditor &ed;

    StatusPanel(uicomm_t &p, ConduitPolysynthEditor &e);
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

        outputPanel = std::make_unique<OutputPanel>(uic, *this);
        addAndMakeVisible(*outputPanel);

        statusPanel = std::make_unique<StatusPanel>(uic, *this);
        addAndMakeVisible(*statusPanel);

        setSize(1000, 700);

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
        aegPanel->setBounds(oscWidth, 0, envWidth, envHeight);
        fegPanel->setBounds(oscWidth, envHeight, envWidth, envHeight);

        static constexpr int filterWidth{oscWidth / 5 * 3};
        static constexpr int filterXPos{envWidth + oscWidth};
        lpfPanel->setBounds(filterXPos, 0, filterWidth, oscHeight);
        svfPanel->setBounds(filterXPos, oscHeight, filterWidth, oscHeight);

        static constexpr int wsXPos = filterXPos + filterWidth;
        static constexpr int wsWidth = {oscWidth / 5 * 2};
        wsPanel->setBounds(wsXPos, 0, wsWidth, oscHeight);
        routingPanel->setBounds(wsXPos, oscHeight, wsWidth, oscHeight);

        static constexpr int lfoWidth{(oscWidth + envWidth) / 2};
        lfo1Panel->setBounds(0, 3 * oscHeight, lfoWidth, oscHeight);
        lfo2Panel->setBounds(lfoWidth, 3 * oscHeight, lfoWidth, oscHeight);

        static constexpr int outputWidth{wsWidth};
        outputPanel->setBounds(wsXPos + wsWidth, 0, outputWidth, 3 * oscHeight);
        statusPanel->setBounds(wsXPos + wsWidth, 3 * oscHeight, outputWidth, oscHeight);
        modMatrixPanel->setBounds(oscWidth + envWidth, 2 * oscHeight, filterWidth + wsWidth,
                                  2 * oscHeight);
    }

    std::unique_ptr<jcmp::NamedPanel> sawPanel, pulsePanel, sinPanel, noisePanel;
    std::unique_ptr<jcmp::NamedPanel> aegPanel, fegPanel;
    std::unique_ptr<jcmp::NamedPanel> lfo1Panel, lfo2Panel;
    std::unique_ptr<jcmp::NamedPanel> lpfPanel, svfPanel, wsPanel, routingPanel;
    std::unique_ptr<jcmp::NamedPanel> modMatrixPanel;
    std::unique_ptr<jcmp::NamedPanel> outputPanel, statusPanel;
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
    : jcmp::NamedPanel("Pusle Width Osc"), uic(p), ed(e)
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
    content->addVSlider(e, ConduitPolysynth::pmFegToLPFCutoff, 4, 0, "LPF");
    content->addVSlider(e, ConduitPolysynth::pmFegToSVFCutoff, 5, 0, "SVF");

    setContentAreaComponent(std::move(content));
}

LPFPanel::LPFPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                   sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Low Pass Filter"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 3, 1>>();

    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmLPFActive);

    content->addMultiSwitch(e, ConduitPolysynth::pmLPFFilterMode, 0, 0, "");
    content->addKnob(e, ConduitPolysynth::pmLPFCutoff, 1, 0, "Cutoff");
    content->addKnob(e, ConduitPolysynth::pmLPFResonance, 2, 0, "Resonance");

    setContentAreaComponent(std::move(content));
}

SVFPanel::SVFPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                   sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("State Variable Filter"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 3, 1>>();

    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmSVFActive);

    content->addMultiSwitch(e, ConduitPolysynth::pmSVFFilterMode, 0, 0, "");
    content->addKnob(e, ConduitPolysynth::pmSVFCutoff, 1, 0, "Cutoff");
    content->addKnob(e, ConduitPolysynth::pmSVFResonance, 2, 0, "Resonance");

    setContentAreaComponent(std::move(content));
}

WSPanel::WSPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                 sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Waveshaper"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 2, 1>>();

    setTogglable(true);
    e.comms->attachDiscreteToParam(toggleButton.get(), ConduitPolysynth::pmWSActive);

    content->addMultiSwitch(e, ConduitPolysynth::pmWSMode, 0, 0, "");
    content->addKnob(e, ConduitPolysynth::pmWSDrive, 1, 0, "Drive");

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
    content->addKnob(e, ConduitPolysynth::pmLFORate + which * ConduitPolysynth::offPmLFO2, 1, 0,
                     "Rate");
    content->addKnob(e, ConduitPolysynth::pmLFOAmplitude + which * ConduitPolysynth::offPmLFO2, 2,
                     0, "Amp");
    content->addKnob(e, ConduitPolysynth::pmLFODeform + which * ConduitPolysynth::offPmLFO2, 3, 0,
                     "Deform");

    setContentAreaComponent(std::move(content));
}

ModMatrixPanel::ModMatrixPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                               sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Mod Matrix"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 2, 1>>();
}

OutputPanel::OutputPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                         sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Main Output"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 2, 1>>();
}

StatusPanel::StatusPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                         sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : jcmp::NamedPanel("Status"), uic(p), ed(e)
{
    auto content = std::make_unique<GridContentBase<ConduitPolysynthEditor, 2, 1>>();
}

} // namespace sst::conduit::polysynth::editor
namespace sst::conduit::polysynth
{
std::unique_ptr<juce::Component> ConduitPolysynth::createEditor()
{
    uiComms.refreshUIValues = true;
    auto innards =
        std::make_unique<sst::conduit::polysynth::editor::ConduitPolysynthEditor>(uiComms);
    auto editor = std::make_unique<conduit::shared::EditorBase<ConduitPolysynth>>(
        uiComms, desc.name, desc.id);
    editor->setContentComponent(std::move(innards));

    return editor;
}
} // namespace sst::conduit::polysynth