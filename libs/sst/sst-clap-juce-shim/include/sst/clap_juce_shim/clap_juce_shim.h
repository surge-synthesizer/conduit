//
// Created by Paul Walker on 9/2/23.
//

#include <functional>
#include <memory>
#include <cstdint>

#include "clap/ext/gui.h"

#ifndef CLAP_SAW_JUICY_CLAP_JUCE_SHIM_H
#define CLAP_SAW_JUICY_CLAP_JUCE_SHIM_H

namespace juce
{
class Component;
}
namespace sst::clap_juce_shim
{
namespace details
{
struct Implementor;
}

struct ClapJuceShim
{
    ClapJuceShim(std::function<std::unique_ptr<juce::Component>()> ce);
    ~ClapJuceShim();

    std::function<std::unique_ptr<juce::Component>()> createEditor;
    std::unique_ptr<details::Implementor> impl;

    bool isEditorAttached();

    bool guiIsApiSupported(const char *api, bool isFloating) noexcept;

    bool guiCreate(const char *api, bool isFloating) noexcept;
    void guiDestroy() noexcept;
    bool guiSetParent(const clap_window *window) noexcept;

    bool guiSetScale(double scale) noexcept;
    bool guiAdjustSize(uint32_t *width, uint32_t *height) noexcept;
    bool guiSetSize(uint32_t width, uint32_t height) noexcept;
    bool guiGetSize(uint32_t *width, uint32_t *height) noexcept;

    bool guiShow() noexcept;
};
} // namespace sst::clap_juce_shim

#define ADD_SHIM_IMPLEMENTATION(clapJuceShim)                                                      \
    bool guiIsApiSupported(const char *api, bool isFloating) noexcept override                     \
    {                                                                                              \
        return clapJuceShim->guiIsApiSupported(api, isFloating);                                   \
    }                                                                                              \
    bool guiCreate(const char *api, bool isFloating) noexcept override                             \
    {                                                                                              \
        return clapJuceShim->guiCreate(api, isFloating);                                           \
    }                                                                                              \
    void guiDestroy() noexcept override { clapJuceShim->guiDestroy(); }                            \
    bool guiSetParent(const clap_window *window) noexcept override                                 \
    {                                                                                              \
        return clapJuceShim->guiSetParent(window);                                                 \
    }                                                                                              \
    bool guiSetScale(double scale) noexcept override { return clapJuceShim->guiSetScale(scale); }  \
    bool guiAdjustSize(uint32_t *width, uint32_t *height) noexcept override                        \
    {                                                                                              \
        return clapJuceShim->guiAdjustSize(width, height);                                         \
    }                                                                                              \
    bool guiSetSize(uint32_t width, uint32_t height) noexcept override                             \
    {                                                                                              \
        return clapJuceShim->guiSetSize(width, height);                                            \
    }                                                                                              \
    bool guiGetSize(uint32_t *width, uint32_t *height) noexcept override                                  \
    {                                                                                              \
        return clapJuceShim->guiGetSize(width, height);                                            \
    }                                                                                              \
    bool guiShow() noexcept override { return clapJuceShim->guiShow(); }
#endif // CLAP_SAW_JUICY_CLAP_JUCE_SHIM_H
