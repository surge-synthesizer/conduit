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

#ifndef CONDUIT_SRC_CONDUIT_SHARED_EDITOR_BASE_H
#define CONDUIT_SRC_CONDUIT_SHARED_EDITOR_BASE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/data/Continuous.h"
#include "sst/jucegui/components/ContinuousParamEditor.h"
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

struct TooltipSupport
{
    void openTooltip(int p) { CNDOUT << "open tooltip for " << p << std::endl; }
    void closeTooltip(int p) { CNDOUT << "close tooltip for " << p << std::endl; }
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

        ToUI r;
        while (uic.toUiQ.try_dequeue(r))
        {
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
#if 0
                    auto pd = uic.getParameterDescription(r.id);
                    CNDOUT << "WARNING - no UI element registered for pid=" << r.id << " nm=" << pd.name
                           << std::endl;
#endif
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

    struct DataToQueueParam : sst::jucegui::data::ContinunousModulatable
    {
        // TODO - this plus begin/end
        // FIX - this can be queues only
        typename T::UICommunicationBundle &uic;
        uint32_t pid;
        typename T::ParamDesc pDesc{};

        DataToQueueParam(typename T::UICommunicationBundle &p, uint32_t pid) : uic(p), pid(pid)
        {
            pDesc = uic.getParameterDescription(pid);
        }
        std::string getLabel() const override { return pDesc.name; }

        float f{0.f};
        float getValue() const override { return f; }
        void setValueFromGUI(const float &fi) override
        {
            using FromUI = typename T::UICommunicationBundle::UIToSynth_Queue_t::value_type;

            f = fi;
            uic.fromUiQ.try_enqueue({FromUI::MType::ADJUST_VALUE, pid, f});
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

    void attachContinuousToParam(sst::jucegui::components::ContinuousParamEditor *comp,
                                 uint32_t pid)
    {
        using FromUI = typename T::UICommunicationBundle::UIToSynth_Queue_t::value_type;
        auto source = std::make_unique<DataToQueueParam>(uic, pid);
        comp->setSource(source.get());

        comp->onBeginEdit = [this, pid]() {
            uic.fromUiQ.try_enqueue({FromUI::MType::BEGIN_EDIT, pid, 1});
            ed.openTooltip(pid);
        };
        comp->onEndEdit = [this, pid]() {
            uic.fromUiQ.try_enqueue({FromUI::MType::END_EDIT, pid, 1});
            ed.closeTooltip(pid);
        };
        comp->onIdleHover = [this, pid]() { ed.openTooltip(pid); };
        comp->onIdleHoverEnd = [this, pid]() { ed.closeTooltip(pid); };

        dataTargets[pid] = {comp, source.get()};
        ownedData[pid] = std::move(source);
    }

    std::unordered_map<uint32_t, std::unique_ptr<DataToQueueParam>> ownedData;
};
} // namespace sst::conduit::shared
#endif // CONDUIT_EDITOR_BASE_H
