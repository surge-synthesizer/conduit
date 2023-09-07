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

#include "polysynth.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "sst/jucegui/style/StyleSheet.h"
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

struct ConduitPolysynthEditor : public jcmp::WindowPanel
{
    uicomm_t &uic;
    std::unique_ptr<sst::conduit::shared::EditorCommunicationsHandler<ConduitPolysynth>> comms;

    ConduitPolysynthEditor(uicomm_t &p) : uic(p)
    {
        comms =
            std::make_unique<sst::conduit::shared::EditorCommunicationsHandler<ConduitPolysynth>>(
                p);

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
    auto editor = std::make_unique<conduit::shared::EditorBase>();
    editor->setContentComponent(std::move(innards));

    return editor;
}
} // namespace sst::conduit::polysynth