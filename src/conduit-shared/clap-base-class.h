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

#ifndef CONDUIT_SRC_CONDUIT_SHARED_CLAP_BASE_CLASS_H
#define CONDUIT_SRC_CONDUIT_SHARED_CLAP_BASE_CLASS_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <cassert>

#include <readerwriterqueue.h>

#include <tinyxml/tinyxml.h>

#include <clap/helpers/plugin.hh>
#include <clap/ext/state.h>

#include <sst/basic-blocks/params/ParamMetadata.h>
#include <sst/clap_juce_shim/clap_juce_shim.h>
#include "debug-helpers.h"

namespace sst::conduit::shared
{
static constexpr clap::helpers::MisbehaviourHandler misLevel =
    clap::helpers::MisbehaviourHandler::Terminate;
static constexpr clap::helpers::CheckingLevel checkLevel = clap::helpers::CheckingLevel::Maximal;

using plugHelper_t = clap::helpers::Plugin<misLevel, checkLevel>;

struct EmptyPatchExtension
{
    static constexpr bool hasExtension{false};

    /*
     void toXml
    void fromXml
     document this if I use it
     */
};

template <typename T, typename TConfig> struct ClapBaseClass : public plugHelper_t
{
    ClapBaseClass(const clap_plugin_descriptor *desc, const clap_host *host)
        : plugHelper_t(desc, host), uiComms(*this)
    {
    }

    using ParamDesc = sst::basic_blocks::params::ParamMetaData;
    std::vector<ParamDesc> paramDescriptions;
    std::unordered_map<uint32_t, ParamDesc> paramDescriptionMap;

    void configureParams()
    {
        assert(paramDescriptions.size() == TConfig::nParams);
        paramDescriptionMap.clear();
        int patchIdx{0};
        for (const auto &pd : paramDescriptions)
        {
            // If you hit this assert you have a duplicate param id
            assert(paramDescriptionMap.find(pd.id) == paramDescriptionMap.end());
            paramDescriptionMap[pd.id] = pd;
            paramToPatchIndex[pd.id] = patchIdx;
            paramToValue[pd.id] = &(patch.params[patchIdx]);

            patch.params[patchIdx] = pd.defaultVal;

            patchIdx++;
        }
        assert(paramDescriptionMap.size() == TConfig::nParams);
        assert(patchIdx == TConfig::nParams);
    }

    bool implementsParams() const noexcept override { return true; }
    bool isValidParamId(clap_id paramId) const noexcept override
    {
        return paramDescriptionMap.find(paramId) != paramDescriptionMap.end();
    }
    uint32_t paramsCount() const noexcept override { return TConfig::nParams; }
    bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept override
    {
        if (paramIndex >= TConfig::nParams)
            return false;

        const auto &pd = paramDescriptions[paramIndex];

        pd.template toClapParamInfo<CLAP_NAME_SIZE>(info);
        return true;
    }

    bool paramsValue(clap_id paramId, double *value) noexcept override
    {
        *value = *paramToValue[paramId];
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

    struct Patch
    {
        float params[TConfig::nParams];
        typename TConfig::PatchExtension extension;
    } patch;
    std::unordered_map<clap_id, float *> paramToValue;
    std::unordered_map<clap_id, int> paramToPatchIndex;

    void attachParam(clap_id paramId, float *&to)
    {
        auto ptpi = paramToPatchIndex.find(paramId);
        if (ptpi == paramToPatchIndex.end())
        {
            to = nullptr;
        }
        else
        {
            to = &patch.params[ptpi->second];
        }
    }

    static constexpr int streamingVersion{1};
    bool implementsState() const noexcept override { return true; }
    bool stateSave(const clap_ostream *ostream) noexcept override
    {
        TiXmlDocument document;

        TiXmlElement conduit("conduit");
        conduit.SetAttribute("streamingVersion", streamingVersion);
        conduit.SetAttribute("plugin_id", TConfig::getDescription()->id);

        TiXmlElement paramel("params");
        for (const auto &a : paramDescriptions)
        {
            TiXmlElement par("param");
            par.SetAttribute("id", a.id);
            par.SetDoubleAttribute("value", *(paramToValue[a.id]));
            par.SetAttribute("name", a.name); // just to debug;
            paramel.InsertEndChild(par);
        }
        conduit.InsertEndChild(paramel);
        document.InsertEndChild(conduit);

        if constexpr (TConfig::PatchExtension::hasExtension)
        {
            TiXmlElement ext("extension");
            if (!patch.extension.toXml(ext))
                return false;
        }

        std::string xmlS;
        xmlS << document;

        auto c = xmlS.c_str();
        auto s = xmlS.length() + 1; // write the null terminator
        while (s > 0)
        {
            auto r = ostream->write(ostream, c, s);
            if (r < 0)
                return false;
            s -= r;
            c += r;
        }
        return true;
    }
    bool stateLoad(const clap_istream *istream) noexcept override
    {
        static constexpr uint32_t maxSize = 1 << 16, chunkSize = 1 << 8;
        char buffer[maxSize];
        char *bp = &(buffer[0]);
        int64_t rd{0};
        int64_t totalRd{0};

        buffer[0] = 0;
        while ((rd = istream->read(istream, bp, chunkSize)) > 0)
        {
            bp += rd;
            totalRd += rd;
            if (totalRd >= maxSize - chunkSize - 1)
            {
                CNDOUT << "Input byte stream larger than " << maxSize << "; Failing" << std::endl;
                return false;
            }
        }

        if (totalRd < maxSize)
            buffer[totalRd] = 0;

        auto xd = std::string(buffer);

        TiXmlDocument document;
        // I forget how to error check this.
        document.Parse(xd.c_str());

#define TINYXML_SAFE_TO_ELEMENT(expr) ((expr) ? (expr)->ToElement() : nullptr)

        auto conduit = TINYXML_SAFE_TO_ELEMENT(document.FirstChild("conduit"));
        if (!conduit)
        {
            CNDOUT << "Document doesn't have conduit top level note" << std::endl;
            return false;
        }

        // TODO - check version and id

        auto params = TINYXML_SAFE_TO_ELEMENT(conduit->FirstChild("params"));
        if (!params)
        {
            CNDOUT << "Conduit doesn't have child note params" << std::endl;
            return false;
        }

        auto currParam = TINYXML_SAFE_TO_ELEMENT(params->FirstChild("param"));
        if (!currParam)
        {
            std::cout << "Params doesn't have first child param" << std::endl;
            return false;
        }

        while (currParam)
        {
            double value;
            int id;
            if (currParam->QueryDoubleAttribute("value", &value) != TIXML_SUCCESS)
            {
                CNDOUT << "Param doesn't have value attribute" << std::endl;
                return false;
            }
            if (currParam->QueryIntAttribute("id", &id) != TIXML_SUCCESS)
            {
                CNDOUT << "Param doesn't have ID attribute" << std::endl;
                return false;
            }

            *paramToValue[(clap_id)id] = value;

            currParam = TINYXML_SAFE_TO_ELEMENT(currParam->NextSiblingElement("param"));
        }

        if constexpr (TConfig::PatchExtension::hasExtension)
        {
            /*TiXmlElement ext("extension");
            if (!patch.extension.toXml(ext))
                return false;
                */
        }

        return true;
    }

    bool implementsGui() const noexcept override { return clapJuceShim != nullptr; }
    std::unique_ptr<sst::clap_juce_shim::ClapJuceShim> clapJuceShim;
    ADD_SHIM_IMPLEMENTATION(clapJuceShim);
    ADD_SHIM_LINUX_TIMER(clapJuceShim);

    struct ToUI
    {
        enum MType
        {
            PARAM_VALUE = 0x31,
            MIDI_NOTE_ON,
            MIDI_NOTE_OFF
        } type;

        uint32_t id;  // param-id for PARAM_VALUE, key for noteon/noteoff
        double value; // value or unused
    };

    struct FromUI
    {
        enum MType
        {
            BEGIN_EDIT = 0xF9,
            END_EDIT,
            ADJUST_VALUE
        } type;
        uint32_t id;
        double value;
    };

    struct UICommunicationBundle
    {
        UICommunicationBundle(const ClapBaseClass<T, TConfig> &h) : cp(h) {}
        typedef moodycamel::ReaderWriterQueue<ToUI, 4096> SynthToUI_Queue_t;
        typedef moodycamel::ReaderWriterQueue<FromUI, 4096> UIToSynth_Queue_t;

        SynthToUI_Queue_t toUiQ;
        UIToSynth_Queue_t fromUiQ;
        typename TConfig::DataCopyForUI dataCopyForUI;

        std::atomic<bool> refreshUIValues{false};

        // todo make this std optional I guess
        ParamDesc getParameterDescription(uint32_t id) const
        {
            // const so [] isn't an option
            auto fp = cp.paramDescriptionMap.find(id);
            if (fp == cp.paramDescriptionMap.end())
            {
                return ParamDesc();
            }
            return fp->second;
        }

      private:
        const ClapBaseClass<T, TConfig> &cp;
    } uiComms;

    uint32_t handleEventsFromUIQueue(const clap_output_events_t *ov)
    {
        bool uiAdjustedValues{false};
        FromUI r;

        uint32_t adjustedCount{0};
        while (uiComms.fromUiQ.try_dequeue(r))
        {
            switch (r.type)
            {
            case FromUI::BEGIN_EDIT:
            case FromUI::END_EDIT:
            {
                adjustedCount++;
                auto evt = clap_event_param_gesture();
                evt.header.size = sizeof(clap_event_param_gesture);
                evt.header.type = (r.type == FromUI::BEGIN_EDIT ? CLAP_EVENT_PARAM_GESTURE_BEGIN
                                                                : CLAP_EVENT_PARAM_GESTURE_END);
                evt.header.time = 0;
                evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                evt.header.flags = 0;
                evt.param_id = r.id;
                ov->try_push(ov, &evt.header);

                break;
            }
            case FromUI::ADJUST_VALUE:
            {
                adjustedCount++;
                // So set my value
                *paramToValue[r.id] = r.value;

                // But we also need to generate outbound message to the host
                auto evt = clap_event_param_value();
                evt.header.size = sizeof(clap_event_param_value);
                evt.header.type = (uint16_t)CLAP_EVENT_PARAM_VALUE;
                evt.header.time = 0; // for now
                evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
                evt.header.flags = 0;
                evt.param_id = r.id;
                evt.value = r.value;

                ov->try_push(ov, &(evt.header));
            }
            }
        }

        // Similarly we need to push values to a UI on startup
        if (uiComms.refreshUIValues && clapJuceShim->isEditorAttached())
        {
            uiComms.refreshUIValues = false;

            for (const auto &[k, v] : paramToValue)
            {
                auto r = ToUI();
                r.type = ToUI::PARAM_VALUE;
                r.id = k;
                r.value = *v;
                uiComms.toUiQ.try_enqueue(r);
            }
        }

        return adjustedCount;
    }

    bool handleParamBaseEvents(const clap_event_header *evt)
    {
        if (evt->space_id != CLAP_CORE_EVENT_SPACE_ID)
            return false;

        switch (evt->type)
        {
        case CLAP_EVENT_PARAM_VALUE:
        {
            auto v = reinterpret_cast<const clap_event_param_value *>(evt);

            *paramToValue[v->param_id] = v->value;

            if (clapJuceShim && clapJuceShim->isEditorAttached())
            {
                auto r = ToUI();
                r.type = ToUI::PARAM_VALUE;
                r.id = v->param_id;
                r.value = (double)v->value;

                uiComms.toUiQ.try_enqueue(r);
            }
            return true;
        }
        break;
        }

        return false;
    }
};
} // namespace sst::conduit::shared

#endif // CONDUIT_CLAP_BASE_CLASS_H
