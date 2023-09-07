//
// Created by Paul Walker on 9/6/23.
//

#ifndef CONDUIT_EDITOR_BASE_H
#define CONDUIT_EDITOR_BASE_H

#include <juce_gui_basics/juce_gui_basics.h>

namespace sst::conduit::shared
{
struct Background;
struct EditorBase : juce::Component
{
    EditorBase();
    ~EditorBase();

    void setContentComponent(std::unique_ptr<juce::Component> c);
    void resized() override;

    std::unique_ptr<Background> container;

    static constexpr int headerSize{35};
};
} // namespace conduit::shared
#endif // CONDUIT_EDITOR_BASE_H
