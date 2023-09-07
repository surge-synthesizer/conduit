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

#include "clap-base-class.h"

// Eject the core symbols for the plugin
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>
#include <clap/helpers/host-proxy.hh>
#include <clap/helpers/host-proxy.hxx>

namespace csh = sst::conduit::shared;
namespace chlp = clap::helpers;
template struct chlp::Plugin<csh::misLevel, csh::checkLevel>;
template struct chlp::HostProxy<csh::misLevel, csh::checkLevel>;
static_assert(std::is_same_v<csh::plugHelper_t, chlp::Plugin<csh::misLevel, csh::checkLevel>>);