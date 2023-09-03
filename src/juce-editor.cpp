
#include "clap-saw-juicy.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "sst/jucegui/style/StyleSheet.h"
#include "sst/jucegui/components/NamedPanel.h"
#include "sst/jucegui/components/WindowPanel.h"


namespace jcmp = sst::jucegui::components;

struct Jomp : public jcmp::WindowPanel
{
    sst::clap_juicy::ClapJuicy &csd;

    struct IdleTimer : juce::Timer
    {
        Jomp &jomp;
        IdleTimer(Jomp &j) : jomp(j) {}
        void timerCallback() override { jomp.onIdle(); }
    };
    std::unique_ptr<IdleTimer> idleTimer;

    Jomp(sst::clap_juicy::ClapJuicy &p) : csd(p) {
        sst::jucegui::style::StyleSheet::initializeStyleSheets([]() {});
        const auto &base = sst::jucegui::style::StyleSheet::getBuiltInStyleSheet(sst::jucegui::style::StyleSheet::DARK);
        base->setColour(jcmp::WindowPanel::Styles::styleClass, jcmp::WindowPanel::Styles::backgroundgradstart, juce::Colour(60,60,70));
        base->setColour(jcmp::WindowPanel::Styles::styleClass, jcmp::WindowPanel::Styles::backgroundgradend, juce::Colour(20,20,30));
        base->setColour(jcmp::BaseStyles::styleClass, jcmp::BaseStyles::regionBorder, juce::Colour(90,90,100));
        setStyle(base);


        morphPanel = std::make_unique<jcmp::NamedPanel>("Morph");
        addAndMakeVisible(*morphPanel);

        tabPanel = std::make_unique<jcmp::NamedPanel>("Tabs");
        tabPanel->isTabbed = true;
        tabPanel->tabNames = {"Scale 1", "Scale 2", "Scale 3", "Scale 4"};
        tabPanel->resetTabState();
        addAndMakeVisible(*tabPanel);
#if 0
        unisonSpread = std::make_unique<juce::Slider>("Unison");
        unisonSpread->setRange(0, 100);
        unisonSpread->setSliderStyle(juce::Slider::SliderStyle::LinearVertical);

        unisonSpread->onDragStart = [w = juce::Component::SafePointer(this)]()
        {
            w->csd.fromUiQ.try_enqueue({sst::clap_juicy::ClapJuicy::FromUI::MType::BEGIN_EDIT,
                                        sst::clap_juicy::ClapJuicy::paramIds::pmUnisonSpread,
            1});
            std::cout << "onDragStart" << std::endl;
        };
        unisonSpread->onDragEnd = [w = juce::Component::SafePointer(this)]()
        {
            w->csd.fromUiQ.try_enqueue({sst::clap_juicy::ClapJuicy::FromUI::MType::END_EDIT,
                                        sst::clap_juicy::ClapJuicy::paramIds::pmUnisonSpread,
                                        1});
            std::cout << "onDragEnd" << std::endl;
        };
        unisonSpread->onValueChange = [w = juce::Component::SafePointer(this)]()
        {
            std::cout << "onValueChange " << w->unisonSpread->getValue() << std::endl;
            w->csd.fromUiQ.try_enqueue({sst::clap_juicy::ClapJuicy::FromUI::MType::ADJUST_VALUE,
                                        sst::clap_juicy::ClapJuicy::paramIds::pmUnisonSpread,
                                        w->unisonSpread->getValue()});
        };
        addAndMakeVisible(*unisonSpread);
#endif

        setSize(500, 400);

        idleTimer = std::make_unique<IdleTimer>(*this);
        idleTimer->startTimerHz(60);
    }

    ~Jomp()
    {
        idleTimer->stopTimer();
    }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized() override
    {
        auto mpWidth = 250;
        morphPanel->setBounds(getLocalBounds().withWidth(mpWidth));
        tabPanel->setBounds(getLocalBounds().withTrimmedLeft(mpWidth));
    }

    std::unique_ptr<jcmp::NamedPanel> tabPanel;
    std::unique_ptr<jcmp::NamedPanel> morphPanel;

    void onIdle()
    {
        sst::clap_juicy::ClapJuicy::ToUI r;
        while (csd.toUiQ.try_dequeue(r))
        {
            if (r.type == sst::clap_juicy::ClapJuicy::ToUI::MType::PARAM_VALUE)
            {
                if (r.id == sst::clap_juicy::ClapJuicy::paramIds::pmUnisonSpread)
                {
                    // unisonSpread->setValue(r.value, juce::dontSendNotification);
                }
            }
            else
            {
                std::cout << "Ignored message of type " << r.type << std::endl;
            }
        }
    }
};

namespace sst::clap_juicy
{
std::unique_ptr<juce::Component> ClapJuicy::createEditor()
{
    refreshUIValues = true;
    return std::make_unique<Jomp>(*this);
}
}