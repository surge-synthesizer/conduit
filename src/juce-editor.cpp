
#include "clap-saw-juicy.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "sst/jucegui/style/StyleSheet.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/data/Continuous.h"


namespace jcmp = sst::jucegui::components;
namespace jdat = sst::jucegui::data;

struct DataToQueueParam : jdat::ContinunousModulatable
{
    // TODO - this plus begin/end
    // FIX - this can be queues only
    sst::clap_juicy::ClapJuicy &csd;
    sst::clap_juicy::ClapJuicy::paramIds pid;
    sst::clap_juicy::ClapJuicy::ParamDesc pDesc{};

    DataToQueueParam(sst::clap_juicy::ClapJuicy &p, sst::clap_juicy::ClapJuicy::paramIds pid)
    : csd(p), pid(pid)
    {
        pDesc = csd.paramDescriptionMap[pid];
    }
    std::string getLabel() const override { return pDesc.name; }

    float f{0.f};
    float getValue() const override { return f; }
    void setValueFromGUI(const float &fi) override {
        f = fi;
        csd.fromUiQ.try_enqueue({sst::clap_juicy::ClapJuicy::FromUI::MType::ADJUST_VALUE,
                                    sst::clap_juicy::ClapJuicy::paramIds::pmUnisonSpread,
                                    f});
    }
    void setValueFromModel(const float &fi) override { f = fi; }
    float getDefaultValue() const override { return pDesc.defaultVal; }
    float getMin() const override { return pDesc.minVal; }
    float getMax() const override { return pDesc.maxVal; }


    float getModulationValuePM1() const override { return 0; }
    void setModulationValuePM1(const float &f) override {}
    bool isModulationBipolar() const override { return false; }
};

struct JuicyEditor;

struct JuicyOscPanel : public juce::Component
{
    // FIX - this can be the queues only
    sst::clap_juicy::ClapJuicy &csd;

    JuicyOscPanel(sst::clap_juicy::ClapJuicy &p, JuicyEditor &e) : csd(p) {
        oscUnisonSpread = std::make_unique<jcmp::Knob>();
        addAndMakeVisible(*oscUnisonSpread);

        oscUnisonSource = std::make_unique<DataToQueueParam>(csd,
                                                             sst::clap_juicy::ClapJuicy::paramIds::pmUnisonSpread);
        oscUnisonSpread->setSource(oscUnisonSource.get());

        oscUnisonSpread->onBeginEdit = [w = juce::Component::SafePointer(this)]()
        {
            w->csd.fromUiQ.try_enqueue({sst::clap_juicy::ClapJuicy::FromUI::MType::BEGIN_EDIT,
                                        sst::clap_juicy::ClapJuicy::paramIds::pmUnisonSpread,
                                        1});
            std::cout << "onDragStart" << std::endl;
        };
        oscUnisonSpread->onEndEdit = [w = juce::Component::SafePointer(this)]()
        {
            w->csd.fromUiQ.try_enqueue({sst::clap_juicy::ClapJuicy::FromUI::MType::END_EDIT,
                                        sst::clap_juicy::ClapJuicy::paramIds::pmUnisonSpread,
                                        1});
            std::cout << "onDragEnd" << std::endl;
        };

        registerDataSources(e);
    }

    ~JuicyOscPanel()
    {
        oscUnisonSpread->setSource(nullptr);
    }

    void resized() override {
        oscUnisonSpread->setBounds({10,10,60,60});
    }

    void registerDataSources(JuicyEditor &e);
    std::unique_ptr<jcmp::Knob> oscUnisonSpread;
    std::unique_ptr<DataToQueueParam> oscUnisonSource;
};

struct JuicyEditor : public jcmp::WindowPanel
{
    // FIX - this can be the queues only
    sst::clap_juicy::ClapJuicy &csd;

    struct IdleTimer : juce::Timer
    {
        JuicyEditor &jomp;
        IdleTimer(JuicyEditor &j) : jomp(j) {}
        void timerCallback() override { jomp.onIdle(); }
    };
    std::unique_ptr<IdleTimer> idleTimer;

    JuicyEditor(sst::clap_juicy::ClapJuicy &p) : csd(p) {
        sst::jucegui::style::StyleSheet::initializeStyleSheets([]() {});
        const auto &base = sst::jucegui::style::StyleSheet::getBuiltInStyleSheet(sst::jucegui::style::StyleSheet::DARK);
        base->setColour(jcmp::WindowPanel::Styles::styleClass, jcmp::WindowPanel::Styles::backgroundgradstart, juce::Colour(60,60,70));
        base->setColour(jcmp::WindowPanel::Styles::styleClass, jcmp::WindowPanel::Styles::backgroundgradend, juce::Colour(20,20,30));
        base->setColour(jcmp::BaseStyles::styleClass, jcmp::BaseStyles::regionBorder, juce::Colour(90,90,100));
        setStyle(base);

        oscPanel = std::make_unique<jcmp::NamedPanel>("Oscillator");
        addAndMakeVisible(*oscPanel);

        auto oct = std::make_unique<JuicyOscPanel>(csd, *this);
        oscPanel->setContentAreaComponent(std::move(oct));

        vcfPanel = std::make_unique<jcmp::NamedPanel>("Filter");
        addAndMakeVisible(*vcfPanel);

        setSize(500, 400);

        idleTimer = std::make_unique<IdleTimer>(*this);
        idleTimer->startTimerHz(60);
    }

    ~JuicyEditor()
    {
        idleTimer->stopTimer();
    }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override
    {
        auto mpWidth = 250;
        oscPanel->setBounds(getLocalBounds().withWidth(mpWidth));
        vcfPanel->setBounds(getLocalBounds().withTrimmedLeft(mpWidth));
    }

    std::unique_ptr<jcmp::NamedPanel> vcfPanel;
    std::unique_ptr<jcmp::NamedPanel> oscPanel;

    std::unordered_map<uint32_t,
        std::pair<juce::Component *, jdat::ContinunousModulatable *>> dataTargets;

    void onIdle()
    {
        sst::clap_juicy::ClapJuicy::ToUI r;
        while (csd.toUiQ.try_dequeue(r))
        {
            if (r.type == sst::clap_juicy::ClapJuicy::ToUI::MType::PARAM_VALUE)
            {
                auto p = dataTargets.find(r.id);
                if (p != dataTargets.end())
                {
                    p->second.second->setValueFromModel(r.value);
                    p->second.first->repaint();
                }
            }
            else
            {
                std::cout << "Ignored message of type " << r.type << std::endl;
            }
        }
    }
};

void JuicyOscPanel::registerDataSources(JuicyEditor &e)
{
    // FIXME clean this up on dtor
    e.dataTargets[sst::clap_juicy::ClapJuicy::pmUnisonSpread] =
    {oscUnisonSpread.get(), oscUnisonSource.get()};
}

namespace sst::clap_juicy
{
std::unique_ptr<juce::Component> ClapJuicy::createEditor()
{
    refreshUIValues = true;
    return std::make_unique<JuicyEditor>(*this);
}
}