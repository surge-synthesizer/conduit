
#include "conduit-polysynth.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "sst/jucegui/style/StyleSheet.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"
#include "sst/jucegui/components/Knob.h"
#include "sst/jucegui/data/Continuous.h"


namespace sst::conduit_polysynth::editor
{
namespace jcmp = sst::jucegui::components;
namespace jdat = sst::jucegui::data;

using cps_t = sst::conduit_polysynth::ConduitPolysynth;
using uicomm_t = cps_t::UICommunicationBundle;

struct DataToQueueParam : jdat::ContinunousModulatable
{
    // TODO - this plus begin/end
    // FIX - this can be queues only
    uicomm_t &uic;
    cps_t::paramIds pid;
    cps_t::ParamDesc pDesc{};

    DataToQueueParam(uicomm_t &p,
                     cps_t::paramIds pid)
        : uic(p), pid(pid)
    {
        pDesc = uic.getParameterDescription(pid);
    }
    std::string getLabel() const override { return pDesc.name; }

    float f{0.f};
    float getValue() const override { return f; }
    void setValueFromGUI(const float &fi) override
    {
        f = fi;
        uic.fromUiQ.try_enqueue(
            {sst::conduit_polysynth::ConduitPolysynth::FromUI::MType::ADJUST_VALUE,
             sst::conduit_polysynth::ConduitPolysynth::paramIds::pmUnisonSpread, f});
    }
    void setValueFromModel(const float &fi) override { f = fi; }
    float getDefaultValue() const override { return pDesc.defaultVal; }
    float getMin() const override { return pDesc.minVal; }
    float getMax() const override { return pDesc.maxVal; }

    float getModulationValuePM1() const override { return 0; }
    void setModulationValuePM1(const float &f) override {}
    bool isModulationBipolar() const override { return false; }
};

struct ConduitPolysynthEditor;

struct OscPanel : public juce::Component
{
    uicomm_t &uic;

    OscPanel(uicomm_t &p, ConduitPolysynthEditor &e) : uic(p)
    {
        oscUnisonSpread = std::make_unique<jcmp::Knob>();
        addAndMakeVisible(*oscUnisonSpread);

        oscUnisonSource = std::make_unique<DataToQueueParam>(
            uic, sst::conduit_polysynth::ConduitPolysynth::paramIds::pmUnisonSpread);
        oscUnisonSpread->setSource(oscUnisonSource.get());

        oscUnisonSpread->onBeginEdit = [w = juce::Component::SafePointer(this)]() {
            w->uic.fromUiQ.try_enqueue(
                {sst::conduit_polysynth::ConduitPolysynth::FromUI::MType::BEGIN_EDIT,
                 sst::conduit_polysynth::ConduitPolysynth::paramIds::pmUnisonSpread, 1});
            std::cout << "onDragStart" << std::endl;
        };
        oscUnisonSpread->onEndEdit = [w = juce::Component::SafePointer(this)]() {
            w->uic.fromUiQ.try_enqueue(
                {sst::conduit_polysynth::ConduitPolysynth::FromUI::MType::END_EDIT,
                 sst::conduit_polysynth::ConduitPolysynth::paramIds::pmUnisonSpread, 1});
            std::cout << "onDragEnd" << std::endl;
        };

        registerDataSources(e);
    }

    ~OscPanel() { oscUnisonSpread->setSource(nullptr); }

    void resized() override { oscUnisonSpread->setBounds({10, 10, 60, 60}); }

    void registerDataSources(ConduitPolysynthEditor &e);
    std::unique_ptr<jcmp::Knob> oscUnisonSpread;
    std::unique_ptr<DataToQueueParam> oscUnisonSource;
};

struct ConduitPolysynthEditor : public jcmp::WindowPanel
{
    uicomm_t &uic;

    struct IdleTimer : juce::Timer
    {
        ConduitPolysynthEditor &jomp;
        IdleTimer(ConduitPolysynthEditor &j) : jomp(j) {}
        void timerCallback() override { jomp.onIdle(); }
    };
    std::unique_ptr<IdleTimer> idleTimer;

    ConduitPolysynthEditor(uicomm_t &p) : uic(p)
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
        setStyle(base);

        oscPanel = std::make_unique<jcmp::NamedPanel>("Oscillator");
        addAndMakeVisible(*oscPanel);

        auto oct = std::make_unique<OscPanel>(uic, *this);
        oscPanel->setContentAreaComponent(std::move(oct));

        vcfPanel = std::make_unique<jcmp::NamedPanel>("Filter");
        addAndMakeVisible(*vcfPanel);

        setSize(500, 400);

        idleTimer = std::make_unique<IdleTimer>(*this);
        idleTimer->startTimerHz(60);
    }

    ~ConduitPolysynthEditor() { idleTimer->stopTimer(); }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override
    {
        auto mpWidth = 250;
        oscPanel->setBounds(getLocalBounds().withWidth(mpWidth));
        vcfPanel->setBounds(getLocalBounds().withTrimmedLeft(mpWidth));
    }

    std::unique_ptr<jcmp::NamedPanel> vcfPanel;
    std::unique_ptr<jcmp::NamedPanel> oscPanel;

    std::unordered_map<uint32_t, std::pair<juce::Component *, jdat::ContinunousModulatable *>>
        dataTargets;

    void onIdle()
    {
        sst::conduit_polysynth::ConduitPolysynth::ToUI r;
        while (uic.toUiQ.try_dequeue(r))
        {
            if (r.type == sst::conduit_polysynth::ConduitPolysynth::ToUI::MType::PARAM_VALUE)
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

void OscPanel::registerDataSources(ConduitPolysynthEditor &e)
{
    // FIXME clean this up on dtor
    e.dataTargets[sst::conduit_polysynth::ConduitPolysynth::pmUnisonSpread] = {
        oscUnisonSpread.get(), oscUnisonSource.get()};
}
} // namespace sst::conduit_polysynth::editor
namespace sst::conduit_polysynth
{
std::unique_ptr<juce::Component> ConduitPolysynth::createEditor()
{
    refreshUIValues = true;
    return std::make_unique<sst::conduit_polysynth::editor::ConduitPolysynthEditor>(uiComms);
}
}