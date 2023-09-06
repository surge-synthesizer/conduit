//
// Created by Paul Walker on 9/6/23.
//

#ifndef CONDUIT_CLAP_BASE_CLASS_H
#define CONDUIT_CLAP_BASE_CLASS_H

#include <clap/helpers/plugin.hh>

namespace sst::conduit::shared
{
static constexpr clap::helpers::MisbehaviourHandler misLevel = clap::helpers::MisbehaviourHandler::Terminate;
static constexpr clap::helpers::CheckingLevel checkLevel = clap::helpers::CheckingLevel::Maximal;

using plugHelper_t = clap::helpers::Plugin<misLevel, checkLevel>;

template<typename T>
struct ClapBaseClass : public plugHelper_t
{
    ClapBaseClass(const clap_plugin_descriptor *desc, const clap_host *host) : plugHelper_t(desc, host) {}
};
}

#endif // CONDUIT_CLAP_BASE_CLASS_H
