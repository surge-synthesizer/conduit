//
// Created by Paul Walker on 9/6/23.
//

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
static_assert(std::is_same_v<csh::plugHelper_t,chlp::Plugin<csh::misLevel, csh::checkLevel>>);