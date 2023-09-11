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

#include "chord-memory.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/data/Continuous.h"
#include "conduit-shared/editor-base.h"

namespace sst::conduit::chord_memory::editor
{
namespace jcmp = sst::jucegui::components;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit::chord_memory::ConduitChordMemory;
using uicomm_t = cps_t::UICommunicationBundle;

struct ConduitChordMemoryEditor;

struct ControlsPanel : juce::Component
{
    uicomm_t &uic;

    ControlsPanel(uicomm_t &p, ConduitChordMemoryEditor &e);
    ~ControlsPanel() { time->setSource(nullptr); }

    void resized() override
    {
        auto b = getLocalBounds().reduced(5);
        auto ks = std::min(b.getWidth(), b.getHeight());

        int yp = 0;
        auto bx = b.withHeight(ks).withWidth(ks - 18).reduced(4);
        time->setBounds(bx);
    }

    std::unique_ptr<jcmp::Knob> time;
};

struct ConduitChordMemoryEditor : public jcmp::WindowPanel, shared::TooltipSupport
{
    uicomm_t &uic;
    using comms_t = sst::conduit::shared::EditorCommunicationsHandler<ConduitChordMemory,
                                                                      ConduitChordMemoryEditor>;
    std::unique_ptr<comms_t> comms;

    ConduitChordMemoryEditor(uicomm_t &p) : uic(p)
    {
        comms = std::make_unique<comms_t>(p, *this);

        ctrlPanel = std::make_unique<jcmp::NamedPanel>("Controls");
        addAndMakeVisible(*ctrlPanel);

        auto oct = std::make_unique<ControlsPanel>(uic, *this);
        ctrlPanel->setContentAreaComponent(std::move(oct));

        setSize(600, 200);

        comms->startProcessing();
    }

    ~ConduitChordMemoryEditor() { comms->stopProcessing(); }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override { ctrlPanel->setBounds(getLocalBounds()); }

    std::unique_ptr<jcmp::NamedPanel> ctrlPanel;
};

ControlsPanel::ControlsPanel(sst::conduit::chord_memory::editor::uicomm_t &p,
                             sst::conduit::chord_memory::editor::ConduitChordMemoryEditor &e)
    : uic(p)
{
    // TODO: Stepped knob
    time = std::make_unique<jcmp::Knob>();
    addAndMakeVisible(*time);
    e.comms->attachContinuousToParam(time.get(), cps_t::paramIds::pmKeyShift);
}
} // namespace sst::conduit::chord_memory::editor

namespace sst::conduit::chord_memory
{
std::unique_ptr<juce::Component> ConduitChordMemory::createEditor()
{
    uiComms.refreshUIValues = true;
    auto innards =
        std::make_unique<sst::conduit::chord_memory::editor::ConduitChordMemoryEditor>(uiComms);
    auto editor = std::make_unique<conduit::shared::EditorBase>(desc.name, desc.id);
    editor->setContentComponent(std::move(innards));

    return editor;
}
} // namespace sst::conduit::chord_memory
