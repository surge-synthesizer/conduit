//
// Created by Paul Walker on 9/2/23.
//

#include <Cocoa/Cocoa.h>
#include <juce_core/juce_core.h>
#include "clap/ext/gui.h"
#include <juce_gui_basics/juce_gui_basics.h>


namespace sst::clap_saw_demo
{
bool guiCocoaAttach(const clap_window_t *window, juce::Component *comp)
{
    auto parentWindowOrView = window->cocoa;
    JUCE_AUTORELEASEPOOL
    {
        NSView* parentView = [(NSView*) parentWindowOrView retain];

        const auto defaultFlags =  0;
        comp->addToDesktop (defaultFlags, parentView);

        comp->setVisible (true);
        comp->toFront (false);

        [[parentView window] setAcceptsMouseMovedEvents: YES];
    }
    return true;

}
}