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
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/data/Continuous.h"
#include "conduit-shared/editor-base.h"

namespace sst::conduit::polysynth::editor
{
namespace jcmp = sst::jucegui::components;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit::polysynth::ConduitPolysynth;
using uicomm_t = cps_t::UICommunicationBundle;

struct ConduitPolysynthEditor;

struct OscPanel : public juce::Component
{
    uicomm_t &uic;

    OscPanel(uicomm_t &p, ConduitPolysynthEditor &e);
    ~OscPanel() { oscUnisonSpread->setSource(nullptr); }

    void resized() override { oscUnisonSpread->setBounds({40, 40, 90, 90}); }

    std::unique_ptr<jcmp::Knob> oscUnisonSpread;
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

        oscPanel = std::make_unique<jcmp::NamedPanel>("Oscillator");
        addAndMakeVisible(*oscPanel);

        auto oct = std::make_unique<OscPanel>(uic, *this);
        oscPanel->setContentAreaComponent(std::move(oct));

        vcfPanel = std::make_unique<jcmp::NamedPanel>("Filter");
        addAndMakeVisible(*vcfPanel);

        setSize(500, 400);

        comms->startProcessing();
    }

    ~ConduitPolysynthEditor() { comms->stopProcessing(); }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override
    {
        auto mpWidth = 250;
        oscPanel->setBounds(getLocalBounds().withWidth(mpWidth));
        vcfPanel->setBounds(getLocalBounds().withTrimmedLeft(mpWidth));
    }

    std::unique_ptr<jcmp::NamedPanel> vcfPanel;
    std::unique_ptr<jcmp::NamedPanel> oscPanel;
};

OscPanel::OscPanel(sst::conduit::polysynth::editor::uicomm_t &p,
                   sst::conduit::polysynth::editor::ConduitPolysynthEditor &e)
    : uic(p)
{
    oscUnisonSpread = std::make_unique<jcmp::Knob>();
    addAndMakeVisible(*oscUnisonSpread);

    e.comms->attachContinuousToParam(oscUnisonSpread.get(), cps_t::paramIds::pmUnisonSpread);
}

} // namespace sst::conduit::polysynth::editor
namespace sst::conduit::polysynth
{
std::unique_ptr<juce::Component> ConduitPolysynth::createEditor()
{
    uiComms.refreshUIValues = true;
    auto innards =
        std::make_unique<sst::conduit::polysynth::editor::ConduitPolysynthEditor>(uiComms);
    auto editor = std::make_unique<conduit::shared::EditorBase>(desc.name, desc.id);
    editor->setContentComponent(std::move(innards));

    return editor;
}
} // namespace sst::conduit::polysynth