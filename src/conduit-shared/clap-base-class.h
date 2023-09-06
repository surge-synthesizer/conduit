//
// Created by Paul Walker on 9/6/23.
//

#ifndef CONDUIT_CLAP_BASE_CLASS_H
#define CONDUIT_CLAP_BASE_CLASS_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <cassert>

#include <clap/helpers/plugin.hh>

#include <sst/basic-blocks/params/ParamMetadata.h>
#include <sst/clap_juce_shim/clap_juce_shim.h>

namespace sst::conduit::shared
{
static constexpr clap::helpers::MisbehaviourHandler misLevel = clap::helpers::MisbehaviourHandler::Terminate;
static constexpr clap::helpers::CheckingLevel checkLevel = clap::helpers::CheckingLevel::Maximal;

using plugHelper_t = clap::helpers::Plugin<misLevel, checkLevel>;

template<typename T>
struct ClapBaseClass : public plugHelper_t
{
    ClapBaseClass(const clap_plugin_descriptor *desc, const clap_host *host) : plugHelper_t(desc, host) {}

    using ParamDesc = sst::basic_blocks::params::ParamMetaData;
    std::vector<ParamDesc> paramDescriptions;
    std::unordered_map<uint32_t, ParamDesc> paramDescriptionMap;

    void configureParams()
    {
        assert(paramDescriptions.size() == T::nParams);
        paramDescriptionMap.clear();
        for (const auto &pd : paramDescriptions)
        {
            // If you hit this assert you have a duplicate param id
            assert(paramDescriptionMap.find(pd.id) == paramDescriptionMap.end());
            paramDescriptionMap[pd.id] = pd;
        }
        assert(paramDescriptionMap.size() == T::nParams);
    }

    bool implementsParams() const noexcept override { return true; }
    bool isValidParamId(clap_id paramId) const noexcept override
    {
        return paramDescriptionMap.find(paramId) != paramDescriptionMap.end();
    }
    uint32_t paramsCount() const noexcept override { return T::nParams; }
    bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept override
    {
        if (paramIndex >= T::nParams)
            return false;

        const auto &pd = paramDescriptions[paramIndex];

        pd.template toClapParamInfo<CLAP_NAME_SIZE>(info);
        return true;
    }

    bool paramsValueToText(clap_id paramId, double value, char *display,
                           uint32_t size) noexcept override
    {
        auto pos = paramDescriptionMap.find(paramId);
        if (pos == paramDescriptionMap.end())
            return false;

        const auto &pd = pos->second;
        auto sValue = pd.valueToString(value);

        if (sValue.has_value())
        {
            strncpy(display, sValue->c_str(), size);
            return true;
        }
        return false;
    }

    bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept override
    {
        auto pos = paramDescriptionMap.find(paramId);
        if (pos == paramDescriptionMap.end())
            return false;

        const auto &pd = pos->second;

        std::string emsg;
        auto res = pd.valueFromString(display, emsg);
        if (res.has_value())
        {
            *value = *res;
            return true;
        }
        return false;
    }

    bool implementsGui() const noexcept override { return clapJuceShim != nullptr; }
    std::unique_ptr<sst::clap_juce_shim::ClapJuceShim> clapJuceShim;
    ADD_SHIM_IMPLEMENTATION(clapJuceShim);
    ADD_SHIM_LINUX_TIMER(clapJuceShim);
};
}

#endif // CONDUIT_CLAP_BASE_CLASS_H
