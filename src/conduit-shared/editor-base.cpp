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

#include "editor-base.h"
#include "sst/jucegui/style/StyleSheet.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/data/Continuous.h"
#include "debug-helpers.h"
#include <cmrc/cmrc.hpp>
#include "version.h"

CMRC_DECLARE(conduit_resources);

namespace sst::conduit::shared
{

static constexpr int headerSize{35};
static constexpr int footerSize{18};

namespace jcmp = sst::jucegui::components;
struct Background : public jcmp::WindowPanel
{
    Background(const std::string &pluginName, const std::string &pluginId)
    {
        labelsTypeface = loadFont("Inter/static/Inter-Medium.ttf");
        versionTypeface = loadFont("Anonymous_Pro/AnonymousPro-Regular.ttf");

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

        base->replaceFontsWithTypeface(labelsTypeface);

        setStyle(base);

        auto lbFont = juce::Font(24);
        auto vsFont = juce::Font(12);

        if (labelsTypeface)
        {
            lbFont = juce::Font(labelsTypeface).withHeight(24);
        }
        if (versionTypeface)
        {
            vsFont = juce::Font(versionTypeface).withHeight(13);
        }

        // FIXME do this with sst labels and a style class
        auto nl = std::make_unique<juce::Label>("Plugin Name", pluginName);
        nl->setColour(juce::Label::ColourIds::textColourId, juce::Colour(220, 220, 230));
        nl->setFont(lbFont);
        addAndMakeVisible(*nl);
        nameLabel = std::move(nl);

        auto vs = std::string(sst::conduit::build::BuildDate) + " " +
                  std::string(sst::conduit::build::BuildTime) + " " + sst::conduit::build::GitHash;
        auto vl = std::make_unique<juce::Label>("Plugin Version", vs);
        vl->setColour(juce::Label::ColourIds::textColourId, juce::Colour(220, 220, 230));
        vl->setFont(vsFont);
        vl->setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(*vl);
        versionLabel = std::move(vl);
    }

    void resized() override
    {
        auto lb = getLocalBounds();
        if (contents)
        {
            auto cb = lb.withTrimmedTop(headerSize).withTrimmedBottom(footerSize);
            contents->setBounds(cb);
        }
        nameLabel->setBounds(lb.withHeight(headerSize));
        versionLabel->setBounds(
            lb.withTrimmedTop(lb.getHeight() - footerSize).withTrimmedBottom(2));
    }

    juce::Typeface::Ptr loadFont(const std::string &path)
    {
        try
        {
            auto fs = cmrc::conduit_resources::get_filesystem();
            auto fntf = fs.open(path);
            std::vector<char> fontData(fntf.begin(), fntf.end());

            auto res = juce::Typeface::createSystemTypefaceFor(fontData.data(), fontData.size());
            return res;
        }
        catch (std::exception &e)
        {
        }
        CNDOUT << "Font '" << path << "' failed to load" << std::endl;
        return nullptr;
    }

    juce::Typeface::Ptr labelsTypeface{nullptr}, versionTypeface{nullptr};

    std::unique_ptr<juce::Component> contents;
    std::unique_ptr<juce::Component> nameLabel, versionLabel;
};
EditorBase::EditorBase(const std::string &pluginName, const std::string &pluginId)
{
    container = std::make_unique<Background>(pluginName, pluginId);
    addAndMakeVisible(*container);
}
EditorBase::~EditorBase() = default;

void EditorBase::setContentComponent(std::unique_ptr<juce::Component> c)
{
    container->contents = std::move(c);
    auto b = container->contents->getBounds();

    b = b.withHeight(b.getHeight() + footerSize + headerSize);
    setSize(b.getWidth(), b.getHeight());

    container->addAndMakeVisible(*container->contents);
}

void EditorBase::resized() { container->setBounds(getLocalBounds()); }
} // namespace sst::conduit::shared