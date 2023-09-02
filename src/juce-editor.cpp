
#include "clap-saw-demo.h"
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
#define TRACE std::cout << __FILE__ << ":" << __LINE__ << " " << __func__ << std::endl;
bool ClapSawDemo::guiAdjustSize(uint32_t *w, uint32_t *h) noexcept  { return true; }

bool ClapSawDemo::guiSetSize(uint32_t width, uint32_t height) noexcept 
{
    TRACE;
    editor->setSize(static_cast<int>(width), static_cast<int>(height));
    return true;
}

bool ClapSawDemo::guiIsApiSupported(const char *api, bool isFloating) noexcept 
{
    TRACE;
    if (isFloating)
        return false;

    if (strcmp(api, CLAP_WINDOW_API_WIN32) == 0 || strcmp(api, CLAP_WINDOW_API_COCOA) == 0 ||
        strcmp(api, CLAP_WINDOW_API_X11) == 0)
        return true;

    return false;
}

bool ClapSawDemo::guiCreate(const char *api, bool isFloating) noexcept 
{
    guiInitializer = std::make_unique<juce::ScopedJuceInitialiser_GUI>();

    TRACE;
    juce::ignoreUnused(api);

    // Should never happen
    if (isFloating)
        return false;

    TRACE;
    const juce::MessageManagerLock mmLock;
    editor = std::make_unique<Jomp>(*this);

    TRACE;

    if (editor == nullptr)
        return false;

    TRACE;
    if (editor != nullptr)
    {
        // FIXME : add lsitener
        // editor->addComponentListener(this);
    }
    else
    {
        // if hasEditor() returns true then createEditorIfNeeded has to return a valid editor
        jassertfalse;
    }

    TRACE;

    return editor != nullptr;
}

void ClapSawDemo::guiDestroy() noexcept
{
    TRACE;
    guiParentAttached = false;
    editor.reset(nullptr);
}

bool ClapSawDemo::guiSetParent(const clap_window *window) noexcept 
{
    TRACE;
    guiParentAttached = true;
#if JUCE_MAC
    extern bool guiCocoaAttach(const clap_window *, juce::Component *);
    return guiCocoaAttach(window, editor.get());
#elif JUCE_LINUX
    return guiX11Attach(nullptr, window->x11);
#elif JUCE_WINDOWS
    return guiWin32Attach(window->win32);
#else
    guiParentAttached = false;
    return false;
#endif
}

// Show doesn't really exist in JUCE per se. If there's an editor and its attached
// we are good.
bool ClapSawDemo::guiShow() noexcept 
{
    TRACE;
#if JUCE_MAC || JUCE_LINUX || JUCE_WINDOWS
    if (editor)
    {
        return guiParentAttached;
    }
#endif
    return false;
}

bool ClapSawDemo::guiGetSize(uint32_t *width, uint32_t *height) noexcept 
{
    TRACE;
    const juce::MessageManagerLock mmLock;
    if (editor)
    {
        auto b = editor->getBounds();
        *width = (uint32_t)b.getWidth();
        *height = (uint32_t)b.getHeight();
        return true;
    }
    else
    {
        *width = 1000;
        *height = 800;
    }
    return false;
}
}