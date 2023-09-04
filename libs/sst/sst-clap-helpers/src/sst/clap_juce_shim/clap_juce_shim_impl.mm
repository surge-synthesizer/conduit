//
// Created by Paul Walker on 9/2/23.
//

#include <Cocoa/Cocoa.h>
#include <juce_core/juce_core.h>
#include "clap/ext/gui.h"
#include <juce_gui_basics/juce_gui_basics.h>


namespace sst::clap_juce_shim
{
bool guiCocoaAttach(const clap_window_t *window, juce::Component *comp)
{
    auto nsv = (NSView *)window->cocoa;
    @autoreleasepool
    {
        NSView* container = [(NSView*) nsv retain];

        const auto defaultFlags =  0;
        comp->addToDesktop (defaultFlags, container);
        comp->setVisible (true);
        comp->toFront (false);

        [[nsv window] setAcceptsMouseMovedEvents: YES];
    }
    return true;
}
}