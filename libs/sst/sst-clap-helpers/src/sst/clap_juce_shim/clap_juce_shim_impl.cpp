#include <iostream>
#include "sst/clap_juce_shim/clap_juce_shim.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace sst::clap_juce_shim
{
namespace details
{
struct Implementor
{
    std::unique_ptr<juce::Component> editor{nullptr};
    std::unique_ptr<juce::ScopedJuceInitialiser_GUI> guiInitializer; // todo deal with lifecycle
    bool guiParentAttached{false};
};
}
// #define TRACE std::cout << __FILE__ << ":" << __LINE__ << " " << __func__ << std::endl;
#define TRACE ;
ClapJuceShim::ClapJuceShim(std::function<std::unique_ptr<juce::Component>()> ce) : createEditor(ce) {
    impl = std::make_unique<details::Implementor>();
}

ClapJuceShim::~ClapJuceShim()
{}

bool ClapJuceShim::isEditorAttached()
{
    return impl->guiParentAttached;
}
bool ClapJuceShim::guiAdjustSize(uint32_t *w, uint32_t *h) noexcept  { return true; }

bool ClapJuceShim::guiSetSize(uint32_t width, uint32_t height) noexcept
{
    TRACE;
    impl->editor->setSize(static_cast<int>(width), static_cast<int>(height));
    return true;
}

bool ClapJuceShim::guiIsApiSupported(const char *api, bool isFloating) noexcept
{
    TRACE;
    if (isFloating)
        return false;

    if (strcmp(api, CLAP_WINDOW_API_WIN32) == 0 || strcmp(api, CLAP_WINDOW_API_COCOA) == 0 ||
        strcmp(api, CLAP_WINDOW_API_X11) == 0)
        return true;

    return false;
}

bool ClapJuceShim::guiCreate(const char *api, bool isFloating) noexcept
{
    TRACE;
    impl->guiInitializer = std::make_unique<juce::ScopedJuceInitialiser_GUI>();
    juce::ignoreUnused(api);

    // Should never happen
    if (isFloating)
        return false;

    const juce::MessageManagerLock mmLock;
    impl->editor = createEditor();
    return impl->editor != nullptr;
}

void ClapJuceShim::guiDestroy() noexcept
{
    TRACE;
    impl->guiParentAttached = false;
    impl->editor.reset(nullptr);
}

bool ClapJuceShim::guiSetParent(const clap_window *window) noexcept
{
    TRACE;
    impl->guiParentAttached = true;
#if JUCE_MAC
    extern bool guiCocoaAttach(const clap_window *, juce::Component *);
    return guiCocoaAttach(window, impl->editor.get());
#elif JUCE_LINUX
    return guiX11Attach(nullptr, window->x11);
#elif JUCE_WINDOWS
    return guiWin32Attach(window->win32);
#else
    impl->guiParentAttached = false;
    return false;
#endif
}

// Show doesn't really exist in JUCE per se. If there's an impl->editor and its attached
// we are good.
bool ClapJuceShim::guiShow() noexcept
{
    TRACE;
#if JUCE_MAC || JUCE_LINUX || JUCE_WINDOWS
    if (impl->editor)
    {
        return impl->guiParentAttached;
    }
#endif
    return false;
}

bool ClapJuceShim::guiGetSize(uint32_t *width, uint32_t *height) noexcept
{
    TRACE;
    const juce::MessageManagerLock mmLock;
    if (impl->editor)
    {
        auto b = impl->editor->getBounds();
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

bool ClapJuceShim::guiSetScale(double scale) noexcept
{
    return true;
}
}
