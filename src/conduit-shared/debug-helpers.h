/*
 * Conduit - a project highlighting CLAP-first development
 *           and exercising the surge synth team libraries.
 *
 * Copyright 2023-2024 Paul Walker and authors in github
 *
 * This file you are viewing now is released under the
 * MIT license as described in LICENSE.md
 *
 * The assembled program which results from compiling this
 * project has GPL3 dependencies, so if you distribute
 * a binary, the combined work would be a GPL3 product.
 *
 * Roughly, that means you are welcome to copy the code and
 * ideas in the src/ directory, but perhaps not code from elsewhere
 * if you are closed source or non-GPL3. And if you do copy this code
 * you will need to replace some of the dependencies. Please see
 * the discussion in README.md for further information on what this may
 * mean for you.
 */

#ifndef CONDUIT_SRC_CONDUIT_SHARED_DEBUG_HELPERS_H
#define CONDUIT_SRC_CONDUIT_SHARED_DEBUG_HELPERS_H

// These are just some macros I put in to trace certain lifecycle and value moments to stdout
#include <iostream>
#include <cstring>

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
} // namespace sst::conduit::shared::details

#define CNDOUT                                                                                     \
    std::cout << "[conduit] " << sst::conduit::shared::details::fixFile(__FILE__) << ":"           \
              << __LINE__ << " (" << __func__ << ") : "
#define CNDVAR(x) " (" << #x << "=" << x << ") "

#endif // CLAP_SAW_DEMO_DEBUG_HELPERS_H
