/*
 * ConduitPolysynth
 * https://github.com/surge-synthesizer/clap-saw-demo
 *
 * Copyright 2022 Paul Walker and others as listed in the git history
 *
 * Released under the MIT License. See LICENSE.md for full text.
 */

#ifndef CLAP_SAW_DEMO_DEBUG_HELPERS_H
#define CLAP_SAW_DEMO_DEBUG_HELPERS_H

// These are just some macros I put in to trace certain lifecycle and value moments to stdout
#include <iostream>

namespace sst::conduit::shared::details
{
inline std::string fixFile(std::string s)
{
    // TODO this sucks also
    auto sp = s.find(CONDUIT_SOURCE_DIR);

    if (sp == 0)
    {
        s = s.substr(sp + strlen(CONDUIT_SOURCE_DIR) + 1);
    }
    return s;
}
}

#define CNDOUT                                                                                     \
    std::cout << "[conduit] " << sst::conduit::shared::details::fixFile(__FILE__) << ":" << __LINE__ << " (" << __func__ << ") : "
#define CNDVAR(x) " (" << #x << "=" << x << ") "

#endif // CLAP_SAW_DEMO_DEBUG_HELPERS_H
