/*
 * Conduit - a project highlighting CLAP-first development
 *           and exercising the surge synth team libraries.
 *
 * Copyright 2023 Paul Walker and authors in github
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

#ifndef CONDUIT_SRC_CONDUIT_SHARED_CLAP_BASE_CLASS_H
#define CONDUIT_SRC_CONDUIT_SHARED_CLAP_BASE_CLASS_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <cassert>
#include <fstream>
#include <filesystem>

#include <tinyxml/tinyxml.h>

#include <clap/helpers/plugin.hh>
#include <clap/ext/state.h>

#include "sst/cpputils/ring_buffer.h"
#include "sst/basic-blocks/dsp/Lag.h"
#include "sst/basic-blocks/tables/DbToLinearProvider.h"
#include "sst/basic-blocks/tables/EqualTuningProvider.h"
#include "sst/basic-blocks/tables/TwoToTheXProvider.h"
#include "sst/plugininfra/paths.h"

// note: this is the extension if you are wrapped as a vst3; it is not any vst3 sdk
#include <clapwrapper/vst3.h>

#include <sst/basic-blocks/params/ParamMetadata.h>
#include <sst/clap_juce_shim/clap_juce_shim.h>
#include "debug-helpers.h"

namespace sst::conduit::shared
{
static constexpr clap::helpers::MisbehaviourHandler misLevel =
    clap::helpers::MisbehaviourHandler::Ignore;
static constexpr clap::helpers::CheckingLevel checkLevel = clap::helpers::CheckingLevel::Maximal;

using plugHelper_t = clap::helpers::Plugin<misLevel, checkLevel>;

struct EmptyPatchExtension
{
    static constexpr bool hasExtension{false};
};

template <typename T, typename TConfig>
struct ClapBaseClass : public plugHelper_t, sst::clap_juce_shim::EditorProvider
{
    using config_t = TConfig;

    ClapBaseClass(const clap_host *host)
        : plugHelper_t(TConfig::getDescription(), host), uiComms(*this)
    {
        dbToLinearTable.init();
        equalTuningTable.init();
        twoToXTable.init();
        guaranteeDocumentsPath();
    }

    ClapBaseClass(const clap_plugin_descriptor *desc, const clap_host *host)
        : plugHelper_t(desc, host), uiComms(*this)
    {
        dbToLinearTable.init();
        equalTuningTable.init();
        twoToXTable.init();
        guaranteeDocumentsPath();
    }

    // Most things are sample accurate, but some have a slow- or block- based approach.
    // This is the default block size for those
    static constexpr int blockSize{16};

    using ParamDesc = sst::basic_blocks::params::ParamMetaData;
    std::vector<ParamDesc> paramDescriptions;
    std::unordered_map<uint32_t, ParamDesc> paramDescriptionMap;

    sst::basic_blocks::tables::DbToLinearProvider dbToLinearTable;
    sst::basic_blocks::tables::EqualTuningProvider equalTuningTable;
    sst::basic_blocks::tables::TwoToTheXProvider twoToXTable;

#define cbassert(x, y)                                                                             \
    {                                                                                              \
        if (!(x))                                                                                  \
        {                                                                                          \
            CNDOUT << #x << " " << y << std::endl;                                                 \
            std::terminate();                                                                      \
        }                                                                                          \
    }

    void configureParams()
    {
        cbassert(paramDescriptions.size() == TConfig::nParams,
                 "Incorrect size " << TConfig::nParams << " vs " << paramDescriptions.size());
        paramDescriptionMap.clear();
        int patchIdx{0};
        for (const auto &pd : paramDescriptions)
        {
            // If you hit this cbassert you have a duplicate param id
            cbassert(paramDescriptionMap.find(pd.id) == paramDescriptionMap.end(),
                     "Duplicate Param IDs");
            paramDescriptionMap[pd.id] = pd;
            paramToPatchIndex[pd.id] = patchIdx;
            paramToValue[pd.id] = &(patch.params[patchIdx]);

            patch.params[patchIdx] = pd.defaultVal;
            if (TConfig::baseClassProvidesMonoModSupport)
            {
                monoModulatedPatch.update(patchIdx, patch);
            }

            patchIdx++;
        }
        cbassert(paramDescriptionMap.size() == TConfig::nParams, "Bad Traversal");
        cbassert(patchIdx == TConfig::nParams, "Bad Traversal");
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

        pd.template toClapParamInfo<CLAP_NAME_SIZE - 1>(info);
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
        auto sValue = paramValueDisplay(paramId, value);
        if (sValue.has_value())
        {
            strncpy(display, sValue->c_str(), size);
            return true;
        }
        return false;
    }

    std::optional<std::string> paramValueDisplay(clap_id paramId, double value) const
    {
        auto pos = paramDescriptionMap.find(paramId);
        if (pos == paramDescriptionMap.end())
            return std::nullopt;

        const auto &pd = pos->second;
        ParamDesc::FeatureState fs;

        auto tsBuddy = temposyncActivatedBy.find(paramId);
        if (tsBuddy != temposyncActivatedBy.end())
        {
            auto pvIt = paramToValue.find(tsBuddy->second);
            if (pvIt != paramToValue.end())
            {
                auto isTS = *(pvIt->second) > 0.5;
                fs = fs.withTemposync(isTS);
            }
        }

        auto sValue = pd.valueToString(value, fs);
        return sValue;
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

    std::unordered_map<clap_id, clap_id> temposyncActivatedBy;
    void addTemposyncActivator(clap_id toThis, clap_id byThat)
    {
        temposyncActivatedBy[toThis] = byThat;
    }

    struct Patch
    {
        float params[TConfig::nParams];
        typename TConfig::PatchExtension extension;
    } patch;

    struct MonoModulatedPatch
    {
        float modulations[TConfig::nParams]{};
        float values[TConfig::nParams]{};
        void update(int idx, Patch &p) { values[idx] = modulations[idx] + p.params[idx]; }
        void updateAll(Patch &p)
        {
            for (auto i = 0U; i < TConfig::nParams; ++i)
            {
                update(i, p);
            }
        }
    } monoModulatedPatch;

    std::unordered_map<clap_id, float *> paramToValue;
    std::unordered_map<clap_id, int> paramToPatchIndex;

    using lag_t = sst::basic_blocks::dsp::SurgeLag<float, true>;
    std::unordered_map<clap_id, lag_t *> paramToLag;

    void processLags()
    {
        for (const auto lp : paramToLag)
        {
            lp.second->process();
        }
    }

    void attachParam(clap_id paramId, float *&to)
    {
        auto ptpi = paramToPatchIndex.find(paramId);
        if (ptpi == paramToPatchIndex.end())
        {
            to = nullptr;
        }
        else
        {
            if (TConfig::baseClassProvidesMonoModSupport)
            {
                to = &monoModulatedPatch.values[ptpi->second];
            }
            else
            {
                to = &patch.params[ptpi->second];
            }
        }
    }

    void attachParam(clap_id paramId, lag_t &to)
    {
        auto val = 0;
        auto ptpi = paramToPatchIndex.find(paramId);
        if (ptpi != paramToPatchIndex.end())
        {
            if (TConfig::baseClassProvidesMonoModSupport)
            {
                val = monoModulatedPatch.values[ptpi->second];
            }
            else
            {
                val = patch.params[ptpi->second];
            }
        }
        paramToLag[paramId] = &to;
        to.newValue(val);
        to.instantize();
    }

  protected:
    // This is an OK default implementation but you may want to replace it
    void paramsFlush(const clap_input_events *in, const clap_output_events *out) noexcept override
    {
        auto sz = in->size(in);

        for (auto e = 0U; e < sz; ++e)
        {
            auto nextEvent = in->get(in, e);
            handleParamBaseEvents(nextEvent);
        }

        handleEventsFromUIQueue(out);
    }

  public:
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

        if constexpr (TConfig::PatchExtension::hasExtension)
        {
            TiXmlElement ext("extension");
            if (!patch.extension.toXml(ext))
                return false;
            conduit.InsertEndChild(ext);
        }

        document.InsertEndChild(conduit);

        TiXmlPrinter pr;
        document.Accept(&pr);

        auto xmlS = pr.Str();

        auto c = xmlS.c_str();
        auto s = xmlS.length(); // write the null terminator
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

        if (document.Error() != TiXmlBase::TIXML_NO_ERROR)
        {
            CNDOUT << "Error Parsing XML : " << document.ErrorDesc()
                   << " row=" << document.ErrorRow() << " col=" << document.ErrorCol() << std::endl;
            return false;
        }

#define TINYXML_SAFE_TO_ELEMENT(expr) ((expr) ? (expr)->ToElement() : nullptr)

        auto conduit = TINYXML_SAFE_TO_ELEMENT(document.FirstChild("conduit"));
        if (!conduit)
        {
            CNDOUT << "Document doesn't have conduit top level note" << std::endl;
            return false;
        }

        // TODO - check streamingVersion once we bump it
        int sv;
        if (conduit->QueryIntAttribute("streamingVersion", &sv) == TIXML_SUCCESS)
        {
            if (sv > streamingVersion)
            {
                CNDOUT << "Streaming version '" << sv << "' greater than '" << streamingVersion
                       << "'" << std::endl;
                return false;
            }
        }

        std::string spid;
        if (conduit->QueryStringAttribute("plugin_id", &spid) == TIXML_SUCCESS)
        {
            if (spid != TConfig::getDescription()->id)
            {
                // TODO - a better way to report this error to the user?
                CNDOUT << "State file for '" << spid << "' doesn't match plugin id '"
                       << TConfig::getDescription()->id << "'" << std::endl;
                return false;
            }
        }
        else
        {
            CNDOUT << "Cannot determine plugin id from stream" << std::endl;
            return false;
        }

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

        int restoredParams{0};
        while (currParam)
        {
            double value;
            int id;
            if (currParam->QueryDoubleAttribute("value", &value) != TIXML_SUCCESS)
            {
                CNDOUT << "Param doesn't have value attribute" << std::endl;
                goto nextParam;
            }
            if (currParam->QueryIntAttribute("id", &id) != TIXML_SUCCESS)
            {
                CNDOUT << "Param doesn't have ID attribute" << std::endl;
                goto nextParam;
            }

            {
                auto pos = paramToValue.find((clap_id)id);
                if (pos != paramToValue.end())
                {
                    restoredParams++;
                    *paramToValue[(clap_id)id] = value;
                }
                else
                {
                    CNDOUT << "Unknown parameter " << id << " in stream" << std::endl;
                    // continue anyway
                }
                auto plv = paramToLag.find(id);
                if (plv != paramToLag.end())
                {
                    plv->second->newValue(value);
                    plv->second->instantize();
                }
            }
        nextParam:
            currParam = TINYXML_SAFE_TO_ELEMENT(currParam->NextSiblingElement("param"));
        }

        if (restoredParams != TConfig::nParams)
        {
            CNDOUT << "Warning : Restored " << restoredParams << " vs expected " << TConfig::nParams
                   << std::endl;
        }

        if constexpr (TConfig::PatchExtension::hasExtension)
        {
            auto ext = TINYXML_SAFE_TO_ELEMENT(conduit->FirstChild("extension"));
            if (ext)
            {
                if (!patch.extension.fromXml(ext))
                {
                    return false;
                }
            }
        }

        if (TConfig::baseClassProvidesMonoModSupport)
        {
            monoModulatedPatch.updateAll(patch);
        }
        onStateRestored();
        return true;
    }

    virtual void onStateRestored() {}

    // Sample Rate Support
    double sampleRate{0}, sampleRateInv{0}, dsamplerate_inv{0};
    void setSampleRate(double sr)
    {
        sampleRate = sr;
        sampleRateInv = 1.0 / sr;
        dsamplerate_inv = sampleRateInv; // just an alis
    }

    bool implementsGui() const noexcept override { return clapJuceShim != nullptr; }
    std::unique_ptr<sst::clap_juce_shim::ClapJuceShim> clapJuceShim;
    ADD_SHIM_IMPLEMENTATION(clapJuceShim)
    ADD_SHIM_LINUX_TIMER(clapJuceShim)

    struct ToUI
    {
        enum MType
        {
            PARAM_VALUE = 0x31,
            PARAM_MODULATION,
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
            ADJUST_VALUE,
            LOAD_PATCH,
            SAVE_PATCH,

            SPECIALIZED // basically use the id as a router
        } type;
        uint32_t id{};

        double value{};

        char *strPointer{nullptr};

        template <bool Cond, typename Tp> struct specTypeTrait
        {
            typedef int type;
        };

        template <typename Tp> struct specTypeTrait<true, Tp>
        {
            typedef typename Tp::specializedMessage_t type;
        };

        typename specTypeTrait<TConfig::usesSpecializedMessages, TConfig>::type
            specializedMessage{};
    };

    /* In the future we will want this off audio thread with a tiny
     * fade and stuff but not yet. Anticipating that though at least
     * make a class that does the doohickies so later we can do asynch
     * of thread etc...
     *
     * This implementation misses many basic things including error
     * handling
     */
    struct PatchIOHandler
    {
        enum Op
        {
            SAVE,
            LOAD
        };
        // but for now its a hack that just does it directly
        void enqueueOperation(ClapBaseClass<T, TConfig> &that, Op operation, char *pointerToPath)
        {
            try
            {
                std::filesystem::path fsp{pointerToPath};
                delete pointerToPath;

                if (operation == SAVE)
                {
                    std::ofstream ofs(fsp, std::ios::out | std::ios::binary);
                    if (!ofs.is_open())
                    {
                        CNDOUT << "Unable to open for writing " << fsp.u8string() << std::endl;
                        return;
                    }
                    CNDOUT << "Writing patch to " << fsp.u8string() << std::endl;
                    clap_ostream cos{};
                    cos.ctx = &ofs;
                    cos.write = clapwrite;
                    that.stateSave(&cos);
                    ofs.close();
                }
                else
                {
                    std::ifstream ifs(fsp, std::ios::in | std::ios::binary);
                    if (!ifs.is_open())
                    {
                        CNDOUT << "Unable to open for reading " << fsp.u8string() << std::endl;
                        return;
                    }
                    CNDOUT << "Reading patch from " << fsp.u8string() << std::endl;
                    clap_istream cis{};
                    cis.ctx = &ifs;
                    cis.read = clapread;
                    that.stateLoad(&cis);
                    ifs.close();

                    that.uiComms.refreshUIValues = true;
                    if (that._host.canUseParams())
                    {
                        that.onMainAction |= OnMainAction::RESCAN;
                        that._host.requestCallback();
                    }
                }
            }
            catch (const std::filesystem::filesystem_error &e)
            {
            }
        }

        static int64_t clapwrite(const clap_ostream *s, const void *buffer, uint64_t size)
        {
            auto ofs = static_cast<std::ofstream *>(s->ctx);
            ofs->write((const char *)buffer, size);
            return size;
        }

        static int64_t clapread(const struct clap_istream *s, void *buffer, uint64_t size)
        {
            auto ifs = static_cast<std::ifstream *>(s->ctx);

            // Oh this API is so terrible. I think this is right?
            ifs->read(static_cast<char *>(buffer), size);
            if (ifs->rdstate() == std::ios::goodbit || ifs->rdstate() == std::ios::eofbit)
                return ifs->gcount();

            if (ifs->rdstate() & std::ios::eofbit)
                return ifs->gcount();

            return -1;
        }

    } patchIOHandler;

    enum OnMainAction
    {
        RESCAN = 1
    };
    int onMainAction{0};
    void onMainThread() noexcept override
    {
        if (onMainAction & OnMainAction::RESCAN)
        {
            _host.paramsRescan(CLAP_PARAM_RESCAN_VALUES | CLAP_PARAM_RESCAN_TEXT);
        }
        onMainAction = 0;
        Plugin::onMainThread();
    }

    struct UICommunicationBundle
    {
        UICommunicationBundle(ClapBaseClass<T, TConfig> &h) : cp(h) {}
        typedef sst::cpputils::SimpleRingBuffer<ToUI, 4096> SynthToUI_Queue_t;
        typedef sst::cpputils::SimpleRingBuffer<FromUI, 4096> UIToSynth_Queue_t;

        SynthToUI_Queue_t toUiQ;
        UIToSynth_Queue_t fromUiQ;
        typename TConfig::DataCopyForUI dataCopyForUI;

        std::atomic<bool> refreshUIValues{true};

        void requestHostParamFlush() const
        {
            if (cp._host.canUseParams())
                cp._host.paramsRequestFlush();
        }

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

        std::vector<ParamDesc> getAllParamDescriptions() const
        {

            auto res = std::vector<ParamDesc>(cp.paramDescriptions.size());
            for (const auto &p : cp.paramDescriptions)
                res.push_back(p);
            return res;
        }

        std::optional<std::string> getParamValueDisplay(clap_id id, double d) const
        {
            return cp.paramValueDisplay(id, d);
        }

        std::filesystem::path getDocumentsPath() const { return cp.documentsPath; }

      private:
        // Used to be const but I want to save and load from the UI thread
        // so make it private and only do that internally
        ClapBaseClass<T, TConfig> &cp;
    } uiComms;

    std::filesystem::path documentsPath;
    void guaranteeDocumentsPath()
    {
        try
        {
            auto bp = sst::plugininfra::paths::bestDocumentsFolderPathFor("Conduit");
            if (!std::filesystem::exists(bp))
            {
                std::filesystem::create_directories(bp);
            }
            documentsPath = bp;
        }
        catch (const std::filesystem::filesystem_error &e)
        {
        }
    }

    void doValueUpdate(clap_id id, float value)
    {
        auto ptpi = paramToPatchIndex.find(id);
        if (ptpi == paramToPatchIndex.end())
            return;

        int index = ptpi->second;
        patch.params[index] = value;
        if (TConfig::baseClassProvidesMonoModSupport)
        {
            monoModulatedPatch.update(index, patch);
        }
        auto ptl = paramToLag.find(id);
        if (ptl != paramToLag.end())
        {
            if (TConfig::baseClassProvidesMonoModSupport)
            {
                ptl->second->newValue(monoModulatedPatch.values[index]);
            }
            else
            {
                ptl->second->newValue(value);
            }
        }
    }

    void doMonoModulationUpdate(clap_id id, float value)
    {
        assert(TConfig::baseClassProvidesMonoModSupport);
        auto ptpi = paramToPatchIndex.find(id);
        if (ptpi == paramToPatchIndex.end())
            return;

        int index = ptpi->second;
        monoModulatedPatch.modulations[index] = value;
        monoModulatedPatch.update(index, patch);
        auto val = monoModulatedPatch.values[index];

        auto ptl = paramToLag.find(id);
        if (ptl != paramToLag.end())
        {
            ptl->second->newValue(val);
        }
    }

    uint32_t handleEventsFromUIQueue(const clap_output_events_t *ov)
    {
        uint32_t adjustedCount{0};
        while (!uiComms.fromUiQ.empty())
        {
            auto r = *uiComms.fromUiQ.pop();
            adjustedCount++;
            generateOutputMessagesFromUI(r, ov);
            if (r.type == FromUI::ADJUST_VALUE)
            {
                doValueUpdate(r.id, r.value);
            }
        }
        refreshUIIfNeeded();
        return adjustedCount;
    }

    void refreshUIIfNeeded()
    {
        // Similarly we need to push values to a UI on startup
        if (uiComms.refreshUIValues && clapJuceShim->isEditorAttached())
        {
            CNDOUT << "Refreshing UI" << std::endl;
            uiComms.refreshUIValues = false;

            for (const auto &[k, v] : paramToValue)
            {
                auto r = ToUI();
                r.type = ToUI::PARAM_VALUE;
                r.id = k;
                r.value = *v;
                uiComms.toUiQ.push(r);
            }
        }
    }

    void generateOutputMessagesFromUI(const FromUI &r, const clap_output_events_t *ov)
    {
        switch (r.type)
        {
        case FromUI::BEGIN_EDIT:
        case FromUI::END_EDIT:
        {
            auto evt = clap_event_param_gesture();
            evt.header.size = sizeof(clap_event_param_gesture);
            evt.header.type = (r.type == FromUI::BEGIN_EDIT ? CLAP_EVENT_PARAM_GESTURE_BEGIN
                                                            : CLAP_EVENT_PARAM_GESTURE_END);
            evt.header.time = 0;
            evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            evt.header.flags = 0;
            evt.param_id = r.id;
            ov->try_push(ov, &evt.header);
        }
        break;
        case FromUI::ADJUST_VALUE:
        {
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
        break;
        case FromUI::LOAD_PATCH:
            // For now just do this. See comment on IO Handler
            patchIOHandler.enqueueOperation(*this, PatchIOHandler::LOAD, r.strPointer);
            break;
        case FromUI::SAVE_PATCH:
            // For now just do this. See comment on IO Handler
            patchIOHandler.enqueueOperation(*this, PatchIOHandler::SAVE, r.strPointer);
            break;
        case FromUI::SPECIALIZED:
            if constexpr (TConfig::usesSpecializedMessages)
            {
                static_cast<T *>(this)->handleSpecializedFromUI(r);
            }
            break;
        }
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
            updateParamInPatch(v);
            return true;
        }
        case CLAP_EVENT_PARAM_MOD:
        {
            if (TConfig::baseClassProvidesMonoModSupport)
            {
                auto v = reinterpret_cast<const clap_event_param_mod *>(evt);
                updateModulation(v);
                return true;
            }
        }

        break;
        }

        return false;
    }

    void updateParamInPatch(const clap_event_param_value *v)
    {
        doValueUpdate(v->param_id, v->value);
        if (clapJuceShim && clapJuceShim->isEditorAttached())
        {
            auto r = ToUI();
            r.type = ToUI::PARAM_VALUE;
            r.id = v->param_id;
            r.value = (double)v->value;

            uiComms.toUiQ.push(r);
        }
    }

    void updateModulation(const clap_event_param_mod *v)
    {
        doMonoModulationUpdate(v->param_id, v->amount);
        if (clapJuceShim && clapJuceShim->isEditorAttached())
        {
            auto r = ToUI();
            r.type = ToUI::PARAM_MODULATION;
            r.id = v->param_id;
            r.value = (double)v->amount;

            uiComms.toUiQ.push(r);
        }
    }

    bool registerOrUnregisterTimer(clap_id &id, int ms, bool reg) override
    {
        if (!_host.canUseTimerSupport())
            return false;
        if (reg)
        {
            _host.timerSupportRegister(ms, &id);
        }
        else
        {
            _host.timerSupportUnregister(id);
        }
        return true;
    };

    virtual bool implementsPluginAsVST3() { return true; }
    virtual uint32_t getAsVst3NumMIDIChannels(int port) { return 16; }
    virtual uint32_t getAsVst3SupportedNodeExpressions() { return 0; }

    static uint32_t pluginAsVst3GetNumMIDIChannels(const clap_plugin *plugin, uint32_t note_port)
    {
        auto self = static_cast<ClapBaseClass<T, TConfig> *>(plugin->plugin_data);
        return self->getAsVst3NumMIDIChannels(note_port);
    }
    static uint32_t pluginAsVst3SupportedNoteExpressions(const clap_plugin *plugin)
    {
        auto self = static_cast<ClapBaseClass<T, TConfig> *>(plugin->plugin_data);
        return self->getAsVst3SupportedNodeExpressions();
    }
    const clap_plugin_as_vst3 _extensionPluginAsVST3 = {&pluginAsVst3GetNumMIDIChannels,
                                                        &pluginAsVst3SupportedNoteExpressions};

    const void *extension(const char *id) noexcept override
    {
        if (!strcmp(id, CLAP_PLUGIN_AS_VST3) && implementsPluginAsVST3())
        {
            return &_extensionPluginAsVST3;
        }

        return Plugin::extension(id);
    }

    // Support for SST Biquads
    float note_to_pitch_ignoring_tuning(float n) const { return equalTuningTable.note_to_pitch(n); }
    float dbToLinear(float n) const { return dbToLinearTable.dbToLinear(n); }
};
} // namespace sst::conduit::shared

#endif // CONDUIT_CLAP_BASE_CLASS_H
