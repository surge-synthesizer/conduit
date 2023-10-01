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

#ifndef CONDUIT_SRC_CONDUIT_SHARED_EDITOR_BASE_H
#define CONDUIT_SRC_CONDUIT_SHARED_EDITOR_BASE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/data/Continuous.h"
#include "sst/jucegui/data/Discrete.h"
#include "sst/jucegui/components/ContinuousParamEditor.h"
#include "sst/jucegui/components/DiscreteParamEditor.h"
#include "sst/jucegui/components/GlyphButton.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "debug-helpers.h"
#include "version.h"
#include "cmrc/cmrc.hpp"

CMRC_DECLARE(conduit_resources);

namespace sst::conduit::shared
{

static constexpr int headerSize{35};
static constexpr int footerSize{18};

template <typename Content> struct EditorBase;

template <typename Content> struct Background : sst::jucegui::components::WindowPanel
{
    EditorBase<Content> &eb;
    Background(const std::string &pluginName, const std::string &pluginId, EditorBase<Content> &e);
    void resized() override;

    void buildBurger();
    juce::Typeface::Ptr labelsTypeface{nullptr}, versionTypeface{nullptr};

    std::unique_ptr<juce::Component> contents;
    std::unique_ptr<juce::Component> nameLabel, versionLabel;

    std::unique_ptr<sst::jucegui::components::GlyphButton> menuButton;

    void loadsave(bool doSave);
    std::unique_ptr<juce::FileChooser> fileChooser;
};

template <typename Content> struct EditorBase : juce::Component
{
    typename Content::UICommunicationBundle &uic;
    EditorBase(typename Content::UICommunicationBundle &, const std::string &pluginName,
               const std::string &pluginId);
    ~EditorBase() = default;

    void setContentComponent(std::unique_ptr<juce::Component> c)
    {
        container->contents = std::move(c);
        auto b = container->contents->getBounds();

        b = b.withHeight(b.getHeight() + footerSize + headerSize);
        setSize(b.getWidth(), b.getHeight());

        container->addAndMakeVisible(*container->contents);
    }
    void resized() override
    {
        if (container)
            container->setBounds(getLocalBounds());
    }

    virtual void populatePluginHamburgerItems(juce::PopupMenu &m) {}

    juce::Typeface::Ptr loadFont(const std::string &path);

    std::unique_ptr<Background<Content>> container;
    std::string pluginName, pluginId;
};

template <typename T> struct ToolTipMixIn
{
    struct ToolTip : juce::Component
    {
        std::string title;
        std::string value;
        void paint(juce::Graphics &g)
        {
            g.fillAll(juce::Colours::white);
            g.setColour(juce::Colours::black);
            g.fillRect(getLocalBounds().reduced(1));

            g.setColour(juce::Colours::white);
            g.setFont(12);
            auto b = getLocalBounds().reduced(3, 1).withHeight(getHeight() / 2);
            g.drawText(title, b, juce::Justification::centredLeft);
            b = b.translated(0, getHeight() / 2);
            g.drawText(value, b, juce::Justification::centredLeft);
        }
    };

    T *asT() { return static_cast<T *>(this); }

    std::unique_ptr<ToolTip> toolTip;

    void guaranteeTooltip()
    {
        if (!toolTip)
        {
            toolTip = std::make_unique<ToolTip>();
            asT()->addChildComponent(*toolTip);
        }
    }

    uint32_t ttDisplay{0};
    void openTooltip(uint32_t p, juce::Component *around, float value)
    {
        guaranteeTooltip();

        ttDisplay = p;
        toolTip->setSize(100, 35);

        int x = 0;
        int y = 0;
        juce::Component *component = around;
        while (component && component != asT())
        {
            auto bounds = component->getBoundsInParent();
            x += bounds.getX();
            y += bounds.getY();

            component = component->getParentComponent();
        }
        auto ttpos = juce::Rectangle<int>(x, y + around->getHeight(), toolTip->getWidth(),
                                          toolTip->getHeight());
        if (!asT()->getLocalBounds().contains(ttpos))
        {
            // try above
            ttpos = juce::Rectangle<int>(x, y - toolTip->getHeight(), toolTip->getWidth(),
                                         toolTip->getHeight());

            // TODO : left and right
        }
        toolTip->setBounds(ttpos);
        toolTip->setVisible(true);
        toolTip->toFront(false);

        updateTooltip(p, value);
    }

    void updateTooltip(uint32_t p, float f)
    {
        if (ttDisplay != p)
            return;

        guaranteeTooltip();

        auto pd = asT()->uic.getParameterDescription(p);
        toolTip->title = pd.name;
        auto sv = pd.valueToString(f);
        if (sv.has_value())
            toolTip->value = *sv;
        else
            toolTip->value = "ERROR";
        toolTip->repaint();
    }
    void closeTooltip(uint32_t p)
    {
        ttDisplay = 0;
        guaranteeTooltip();
        toolTip->setVisible(false);
    }
};

template <typename T, typename TEd> struct EditorCommunicationsHandler
{
    struct IdleTimer : juce::Timer
    {
        EditorCommunicationsHandler<T, TEd> &icb;
        IdleTimer(EditorCommunicationsHandler<T, TEd> &ji) : icb(ji) {}
        void timerCallback() override { icb.onIdle(); }
    };
    std::unique_ptr<IdleTimer> idleTimer;
    typename T::UICommunicationBundle &uic;
    TEd &ed;
    EditorCommunicationsHandler(typename T::UICommunicationBundle &b, TEd &e) : uic(b), ed(e) {}
    ~EditorCommunicationsHandler()
    {
        assert(!idleTimer); // hit this and you never called stop or start
    }

    std::unordered_map<std::string, std::function<void()>> idleHandlers;
    void addIdleHandler(const std::string &key, std::function<void()> op)
    {
        idleHandlers[key] = op;
    }
    void removeIdleHandler(const std::string &key) { idleHandlers.erase(key); }

    void onIdle()
    {
        using ToUI = typename T::UICommunicationBundle::SynthToUI_Queue_t::value_type;

        while (!uic.toUiQ.empty())
        {
            auto r = *uic.toUiQ.pop();

            if (r.type == ToUI::MType::PARAM_VALUE)
            {
                auto p = dataTargets.find(r.id);
                if (p != dataTargets.end())
                {
                    p->second.second->setValueFromModel(r.value);
                    p->second.first->repaint();
                }
                else
                {
                    auto pd = discreteDataTargets.find(r.id);
                    if (pd != discreteDataTargets.end())
                    {
                        pd->second.second->setValueFromModel((int)r.value);
                        pd->second.first->repaint();
                    }
                }
            }
            else
            {
            }
        }

        for (auto &[k, f] : idleHandlers)
        {
            f();
        }
    }

    std::unordered_map<uint32_t,
                       std::pair<juce::Component *, sst::jucegui::data::ContinunousModulatable *>>
        dataTargets;
    std::unordered_map<uint32_t, std::pair<juce::Component *, sst::jucegui::data::Discrete *>>
        discreteDataTargets;

    void startProcessing()
    {
        idleTimer = std::make_unique<IdleTimer>(*this);
        idleTimer->startTimerHz(60);
    }
    void stopProcessing()
    {
        idleTimer->stopTimer();
        idleTimer.reset();
    }

    struct D2QContinuousParam : sst::jucegui::data::ContinunousModulatable
    {
        typename T::UICommunicationBundle &uic;
        uint32_t pid;
        typename T::ParamDesc pDesc{};
        TEd &editor;

        D2QContinuousParam(TEd &ed, typename T::UICommunicationBundle &p, uint32_t pid)
            : uic(p), pid(pid), editor(ed)
        {
            pDesc = uic.getParameterDescription(pid);
        }
        std::string getLabel() const override { return pDesc.name; }

        float f{0.f};
        float getValue() const override { return f; }
        void setValueFromGUI(const float &fi) override
        {
            using FromUI = typename T::UICommunicationBundle::UIToSynth_Queue_t::value_type;

            editor.updateTooltip(pid, f);
            f = fi;
            uic.fromUiQ.push({FromUI::MType::ADJUST_VALUE, pid, f});
            uic.requestHostParamFlush();
        }
        void setValueFromModel(const float &fi) override { f = fi; }
        float getDefaultValue() const override { return pDesc.defaultVal; }
        float getMin() const override { return pDesc.minVal; }
        float getMax() const override { return pDesc.maxVal; }

        float getModulationValuePM1() const override { return 0; }
        void setModulationValuePM1(const float &f) override {}
        bool isModulationBipolar() const override { return false; }
    };

    struct D2QDiscreteParam : sst::jucegui::data::Discrete
    {
        typename T::UICommunicationBundle &uic;
        uint32_t pid;
        typename T::ParamDesc pDesc{};
        TEd &editor;

        D2QDiscreteParam(TEd &ed, typename T::UICommunicationBundle &p, uint32_t pid)
            : uic(p), pid(pid), editor(ed)
        {
            pDesc = uic.getParameterDescription(pid);
        }
        std::string getLabel() const override { return pDesc.name; }

        int val{0};
        int getValue() const override { return val; }
        void setValueFromGUI(const int &vi) override
        {
            using FromUI = typename T::UICommunicationBundle::UIToSynth_Queue_t::value_type;

            val = vi;

            editor.updateTooltip(pid, val);
            uic.fromUiQ.push({FromUI::MType::ADJUST_VALUE, pid, (float)val});
            uic.requestHostParamFlush();
        }

        void setValueFromModel(const int &vi) override { val = vi; }
        int getDefaultValue() const override { return pDesc.defaultVal; }
        int getMin() const override { return (int)pDesc.minVal; }
        int getMax() const override { return (int)pDesc.maxVal; }

        std::string getValueAsStringFor(int i) const override
        {
            auto s = pDesc.valueToString((float)i);
            if (s.has_value())
                return *s;
            return "ERR " + std::to_string(i);
        }
    };

    void attachContinuousToParam(sst::jucegui::components::ContinuousParamEditor *comp,
                                 uint32_t pid)
    {
        using FromUI = typename T::UICommunicationBundle::UIToSynth_Queue_t::value_type;
        auto source = std::make_unique<D2QContinuousParam>(ed, uic, pid);
        comp->setSource(source.get());

        comp->onBeginEdit = [this, w = juce::Component::SafePointer(comp), pid]() {
            uic.fromUiQ.push({FromUI::MType::BEGIN_EDIT, pid, 1});
            if (w)
            {
                ed.openTooltip(pid, w, w->source->getValue());
            }
        };
        comp->onEndEdit = [this, pid]() {
            uic.fromUiQ.push({FromUI::MType::END_EDIT, pid, 1});
            ed.closeTooltip(pid);
        };
        comp->onIdleHover = [this, w = juce::Component::SafePointer(comp), pid]() {
            if (w)
            {
                ed.openTooltip(pid, w, w->source->getValue());
            }
        };
        comp->onIdleHoverEnd = [this, pid]() { ed.closeTooltip(pid); };

        dataTargets[pid] = {comp, source.get()};
        ownedData[pid] = std::move(source);
    }

    void attachDiscreteToParam(sst::jucegui::components::DiscreteParamEditor *comp, uint32_t pid)
    {
        using FromUI = typename T::UICommunicationBundle::UIToSynth_Queue_t::value_type;
        auto source = std::make_unique<D2QDiscreteParam>(ed, uic, pid);
        comp->setSource(source.get());

        comp->onBeginEdit = [this, w = juce::Component::SafePointer(comp), pid]() {
            uic.fromUiQ.push({FromUI::MType::BEGIN_EDIT, pid, 1});
            if (w)
            {
                ed.openTooltip(pid, w, w->data->getValue());
            }
        };
        comp->onEndEdit = [this, pid]() {
            uic.fromUiQ.push({FromUI::MType::END_EDIT, pid, 1});
            ed.closeTooltip(pid);
        };
        comp->onIdleHover = [this, w = juce::Component::SafePointer(comp), pid]() {
            if (w)
            {
                ed.openTooltip(pid, w, w->data->getValue());
            }
        };
        comp->onIdleHoverEnd = [this, pid]() { ed.closeTooltip(pid); };

        discreteDataTargets[pid] = {comp, source.get()};
        ownedDataDiscrete[pid] = std::move(source);
    }

    std::unordered_map<uint32_t, std::unique_ptr<D2QContinuousParam>> ownedData;
    std::unordered_map<uint32_t, std::unique_ptr<D2QDiscreteParam>> ownedDataDiscrete;
};

namespace jcmp = sst::jucegui::components;
template <typename Content>
Background<Content>::Background(const std::string &pluginName, const std::string &pluginId,
                                EditorBase<Content> &e)
    : eb(e)

{
    labelsTypeface = eb.loadFont("Inter/static/Inter-Medium.ttf");
    versionTypeface = eb.loadFont("Anonymous_Pro/AnonymousPro-Regular.ttf");

    sst::jucegui::style::StyleSheet::initializeStyleSheets([]() {});
    const auto &base = sst::jucegui::style::StyleSheet::getBuiltInStyleSheet(
        sst::jucegui::style::StyleSheet::DARK);
    base->setColour(jcmp::WindowPanel::Styles::styleClass,
                    jcmp::WindowPanel::Styles::backgroundgradstart, juce::Colour(60, 60, 70));
    base->setColour(jcmp::WindowPanel::Styles::styleClass,
                    jcmp::WindowPanel::Styles::backgroundgradend, juce::Colour(20, 20, 30));
    base->setColour(jcmp::BaseStyles::styleClass, jcmp::BaseStyles::regionBorder,
                    juce::Colour(90, 90, 100));
    base->setColour(jcmp::NamedPanel::Styles::styleClass, jcmp::NamedPanel::Styles::regionLabelCol,
                    juce::Colour(210, 210, 230));

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

    auto gb = std::make_unique<jcmp::GlyphButton>(jcmp::GlyphPainter::HAMBURGER);
    gb->setOnCallback([w = juce::Component::SafePointer(this)]() {
        if (w)
            w->buildBurger();
    });
    gb->setIsInactiveValue(true);
    addAndMakeVisible(*gb);
    menuButton = std::move(gb);
}

template <typename Content> void Background<Content>::resized()
{
    auto lb = getLocalBounds();
    if (contents)
    {
        auto cb = lb.withTrimmedTop(headerSize).withTrimmedBottom(footerSize);
        contents->setBounds(cb);
    }
    nameLabel->setBounds(lb.withHeight(headerSize).withTrimmedRight(headerSize - 4));
    auto gb = lb.withHeight(headerSize)
                  .withWidth(headerSize)
                  .translated(lb.getWidth() - headerSize - 2, 0)
                  .reduced(5);
    menuButton->setBounds(gb);
    versionLabel->setBounds(lb.withTrimmedTop(lb.getHeight() - footerSize).withTrimmedBottom(2));
}

template <typename Content> void Background<Content>::buildBurger()
{
    juce::PopupMenu menu;
    menu.addSectionHeader(eb.pluginName);
    eb.populatePluginHamburgerItems(menu);
    menu.addSeparator();
    menu.addItem("Save Settings To...", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->loadsave(true);
    });
    menu.addItem("Load Settings From...", [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->loadsave(false);
    });
    menu.addItem("About", []() {});

    menu.showMenuAsync(juce::PopupMenu::Options().withParentComponent(this));
}

template <typename Content>
EditorBase<Content>::EditorBase(typename Content::UICommunicationBundle &u,
                                const std::string &pluginName, const std::string &pluginId)
    : uic(u), pluginName(pluginName), pluginId(pluginId)
{
    container = std::make_unique<Background<Content>>(pluginName, pluginId, *this);
    addAndMakeVisible(*container);
}

template <typename Content>
juce::Typeface::Ptr EditorBase<Content>::loadFont(const std::string &path)
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

template <typename Content> void Background<Content>::loadsave(bool doSave)
{
    using FromUI = typename Content::UICommunicationBundle::UIToSynth_Queue_t::value_type;

    auto dp = eb.uic.getDocumentsPath();
    if (doSave)
    {
        fileChooser =
            std::make_unique<juce::FileChooser>("Save Patch", juce::File(dp.u8string()), "*.cndx");

        auto folderChooserFlags =
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(folderChooserFlags, [this](auto &chooser) {
            if (chooser.getResults().size())
            {
                auto file{chooser.getResult()};

                std::string s = file.getFullPathName().toStdString();
                FromUI sv;
                sv.type = FromUI::MType::SAVE_PATCH;
                sv.extended.strPointer = (char *)malloc(s.size() + 1);
                strncpy(sv.extended.strPointer, s.c_str(), s.size() + 1);
                this->eb.uic.fromUiQ.push(sv);
            }
        });
    }
    else
    {
        fileChooser =
            std::make_unique<juce::FileChooser>("Load Patch", juce::File(dp.u8string()), "*.cndx");

        auto folderChooserFlags =
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(folderChooserFlags, [this](auto &chooser) {
            if (chooser.getResults().size())
            {
                auto file{chooser.getResult()};

                std::string s = file.getFullPathName().toStdString();
                FromUI sv;
                sv.type = FromUI::MType::LOAD_PATCH;
                sv.extended.strPointer = (char *)malloc(s.size() + 1);
                strncpy(sv.extended.strPointer, s.c_str(), s.size() + 1);
                this->eb.uic.fromUiQ.push(sv);
            }
        });
    }
}

} // namespace sst::conduit::shared
#endif // CONDUIT_EDITOR_BASE_H
