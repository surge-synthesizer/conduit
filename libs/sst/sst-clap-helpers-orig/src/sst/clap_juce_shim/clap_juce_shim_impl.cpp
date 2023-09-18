#include <iostream>
#include "sst/clap_juce_shim/clap_juce_shim.h"

#define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
#include <juce_gui_basics/juce_gui_basics.h>


#if JUCE_LINUX
#include <juce_audio_plugin_client/detail/juce_LinuxMessageThread.h>
#endif

#include <memory>

namespace sst::clap_juce_shim
{
namespace details
{
struct Implementor
{
    std::unique_ptr<juce::Component> editor{nullptr};
    std::unique_ptr<juce::ScopedJuceInitialiser_GUI> guiInitializer{nullptr}; // todo deal with lifecycle

    bool guiParentAttached{false};
    void guaranteeSetup()
    {
        if (!guiInitializer)
        {
            guiInitializer = std::make_unique<juce::ScopedJuceInitialiser_GUI>();
        }
    }
};
} // namespace details
// #define TRACE std::cout << __FILE__ << ":" << __LINE__ << " " << __func__ << std::endl;
#define TRACE ;
ClapJuceShim::ClapJuceShim(EditorProvider *ep)
    : editorProvider(ep)
{
    impl = std::make_unique<details::Implementor>();
}

ClapJuceShim::~ClapJuceShim() {}

bool ClapJuceShim::isEditorAttached() { return impl->guiParentAttached; }
bool ClapJuceShim::guiAdjustSize(uint32_t *w, uint32_t *h) noexcept { return true; }

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
    impl->guaranteeSetup();
#if JUCE_LINUX
    idleTimerId = 0;
    editorProvider->registerOrUnregisterTimer(idleTimerId, 1000/50, true);
#endif

    impl->guiInitializer = std::make_unique<juce::ScopedJuceInitialiser_GUI>();
    juce::ignoreUnused(api);

    // Should never happen
    if (isFloating)
        return false;

    const juce::MessageManagerLock mmLock;
    impl->editor = editorProvider->createEditor();
    return impl->editor != nullptr;
}

void ClapJuceShim::guiDestroy() noexcept
{
    TRACE;
#if JUCE_LINUX
    editorProvider->registerOrUnregisterTimer(idleTimerId, 0, false);
#endif

    impl->guiParentAttached = false;
    impl->editor.reset(nullptr);
}

bool ClapJuceShim::guiSetParent(const clap_window *window) noexcept
{
    TRACE;
    impl->guiParentAttached = true;
#if JUCE_MAC
    extern bool guiCocoaAttach(const clap_window *, juce::Component *);
    auto res = guiCocoaAttach(window, impl->editor.get());
    impl->editor->repaint();
    return res;
#elif JUCE_LINUX
    const juce::MessageManagerLock mmLock;
    impl->editor->setVisible(false);
    impl->editor->addToDesktop(0, (void *)window->x11);
    auto *display = juce::XWindowSystem::getInstance()->getDisplay();
    juce::X11Symbols::getInstance()->xReparentWindow(
        display, (Window)impl->editor->getWindowHandle(), window->x11, 0, 0);
    impl->editor->setVisible(true);
    return true;

    return false;
#elif JUCE_WINDOWS
    impl->editor->setVisible(false);
    impl->editor->setOpaque(true);
    impl->editor->setTopLeftPosition(0, 0);
    impl->editor->addToDesktop(0, (void *)window->win32);
    impl->editor->setVisible(true);
    return true;
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

bool ClapJuceShim::guiSetScale(double scale) noexcept { return true; }

#if JUCE_LINUX
void ClapJuceShim::onTimer(clap_id timerId) noexcept {
    if (timerId != idleTimerId)
        return;

    juce::ScopedJuceInitialiser_GUI libraryInitialiser;
    const juce::MessageManagerLock mmLock;

    while (juce::detail::dispatchNextMessageOnSystemQueue(true))
    {
    }
}
#endif

} // namespace sst::clap_juce_shim
