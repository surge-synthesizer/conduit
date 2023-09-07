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

        /*
        auto oct = std::make_unique<OscPanel>(uic, *this);
        oscPanel->setContentAreaComponent(std::move(oct));
         */

        setSize(600, 200);

        comms->startProcessing();
    }

    ~ConduitPolymetricDelayEditor() { comms->stopProcessing(); }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override { ctrlPanel->setBounds(getLocalBounds()); }

    std::unique_ptr<jcmp::NamedPanel> ctrlPanel;
};

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