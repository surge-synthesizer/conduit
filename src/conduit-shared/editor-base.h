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
#include "debug-helpers.h"

namespace sst::conduit::shared
{
struct Background;
struct EditorBase : juce::Component
{
    EditorBase(const std::string &pluginName, const std::string &pluginId);
    ~EditorBase();

    void setContentComponent(std::unique_ptr<juce::Component> c);
    void resized() override;

    std::unique_ptr<Background> container;
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
            : editor(ed), uic(p), pid(pid)
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
            : editor(ed), uic(p), pid(pid)
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
} // namespace sst::conduit::shared
#endif // CONDUIT_EDITOR_BASE_H
