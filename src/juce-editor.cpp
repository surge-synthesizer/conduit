
#include "clap-saw-juicy.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

struct Jomp : public juce::Component
{
    sst::clap_saw_demo::ClapSawDemo &csd;

    struct IdleTimer : juce::Timer
    {
        Jomp &jomp;
        IdleTimer(Jomp &j) : jomp(j) {}

        void timerCallback() override { jomp.onIdle(); }
    };
    std::unique_ptr<IdleTimer> idleTimer;

    Jomp(sst::clap_saw_demo::ClapSawDemo &p) : csd(p) {
        unisonSpread = std::make_unique<juce::Slider>("Unison");
        unisonSpread->setRange(0, 100);
        unisonSpread->setSliderStyle(juce::Slider::SliderStyle::LinearVertical);

        unisonSpread->onDragStart = [w = juce::Component::SafePointer(this)]()
        {
            w->csd.fromUiQ.try_enqueue({sst::clap_saw_demo::ClapSawDemo::FromUI::MType::BEGIN_EDIT,
                                        sst::clap_saw_demo::ClapSawDemo::paramIds::pmUnisonSpread,
            1});
            std::cout << "onDragStart" << std::endl;
        };
        unisonSpread->onDragEnd = [w = juce::Component::SafePointer(this)]()
        {
            w->csd.fromUiQ.try_enqueue({sst::clap_saw_demo::ClapSawDemo::FromUI::MType::END_EDIT,
                                        sst::clap_saw_demo::ClapSawDemo::paramIds::pmUnisonSpread,
                                        1});
            std::cout << "onDragEnd" << std::endl;
        };
        unisonSpread->onValueChange = [w = juce::Component::SafePointer(this)]()
        {
            std::cout << "onValueChange " << w->unisonSpread->getValue() << std::endl;
            w->csd.fromUiQ.try_enqueue({sst::clap_saw_demo::ClapSawDemo::FromUI::MType::ADJUST_VALUE,
                                        sst::clap_saw_demo::ClapSawDemo::paramIds::pmUnisonSpread,
                                        w->unisonSpread->getValue()});
        };
        addAndMakeVisible(*unisonSpread);

        setSize(500, 400);

        idleTimer = std::make_unique<IdleTimer>(*this);
        idleTimer->startTimerHz(60);
    }

    ~Jomp()
    {
        idleTimer->stopTimer();
    }

    std::unique_ptr<juce::Slider> unisonSpread;

    void resized()
    {
        if (unisonSpread)
        unisonSpread->setBounds(juce::Rectangle<int>(10,10,40,300));
    }
    void paint(juce::Graphics &g) {
        g.fillAll(juce::Colours::red);
        g.setColour(juce::Colours::black);
        g.setFont(30);
        g.drawText("Hi from JUCE", getLocalBounds(), juce::Justification::centred);
    }

    void onIdle()
    {
        sst::clap_saw_demo::ClapSawDemo::ToUI r;
        while (csd.toUiQ.try_dequeue(r))
        {
            if (r.type == sst::clap_saw_demo::ClapSawDemo::ToUI::MType::PARAM_VALUE)
            {
                std::cout << "Update on " << r.id << std::endl;
                if (r.id == sst::clap_saw_demo::ClapSawDemo::paramIds::pmUnisonSpread)
                {
                    unisonSpread->setValue(r.value, juce::dontSendNotification);
                }
            }
        }
    }
};

namespace sst::clap_saw_demo
{
std::unique_ptr<juce::Component> ClapSawDemo::createEditor()
{
    return std::make_unique<Jomp>(*this);
}
}