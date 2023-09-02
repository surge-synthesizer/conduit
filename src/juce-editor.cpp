
#include "clap-saw-demo.h"
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

struct Jomp : public juce::Component
{
    Jomp() { setSize(500, 400); }
    void paint(juce::Graphics &g) { g.fillAll(juce::Colours::red); }
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
    TRACE;
    juce::ignoreUnused(api);

    // Should never happen
    if (isFloating)
        return false;

    TRACE;
    const juce::MessageManagerLock mmLock;
    editor = std::make_unique<Jomp>();

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