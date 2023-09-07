/*
 * Conduit - a series of demonstration and fun plugins
 *
 * Copyright 2023 Paul Walker and authors in github
 *
 * This file you are viewing now is released under the
 * MIT license, but the assembled program which results
 * from compiling it has GPL3 dependencies, so the total
 * program is a GPL3 program. More details to come.
 *
 * Basically before I give this to folks, document this bit and
 * replace these headers
 *
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
