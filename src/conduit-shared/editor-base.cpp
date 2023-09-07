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

#include "editor-base.h"
#include "sst/jucegui/style/StyleSheet.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/data/Continuous.h"

namespace sst::conduit::shared
{
namespace jcmp = sst::jucegui::components;
struct Background : public jcmp::WindowPanel
{
    Background()
    {
        sst::jucegui::style::StyleSheet::initializeStyleSheets([]() {});
        const auto &base = sst::jucegui::style::StyleSheet::getBuiltInStyleSheet(
            sst::jucegui::style::StyleSheet::DARK);
        base->setColour(jcmp::WindowPanel::Styles::styleClass,
                        jcmp::WindowPanel::Styles::backgroundgradstart, juce::Colour(60, 60, 70));
        base->setColour(jcmp::WindowPanel::Styles::styleClass,
                        jcmp::WindowPanel::Styles::backgroundgradend, juce::Colour(20, 20, 30));
        base->setColour(jcmp::BaseStyles::styleClass, jcmp::BaseStyles::regionBorder,
                        juce::Colour(90, 90, 100));
        base->setColour(jcmp::NamedPanel::Styles::styleClass,
                        jcmp::NamedPanel::Styles::regionLabelCol, juce::Colour(210, 210, 230));
        setStyle(base);
    }
    void resized() override
    {
        auto lb = getLocalBounds();
        if (contents)
        {
            lb = lb.withTrimmedTop(EditorBase::headerSize);
            contents->setBounds(lb);
        }
    }

    std::unique_ptr<juce::Component> contents;
};
EditorBase::EditorBase()
{
    container = std::make_unique<Background>();
    addAndMakeVisible(*container);
}
EditorBase::~EditorBase() = default;

void EditorBase::setContentComponent(std::unique_ptr<juce::Component> c)
{
    container->contents = std::move(c);
    container->addAndMakeVisible(*container->contents);
    auto b = container->contents->getBounds();
    b = b.expanded(0, headerSize);

    setSize(b.getWidth(), b.getHeight());
}

void EditorBase::resized() { container->setBounds(getLocalBounds()); }
} // namespace sst::conduit::shared