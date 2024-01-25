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

#include "clap-base-class.h"

// Eject the core symbols for the plugin
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>
#include <clap/helpers/host-proxy.hh>
#include <clap/helpers/host-proxy.hxx>

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(conduit_resources);

namespace csh = sst::conduit::shared;
namespace chlp = clap::helpers;
template class chlp::Plugin<csh::misLevel, csh::checkLevel>;
template class chlp::HostProxy<csh::misLevel, csh::checkLevel>;
static_assert(std::is_same_v<csh::plugHelper_t, chlp::Plugin<csh::misLevel, csh::checkLevel>>);