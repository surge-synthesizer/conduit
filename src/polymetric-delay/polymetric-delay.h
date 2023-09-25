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

#ifndef CONDUIT_SRC_POLYMETRIC_DELAY_POLYMETRIC_DELAY_H
#define CONDUIT_SRC_POLYMETRIC_DELAY_POLYMETRIC_DELAY_H

#include <clap/helpers/plugin.hh>
#include <atomic>
#include <array>
#include <unordered_map>
#include <memory>

#include <memory>

#include "conduit-shared/sse-include.h"

#include "sst/basic-blocks/params/ParamMetadata.h"
#include "sst/basic-blocks/dsp/VUPeak.h"
#include "sst/basic-blocks/dsp/SSESincDelayLine.h"
#include "sst/basic-blocks/dsp/QuadratureOscillators.h"
#include "sst/basic-blocks/tables/SincTableProvider.h"

#include "sst/filters/BiquadFilter.h"

#include "conduit-shared/clap-base-class.h"

namespace sst::conduit::polymetric_delay
{

extern clap_plugin_descriptor desc;
static constexpr int nParams = 45;

struct ConduitPolymetricDelayConfig
{
    static constexpr int nParams{sst::conduit::polymetric_delay::nParams};
    static constexpr bool baseClassProvidesMonoModSupport{true};
    using PatchExtension = sst::conduit::shared::EmptyPatchExtension;
    struct DataCopyForUI
    {
        std::atomic<uint32_t> updateCount{0};
        std::atomic<bool> isProcessing{false};

        std::atomic<bool> isPlayingOrRecording;
        std::atomic<double> tempo;
        std::atomic<clap_beattime> bar_start;
        std::atomic<int32_t> bar_number;
        std::atomic<clap_beattime> song_pos_beats;

        std::atomic<uint16_t> tsig_num, tsig_denom;

        std::atomic<float> inVu[2], outVu[2], tapVu[4][2];
    };

    static clap_plugin_descriptor *getDescription() { return &desc; }
};

struct ConduitPolymetricDelay
    : sst::conduit::shared::ClapBaseClass<ConduitPolymetricDelay, ConduitPolymetricDelayConfig>
{
    ConduitPolymetricDelay(const clap_host *host);
    ~ConduitPolymetricDelay();

    bool activate(double sr, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept override
    {
        setSampleRate(sr);
        recalcTaps();
        recalcModulators();

        inVU.setSampleRate(sr);
        outVU.setSampleRate(sr);
        for (auto &t : tapOutVU)
            t.setSampleRate(sr);
        return true;
    }

    static constexpr int nTaps{4};
    enum paramIds : uint32_t
    {
        // These set of parameters are one-per
        pmDryLevel = 81, // in dsp, in gui

        // These set of parameters are per tap, so level on tap N = pmTapLevel + N. So you need
        // to space them by more than nTaps. I space them by 1000 here
        pmTapActive = 103241, // in dsp, in gui

        pmDelayTimeNTaps = 100241,  // in dsp, in gui, need new widget
        pmDelayTimeEveryM = 101241, // in dsp, in gui, need new widget
        pmDelayModRate = 107241,
        pmDelayModDepth = 108241,

        pmTapLowCut = 104241,
        pmTapHighCut = 105241,

        pmTapLevel = 109241,         // in dsp, in gui
        pmTapFeedback = 110241,      // in dsp, in gui
        pmTapCrossFeedback = 110341, // in dsp in gui
        pmTapOutputPan = 110441

    };

    inline bool isTapParam(clap_id pid, paramIds base) { return pid >= base && pid < base + nTaps; }

    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override { return 1; }
    bool audioPortsInfo(uint32_t index, bool isInput,
                        clap_audio_port_info *info) const noexcept override;

    /*
     * I have an unacceptably crude state dump and restore. If you want to
     * improve it, PRs welcome! But it's just like any other read-and-write-goop
     * from-a-stream api really.
     */

    clap_process_status process(const clap_process *process) noexcept override;
    void handleInboundEvent(const clap_event_header_t *evt);

    bool startProcessing() noexcept override
    {
        uiComms.dataCopyForUI.isProcessing = true;
        uiComms.dataCopyForUI.updateCount++;

        for (int i = 0; i < nTaps; ++i)
        {
            hp[i].suspend();
            lp[i].suspend();

            setTapFilterFrequencies(i);

            hp[i].coeff_instantize();
            lp[i].coeff_instantize();
        }

        return true;
    }

    void stopProcessing() noexcept override
    {
        uiComms.dataCopyForUI.isProcessing = false;
        uiComms.dataCopyForUI.updateCount++;
    }

    void setTapFilterFrequencies(int i)
    {
        hp[i].coeff_HP(hp[i].calc_omega(*(tapData[i].locut) / 12.0), 0.707);
        lp[i].coeff_LP2B(lp[i].calc_omega(*(tapData[i].hicut) / 12.0), 0.707);
    }

    typedef std::unordered_map<int, int> PatchPluginExtension;

    sst::basic_blocks::dsp::VUPeak inVU, outVU, tapOutVU[nTaps];
    uint32_t slowProcess{blockSize};

    // For now our strategy is to just have honkin big delay lines
    // but we want to make these adapt with max time going forward
    // This is enough for about 20 seconds of delay at 48khz
    sst::basic_blocks::tables::SurgeSincTableProvider st{};
    static constexpr uint32_t dlSize{1 << 20};
    sst::basic_blocks::dsp::SSESincDelayLine<dlSize> delayLine[2]{st, st};

  protected:
    std::unique_ptr<juce::Component> createEditor() override;
    std::atomic<bool> refreshUIValues{false};

    float tapPanMatrix[nTaps][4];

    void specificParamChange(clap_id id, float val);

  public:
    float *dryLev;

    struct TapData
    {
        float *ntaps, *mbeats, *active;

        float *locut, *hicut;
        lag_t level, fblev, crossfblev, pan, moddepth, modrate;

        sst::basic_blocks::dsp::QuadratureOscillator<float> modulator;
    } tapData[nTaps];

    float baseTapSamples[nTaps]{};
    float tempo;

    void recalcTaps();
    void recalcModulators();

    static constexpr float modDepthScale{0.05f};

    void onStateRestored() override
    {
        recalcTaps();
        recalcModulators();
    }

    std::array<sst::filters::Biquad::BiquadFilter<ConduitPolymetricDelay, blockSize>, nTaps> hp, lp;
};
} // namespace sst::conduit::polymetric_delay

#endif // CONDUIT_POLYMETRIC_DELAY_H
