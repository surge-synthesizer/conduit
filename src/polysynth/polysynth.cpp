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

#include "polysynth.h"
#include <iostream>
#include <cmath>
#include <cstring>

#include <iomanip>
#include <locale>

#include "version.h"

#include "libMTSClient.h"

#include "sst/cpputils/constructors.h"
#include "sst/basic-blocks/mechanics/block-ops.h"

#include "effects-impl.h"

namespace sst::conduit::polysynth
{

const char *features[] = {CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_SYNTHESIZER, nullptr};
clap_plugin_descriptor desc = {CLAP_VERSION,
                               "org.surge-synth-team.conduit.polysynth",
                               "Conduit Polysynth",
                               "Surge Synth Team",
                               "https://surge-synth-team.org",
                               "",
                               "",
                               sst::conduit::build::FullVersionStr,
                               "The Conduit Polysynth is a work in progress",
                               features};

ConduitPolysynth::ConduitPolysynth(const clap_host *host)
    : sst::conduit::shared::ClapBaseClass<ConduitPolysynth, ConduitPolysynthConfig>(&desc, host),
      gen((size_t)this), urd(0.f, 1.f),hr_dn(6, true),
      voiceManager(*this), voices{sst::cpputils::make_array<PolysynthVoice, max_voices>(*this)}
{
    auto autoFlag = CLAP_PARAM_IS_AUTOMATABLE;
    auto monoModFlag = autoFlag | CLAP_PARAM_IS_MODULATABLE;
    auto modFlag = autoFlag | CLAP_PARAM_IS_MODULATABLE | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID |
                   CLAP_PARAM_IS_MODULATABLE_PER_KEY;
    auto steppedFlag = autoFlag | CLAP_PARAM_IS_STEPPED;

    auto coarseBase = ParamDesc()
                          .asFloat()
                          .withRange(-24, 24)
                          .withDefault(0)
                          .withFlags(modFlag)
                          .withLinearScaleFormatting("keys");
    auto fineBase = ParamDesc()
                        .asFloat()
                        .withRange(-100, 100)
                        .withDefault(0)
                        .withFlags(modFlag)
                        .withLinearScaleFormatting("cents");
    auto levelBase = ParamDesc().asCubicDecibelAttenuation().withDefault(1).withFlags(modFlag);
    auto activeBase = ParamDesc().asBool().withFlags(steppedFlag).withDefault(true);
    auto freqDivBase =
        ParamDesc()
            .asInt()
            .withRange(-3, 3)
            .withDefault(0)
            .withFlags(steppedFlag)
            .withUnorderedMapFormatting(
                {{-3, "/8"}, {-2, "/4"}, {-1, "/2"}, {0, "x1"}, {1, "x2"}, {2, "x4"}, {3, "x8"}});

    paramDescriptions.push_back(
        activeBase.withID(pmSawActive).withName("Saw Active").withGroupName("Saw Oscillator"));
    paramDescriptions.push_back(ParamDesc()
                                    .asInt()
                                    .withID(pmSawUnisonCount)
                                    .withName("Saw Unison Count")
                                    .withGroupName("Saw Oscillator")
                                    .withRange(1, PolysynthVoice::max_uni)
                                    .withDefault(3)
                                    .withFlags(steppedFlag)
                                    .withLinearScaleFormatting("voices"));
    paramDescriptions.push_back(ParamDesc()
                                    .asFloat()
                                    .withID(pmSawUnisonSpread)
                                    .withName("Saw Unison Spread")
                                    .withGroupName("Saw Oscillator")
                                    .withLinearScaleFormatting("cents")
                                    .withRange(0, 100)
                                    .withDefault(10)
                                    .withFlags(modFlag));
    paramDescriptions.push_back(coarseBase.withID(pmSawCoarse)
                                    .withName("Saw Coarse Offset")
                                    .withGroupName("Saw Oscillator"));
    paramDescriptions.push_back(
        fineBase.withID(pmSawFine).withName("Saw Fine Tuning").withGroupName("Saw Oscillator"));
    paramDescriptions.push_back(
        levelBase.withID(pmSawLevel).withName("Saw Level").withGroupName("Saw Oscillator"));

    paramDescriptions.push_back(
        activeBase.withID(pmPWActive).withName("Pulse Width Active").withGroupName("Pulse Width"));
    paramDescriptions.push_back(ParamDesc()
                                    .asPercent()
                                    .withID(pmPWWidth)
                                    .withName("Pulse Width Width")
                                    .withGroupName("Pulse Width")
                                    .withDefault(0.5));
    paramDescriptions.push_back(freqDivBase.withID(pmPWFrequencyDiv)
                                    .withName("Pulse Width Frequency Multiple")
                                    .withGroupName("Pulse Width")
                                    .withDefault(-1));
    paramDescriptions.push_back(
        coarseBase.withID(pmPWCoarse).withName("Pulse Width Coarse").withGroupName("Pulse Width"));
    paramDescriptions.push_back(
        fineBase.withID(pmPWFine).withName("Pulse Width Fine").withGroupName("Pulse Width"));
    paramDescriptions.push_back(
        levelBase.withID(pmPWLevel).withName("Pulse Width Level").withGroupName("Pulse Width"));

    paramDescriptions.push_back(activeBase.withID(pmSinActive)
                                    .withName("Sin Active")
                                    .withGroupName("Sin")
                                    .withDefault(false));
    paramDescriptions.push_back(freqDivBase.withID(pmSinFrequencyDiv)
                                    .withName("Sin Frequency Multiple")
                                    .withGroupName("Sin"));
    paramDescriptions.push_back(
        coarseBase.withID(pmSinCoarse).withName("Sin Coarse").withGroupName("Sin"));
    paramDescriptions.push_back(
        levelBase.withID(pmSinLevel).withName("Sin Level").withGroupName("Sin"));

    paramDescriptions.push_back(activeBase.withID(pmNoiseActive)
                                    .withName("Noise Active")
                                    .withGroupName("Noise")
                                    .withDefault(false));
    paramDescriptions.push_back(ParamDesc()
                                    .asPercentBipolar()
                                    .withID(pmNoiseColor)
                                    .withName("Noise Color")
                                    .withGroupName("Noise")
                                    .withFlags(modFlag));
    paramDescriptions.push_back(
        levelBase.withID(pmNoiseLevel).withName("Noise Level").withGroupName("Noise"));

    paramDescriptions.push_back(activeBase.withID(pmLPFActive)
                                    .withName("LPF Active")
                                    .withGroupName("LPF Filter")
                                    .withDefault(0));
    paramDescriptions.push_back(ParamDesc()
                                    .asFloat()
                                    .withID(pmLPFCutoff)
                                    .withName("LPF Cutoff")
                                    .withGroupName("LPF Filter")
                                    .withRange(1, 127)
                                    .withDefault(60)
                                    .withSemitoneZeroAtMIDIZeroFormatting()
                                    .withFlags(modFlag));
    paramDescriptions.push_back(ParamDesc()
                                    .asFloat()
                                    .withID(pmLPFResonance)
                                    .withName("LPF Resonance")
                                    .withGroupName("LPF Filter")
                                    .withRange(0, 1)
                                    .withDefault(sqrt(2) / 2)
                                    .withLinearScaleFormatting("")
                                    .withFlags(modFlag));
    paramDescriptions.push_back(
        ParamDesc()
            .asInt()
            .withID(pmLPFFilterMode)
            .withName("LPF Mode")
            .withGroupName("LPF Filter")
            .withRange(0, 5)
            .withDefault(0)
            .withFlags(steppedFlag)
            .withUnorderedMapFormatting({{PolysynthVoice::OBXD, "ObXD"},
                                         {PolysynthVoice::Vintage, "Vintage"},
                                         {PolysynthVoice::K35, "K-35"},
                                         {PolysynthVoice::Comb, "Comb"},
                                         {PolysynthVoice::CutWarp, "CutWarp"},
                                         {PolysynthVoice::ResWarp, "ResWarp"}})); // FIXME - enums
    paramDescriptions.push_back(ParamDesc()
                                    .asPercentBipolar()
                                    .withID(pmLPFKeytrack)
                                    .withName("LPF Keytrack")
                                    .withGroupName("LPF Filter")
                                    .withFlags(modFlag)
                                    .withDefault(0));

    paramDescriptions.push_back(activeBase.withID(pmSVFActive)
                                    .withName("Multi-Mode Active")
                                    .withGroupName("Multi-Mode Filter"));
    paramDescriptions.push_back(ParamDesc()
                                    .asFloat()
                                    .withID(pmSVFCutoff)
                                    .withName("Multi-Mode Cutoff")
                                    .withGroupName("Multi-Mode Filter")
                                    .withRange(1, 127)
                                    .withDefault(69)
                                    .withSemitoneZeroAtMIDIZeroFormatting()
                                    .withFlags(modFlag));
    paramDescriptions.push_back(ParamDesc()
                                    .asFloat()
                                    .withID(pmSVFResonance)
                                    .withName("Multi-Mode Resonance")
                                    .withGroupName("Multi-Mode Filter")
                                    .withRange(0, 1)
                                    .withDefault(sqrt(2) / 2)
                                    .withLinearScaleFormatting("")
                                    .withFlags(modFlag));
    std::unordered_map<int, std::string> filterModes;
    using sv = PolysynthVoice::StereoSimperSVF;
    filterModes[sv::LP] = "Low Pass";
    filterModes[sv::HP] = "High Pass";
    filterModes[sv::BP] = "Band Pass";
    filterModes[sv::NOTCH] = "Notch";
    filterModes[sv::PEAK] = "Peak";
    filterModes[sv::ALL] = "All Pass";
    paramDescriptions.push_back(
        ParamDesc()
            .asInt()
            .withID(pmSVFFilterMode)
            .withName("Multi-Mode Filter Type")
            .withGroupName("Filter")
            .withRange(PolysynthVoice::StereoSimperSVF::LP, PolysynthVoice::StereoSimperSVF::ALL)
            .withUnorderedMapFormatting(filterModes)
            .withFlags(steppedFlag));
    paramDescriptions.push_back(ParamDesc()
                                    .asPercentBipolar()
                                    .withID(pmSVFKeytrack)
                                    .withName("Multi-Mode Keytrack")
                                    .withGroupName("Multi-Mode Filter")
                                    .withFlags(modFlag)
                                    .withDefault(0));

    paramDescriptions.push_back(
        activeBase.withID(pmWSActive).withName("WaveShaper Active").withGroupName("WaveShaper"));
    paramDescriptions.push_back(ParamDesc()
                                    .asFloat()
                                    .asLinearDecibel(-24, 24)
                                    .withID(pmWSDrive)
                                    .withDefault(0)
                                    .withName("WaveShaper Drive")
                                    .withGroupName("WaveShaper")
                                    .withFlags(modFlag));
    paramDescriptions.push_back(ParamDesc()
                                    .asPercentBipolar()
                                    .withID(pmWSBias)
                                    .withDefault(0)
                                    .withName("WaveShaper Bias")
                                    .withGroupName("WaveShaper")
                                    .withFlags(modFlag));
    paramDescriptions.push_back(ParamDesc()
                                    .asInt()
                                    .withID(pmWSMode)
                                    .withDefault(1)
                                    .withRange(0, 5)
                                    .withName("WaveShaper Mode")
                                    .withGroupName("WaveShaper")
                                    .withFlags(steppedFlag)
                                    .withUnorderedMapFormatting({
                                        {PolysynthVoice::Waveshapers::Soft, "Soft"},
                                        {PolysynthVoice::Waveshapers::OJD, "OJD"},
                                        {PolysynthVoice::Waveshapers::Digital, "Digital"},
                                        {PolysynthVoice::Waveshapers::FullWaveRect, "Rectifier"},
                                        {PolysynthVoice::Waveshapers::WestcoastFold, "Fold"},
                                        {PolysynthVoice::Waveshapers::Fuzz, "Fuzz"},
                                    })); // FIXME enums

    paramDescriptions.push_back(
        ParamDesc()
            .asInt()
            .withID(pmFilterRouting)
            .withDefault(0)
            .withRange(0, 5)
            .withFlags(steppedFlag)
            .withName("Filter Routing")
            .withGroupName("Filters")
            .withUnorderedMapFormatting({{PolysynthVoice::WSLowMulti, "WS-LP-M"},
                                         {PolysynthVoice::LowMultiWS, "LP-M-WS"},
                                         {PolysynthVoice::LowWSMulti, "LP-WS-M"},
                                         {PolysynthVoice::MultiWSLow, "M-WS-LP"},
                                         {PolysynthVoice::WSPar, "WS-Par"},
                                         {PolysynthVoice::ParWS, "Par-WS"},
            }));
    paramDescriptions.push_back(ParamDesc()
                                    .asPercentBipolar()
                                    .withID(pmFilterFeedback)
                                    .withDefault(0)
                                    .withFlags(modFlag)
                                    .withName("Filter Feedback")
                                    .withGroupName("Filters"));

    auto adrBase = ParamDesc()
                       .asFloat()
                       .withRange(0.f, 1.f)
                       .withDefault(0.2f)
                       .withATwoToTheBPlusCFormatting(
                           1.f, PolysynthVoice::env_t::etMax - PolysynthVoice::env_t::etMin,
                           PolysynthVoice::env_t::etMin, "s")
                       .withDecimalPlaces(4);

    paramDescriptions.push_back(
        adrBase.withID(pmEnvA).withName("AEG Attack").withGroupName("AEG").withFlags(autoFlag));
    paramDescriptions.push_back(
        adrBase.withID(pmEnvD).withName("AEG Decay").withGroupName("AEG").withFlags(autoFlag));
    paramDescriptions.push_back(ParamDesc()
                                    .asPercent()
                                    .withID(pmEnvS)
                                    .withName("AEG Sustain")
                                    .withGroupName("AEG")
                                    .withDefault(1)
                                    .withFlags(modFlag));
    paramDescriptions.push_back(adrBase.withID(pmEnvR)
                                    .withName("AEG Release")
                                    .withGroupName("AEG")
                                    .withFlags(autoFlag)
                                    .withDefault(0.3));

    paramDescriptions.push_back(ParamDesc()
                                    .asPercent()
                                    .withID(pmAegVelocitySens)
                                    .withDefault(0.2)
                                    .withName("Velocity Sensitivity")
                                    .withGroupName("AEG")
                                    .withFlags(modFlag));

    paramDescriptions.push_back(ParamDesc()
                                    .asLinearDecibel(-24, 24)
                                    .withID(pmAegPreFilterGain)
                                    .withDefault(0.0)
                                    .withName("Pre-Filter Gain")
                                    .withGroupName("AEG")
                                    .withFlags(modFlag));

    paramDescriptions.push_back(adrBase.withID(pmEnvA + offPmFeg)
                                    .withName("FEG Attack")
                                    .withGroupName("FEG")
                                    .withFlags(autoFlag));
    paramDescriptions.push_back(adrBase.withID(pmEnvD + offPmFeg)
                                    .withName("FEG Decay")
                                    .withGroupName("FEG")
                                    .withFlags(autoFlag)
                                    .withDefault(0.25));
    paramDescriptions.push_back(ParamDesc()
                                    .asPercent()
                                    .withID(pmEnvS + offPmFeg)
                                    .withName("FEG Sustain")
                                    .withGroupName("FEG")
                                    .withFlags(autoFlag));
    paramDescriptions.push_back(adrBase.withID(pmEnvR + offPmFeg)
                                    .withName("FEG Release")
                                    .withGroupName("FEG")
                                    .withFlags(autoFlag));

    paramDescriptions.push_back(ParamDesc()
                                    .asFloat()
                                    .withID(pmFegToLPFCutoff)
                                    .withName("FEG to LPF Depth")
                                    .withGroupName("FEG")
                                    .withFlags(autoFlag)
                                    .withRange(-48, 48)
                                    .withDefault(0)
                                    .withLinearScaleFormatting("semitones"));

    paramDescriptions.push_back(ParamDesc()
                                    .asFloat()
                                    .withID(pmFegToSVFCutoff)
                                    .withName("FEG to Cutoff Depth")
                                    .withGroupName("FEG")
                                    .withFlags(autoFlag)
                                    .withRange(-48, 48)
                                    .withDefault(0)
                                    .withLinearScaleFormatting("semitones"));

    for (int i = 0; i < n_lfos; ++i)
    {
        auto nm = std::string("LFO ") + std::to_string(i + 1);
        paramDescriptions.push_back(ParamDesc()
                                        .asBool()
                                        .withID(pmLFOActive + i * offPmLFO2)
                                        .withName(nm + " Active")
                                        .withGroupName(nm)
                                        .withFlags(steppedFlag));

        paramDescriptions.push_back(ParamDesc()
                                        .asLfoRate()
                                        .withID(pmLFORate + i * offPmLFO2)
                                        .withName(nm + " Rate")
                                        .withGroupName(nm)
                                        .withFlags(modFlag));

        paramDescriptions.push_back(ParamDesc()
                                        .asBool()
                                        .withID(pmLFOTempoSync + i * offPmLFO2)
                                        .withName(nm + " Temposync")
                                        .withGroupName(nm)
                                        .withFlags(steppedFlag));

        addTemposyncActivator(pmLFORate + i * offPmLFO2, pmLFOTempoSync + i * offPmLFO2);

        paramDescriptions.push_back(ParamDesc()
                                        .asPercentBipolar()
                                        .withID(pmLFODeform + i * offPmLFO2)
                                        .withName(nm + " Deform")
                                        .withGroupName(nm)
                                        .withFlags(modFlag));
        paramDescriptions.push_back(ParamDesc()
                                        .asPercent()
                                        .withID(pmLFOAmplitude + i * offPmLFO2)
                                        .withName(nm + " Amplitude")
                                        .withGroupName(nm)
                                        .withFlags(modFlag)
                                        .withDefault(1.0));
        paramDescriptions.push_back(
            ParamDesc()
                .asInt()
                .withID(pmLFOShape + i * offPmLFO2)
                .withName(nm + " Shape")
                .withGroupName(nm)
                .withFlags(steppedFlag)
                .withRange(0, 5)
                .withUnorderedMapFormatting(
                    {{0, "Sin"}, {1, "Square"}, {2, "Saw"}, {3, "Tri"}, {4, "Noise"}, {5, "S&H"}}));
    }

    paramDescriptions.push_back(ParamDesc()
                                    .asPercentBipolar()
                                    .withID(pmVoicePan)
                                    .withName("Pan")
                                    .withGroupName("Voice")
                                    .withFlags(modFlag));
    paramDescriptions.push_back(ParamDesc()
                                    .asCubicDecibelAttenuation()
                                    .withID(pmVoiceLevel)
                                    .withName("Level")
                                    .withGroupName("Voice")
                                    .withFlags(modFlag)
                                    .withDefault(1.0));

    paramDescriptions.push_back(ParamDesc()
                                    .asBool()
                                    .withID(pmModFXActive)
                                    .withName("Mod FX Active")
                                    .withGroupName("Mod FX")
                                    .withFlags(autoFlag)
                                    .withDefault(1));
    paramDescriptions.push_back(ParamDesc()
                                    .asInt()
                                    .withID(pmModFXType)
                                    .withName("Mod FX Type")
                                    .withGroupName("Mod FX")
                                    .withFlags(autoFlag)
                                    .withRange(0, 1)
                                    .withDefault(0)
                                    .withUnorderedMapFormatting({{0, "Phaser"}, {1, "Flanger"}}));
    paramDescriptions.push_back(ParamDesc()
                                    .asInt()
                                    .withID(pmModFXPreset)
                                    .withName("Mod FX Preset")
                                    .withGroupName("Mod FX")
                                    .withFlags(autoFlag)
                                    .withRange(0, 2)
                                    .withDefault(0)
                                    .withUnorderedMapFormatting({{0, "I"}, {1, "II"}, {2, "III"}}));
    paramDescriptions.push_back(ParamDesc()
                                    .asLfoRate()
                                    .withRange(-6, log2(20))
                                    .withID(pmModFXRate)
                                    .withName("Mod FX Rate")
                                    .withGroupName("Mod FX")
                                    .withFlags(monoModFlag));
    paramDescriptions.push_back(ParamDesc()
                                    .asBool()
                                    .withID(pmModFXRateTemposync)
                                    .withName("Mod FX Rate Temposync")
                                    .withGroupName("Mod FX")
                                    .withFlags(autoFlag)
                                    .withDefault(0));
    addTemposyncActivator(pmModFXRate, pmModFXRateTemposync);

    paramDescriptions.push_back(ParamDesc()
                                    .asPercent()
                                    .withID(pmModFXMix)
                                    .withName("Mod FX Mix")
                                    .withGroupName("Mod FX")
                                    .withDefault(0.5)
                                    .withFlags(monoModFlag));

    paramDescriptions.push_back(ParamDesc()
                                    .asBool()
                                    .withID(pmRevFXActive)
                                    .withName("Reverb FX Active")
                                    .withGroupName("Reverb FX")
                                    .withFlags(autoFlag)
                                    .withDefault(0));

    paramDescriptions.push_back(ParamDesc()
                                    .asInt()
                                    .withID(pmRevFXPreset)
                                    .withName("Reverb Preset")
                                    .withGroupName("Reverb FX")
                                    .withFlags(autoFlag)
                                    .withRange(0, 2)
                                    .withDefault(0)
                                    .withUnorderedMapFormatting({{0, "I"}, {1, "II"}, {2, "III"}}));
    paramDescriptions.push_back(ParamDesc()
                                    .withRange(-4, 6)
                                    .withDefault(1.f)
                                    .withLog2SecondsFormatting()
                                    .withID(pmRevFXTime)
                                    .withName("Reverb Decay Time")
                                    .withGroupName("Reverb FX")
                                    .withFlags(monoModFlag));
    paramDescriptions.push_back(ParamDesc()
                                    .asPercent()
                                    .withID(pmRevFXMix)
                                    .withName("Reverb Mix")
                                    .withGroupName("Reverb FX")
                                    .withDefault(0.3)
                                    .withFlags(monoModFlag));

    paramDescriptions.push_back(ParamDesc()
                                    .asCubicDecibelAttenuation()
                                    .withID(pmOutputLevel)
                                    .withName("Output Level")
                                    .withGroupName("Global")
                                    .withFlags(monoModFlag)
                                    .withDefault(1.0));

    configureParams();

    terminatedVoices.reserve(max_voices * 4);

    clapJuceShim = std::make_unique<sst::clap_juce_shim::ClapJuceShim>(this);
    clapJuceShim->setResizable(true);

    mtsClient = MTS_RegisterClient();

    if (mtsClient)
    {
        if (MTS_HasMaster(mtsClient))
        {
            CNDOUT << "MTS: Client registered with " << MTS_GetScaleName(mtsClient) << std::endl;
        }
        else
        {
            CNDOUT << "MTS: Client present without available source" << std::endl;
        }
    }

    phaserFX = std::make_unique<PhaserFX>(this, this, this);
    phaserFX->initialize();

    flangerFX = std::make_unique<FlangerFX>(this, this, this);
    flangerFX->initialize();

    reverbFX = std::make_unique<ReverbFX>(this, this, this);
    reverbFX->initialize();

    for (auto &v : voices)
    {
        v.attachTo(*this);
    }

    patch.extension.initialize();
}
ConduitPolysynth::~ConduitPolysynth()
{
    if (mtsClient)
        MTS_DeregisterClient(mtsClient);

    // I *think* this is a bitwig bug that they won't call guiDestroy if destroying a plugin
    // with an open window but
    if (clapJuceShim)
        guiDestroy();
}

bool ConduitPolysynth::activate(double sampleRate, uint32_t minFrameCount,
                                uint32_t maxFrameCount) noexcept
{
    setSampleRate(sampleRate);
    for (auto &v : voices)
        v.setSampleRate(sampleRate * 2); // run voices oversampled
    phaserFX->onSampleRateChanged();
    flangerFX->onSampleRateChanged();
    reverbFX->onSampleRateChanged();
    return true;
}

/*
 * Stereo out, Midi in, in a pretty obvious way.
 * The only trick is the idi in also has NOTE_DIALECT_CLAP which provides us
 * with options on note expression and the like.
 */
bool ConduitPolysynth::audioPortsInfo(uint32_t index, bool isInput,
                                      clap_audio_port_info *info) const noexcept
{
    if (isInput || index != 0)
        return false;

    info->id = 0;
    info->in_place_pair = CLAP_INVALID_ID;
    strncpy(info->name, "main", sizeof(info->name));
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = 2;
    info->port_type = CLAP_PORT_STEREO;

    return true;
}

bool ConduitPolysynth::notePortsInfo(uint32_t index, bool isInput,
                                     clap_note_port_info *info) const noexcept
{
    if (isInput)
    {
        info->id = 1;
        info->supported_dialects = CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_CLAP;
        info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
        strncpy(info->name, "NoteInput", CLAP_NAME_SIZE - 1);
        return true;
    }
    return false;
}

/*
 * The process function is the heart of any CLAP. It reads inbound events,
 * generates audio if appropriate, writes outbound events, and informs the host
 * to continue operating.
 *
 * In the ConduitPolysynth, our process loop has 3 basic stages
 *
 * 1. See if the UI has sent us any events on the thread-safe UI Queue (
 *    see the discussion in the clap header file for this structure), apply them
 *    to my internal state, and generate CLAP changed messages
 *
 * 2. Iterate over samples rendering the voices, and if an inbound event is coincident
 *    with a sample, process that event for note on, modulation, parameter automation, and so on
 *
 * 3. Detect any voices which have terminated in the block (their state has become 'NEWLY_OFF'),
 *    update them to 'OFF' and send a CLAP NOTE_END event to terminate any polyphonic modulators.
 */
clap_process_status ConduitPolysynth::process(const clap_process *process) noexcept
{
    // If I have no outputs, do nothing
    if (process->audio_outputs_count <= 0)
        return CLAP_PROCESS_SLEEP;

    /*
     * Stage 1:
     *
     * The UI can send us gesture begin/end events which translate in to a
     * `clap_event_param_gesture` or value adjustments.
     */
    auto ct = handleEventsFromUIQueue(process->out_events);
    if (ct)
        pushParamsToVoices();

    /*
     * Stage 2: Create the AUDIO output and process events
     *
     * CLAP has a single inbound event loop where every event is time stamped with
     * a sample id. This means the process loop can easily interleave note and parameter
     * and other events with audio generation. Here we do everything completely sample accurately
     * by maintaining a pointer to the 'nextEvent' which we check at every sample.
     */
    float **out = process->audio_outputs[0].data32;
    auto chans = process->audio_outputs->channel_count;
    if (chans != 2)
    {
        return CLAP_PROCESS_SLEEP;
    }

    auto ev = process->in_events;
    auto sz = ev->size(ev);

    // This pointer is the sentinel to our next event which we advance once an event is processed
    const clap_event_header_t *nextEvent{nullptr};
    uint32_t nextEventIndex{0};
    if (sz != 0)
    {
        nextEvent = ev->get(ev, nextEventIndex);
    }

    bool modActive = *paramToValue[pmModFXActive] > 0.5;
    bool revActive = *paramToValue[pmRevFXActive] > 0.5;
    bool usePhaser = *paramToValue[pmModFXType] < 0.5;

    for (auto i = 0U; i < process->frames_count; ++i)
    {
        // Do I have an event to process. Note that multiple events
        // can occur on the same sample, hence 'while' not 'if'
        while (nextEvent && nextEvent->time == i)
        {
            // handleInboundEvent is a separate function which adjusts the state based
            // on event type. We segregate it for clarity but you really should read it!
            handleInboundEvent(nextEvent);
            nextEventIndex++;
            if (nextEventIndex >= sz)
                nextEvent = nullptr;
            else
                nextEvent = ev->get(ev, nextEventIndex);
        }

        if (blockPos == 0)
        {
            renderVoices();
            if (modActive)
            {
                if (usePhaser)
                {
                    phaserFX->processBlock(output[0], output[1]);
                }
                else
                {
                    flangerFX->processBlock(output[0], output[1]);
                }
            }
            if (revActive)
            {
                reverbFX->processBlock(output[0], output[1]);
            }
        }
        out[0][i] = output[0][blockPos];
        out[1][i] = output[1][blockPos];

        blockPos = (blockPos + 1) & (PolysynthVoice::blockSize - 1);
    }

    /*
     * Stage 3 is to inform the host of our terminated voices.
     *
     * This allows hosts which support polyphonic modulation to terminate those
     * modulators, and it is also the reason we have the NEWLY_OFF state in addition
     * to the OFF state.
     *
     * Note that there are two ways to enter the terminatedVoices array. The first
     * is here through natural state transition to NEWLY_OFF and the second is in
     * handleNoteOn when we steal a voice.
     */
    for (auto &v : voices)
    {
        if (v.active && !v.isPlaying())
        {
            terminatedVoices.emplace_back(v.portid, v.channel, v.key, v.note_id);
            v.active = false;
            voiceEndCallback(&v);
        }
    }

    for (const auto &[portid, channel, key, note_id] : terminatedVoices)
    {
        auto ov = process->out_events;
        auto evt = clap_event_note();
        evt.header.size = sizeof(clap_event_note);
        evt.header.type = (uint16_t)CLAP_EVENT_NOTE_END;
        evt.header.time = process->frames_count - 1;
        evt.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
        evt.header.flags = 0;

        evt.port_index = portid;
        evt.channel = channel;
        evt.key = key;
        evt.note_id = note_id;
        evt.velocity = 0.0;

        ov->try_push(ov, &(evt.header));

        uiComms.dataCopyForUI.updateCount++;
        uiComms.dataCopyForUI.polyphony--;
    }
    terminatedVoices.clear();

    // We should have gotten all the events
    assert(!nextEvent);

    return CLAP_PROCESS_CONTINUE;
}

void ConduitPolysynth::renderVoices()
{
    memset(outputOS, 0, sizeof(outputOS));
    for (auto &v : voices)
    {
        if (v.isPlaying())
        {
            v.processBlock();
            sst::basic_blocks::mechanics::accumulate_from_to<PolysynthVoice::blockSizeOS>(
                v.outputOS[0], outputOS[0]);
            sst::basic_blocks::mechanics::accumulate_from_to<PolysynthVoice::blockSizeOS>(
                v.outputOS[1], outputOS[1]);
        }
    }

    hr_dn.process_block_D2(outputOS[0], outputOS[1], blockSize, output[0], output[1]);
}

/*
 * handleInboundEvent provides the core event mechanism including
 * voice activation and deactivation, parameter modulation, note expression,
 * and so on.
 *
 * It reads, unsurprisingly, as a simple switch over type with reactions.
 */
void ConduitPolysynth::handleInboundEvent(const clap_event_header_t *evt)
{
    if (handleParamBaseEvents(evt))
    {
        pushParamsToVoices();
        return;
    }

    if (evt->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;

    switch (evt->type)
    {
    case CLAP_EVENT_MIDI:
    {
        /*
         * We advertise both CLAP_DIALECT_MIDI and CLAP_DIALECT_CLAP_NOTE so we do need
         * to handle midi events. CLAP just gives us MIDI 1 (or 2 if you want, but I didn't code
         * that) streams to do with as you wish. The CLAP_MIDI_EVENT here does the obvious thing.
         */
        auto mevt = reinterpret_cast<const clap_event_midi *>(evt);
        auto msg = mevt->data[0] & 0xF0;
        auto chan = mevt->data[0] & 0x0F;
        switch (msg)
        {
        case 0x90:
        {
            if (mevt->data[2] == 0)
            {
                voiceManager.processNoteOffEvent(mevt->port_index, chan, mevt->data[1], -1,
                                                 voiceManager.midiToFloatVelocity(mevt->data[2]));
            }
            else
            {
                // Hosts should prefer CLAP_NOTE events but if they don't
                voiceManager.processNoteOnEvent(mevt->port_index, chan, mevt->data[1], -1,
                                                voiceManager.midiToFloatVelocity(mevt->data[2]),
                                                0.f);
            }
            break;
        }
        case 0x80:
        {
            // Hosts should prefer CLAP_NOTE events but if they don't
            voiceManager.processNoteOffEvent(mevt->port_index, chan, mevt->data[1], -1,
                                             voiceManager.midiToFloatVelocity(mevt->data[2]));
            break;
        }
        case 0xA0:
        {
            break;
        }
        case 0xB0:
        {
            if (mevt->data[1] == 64)
            {
                voiceManager.updateSustainPedal(mevt->port_index, chan, mevt->data[2]);
            }
            CNDOUT << "Controller " << (int)mevt->data[1] << std::endl;
            break;
        }
        case 0xE0:
        {
            // pitch bend
            auto bv = mevt->data[1] + mevt->data[2] * 128;

            voiceManager.routeMIDIPitchBend(mevt->port_index, chan, bv);

            break;
        }
        }
        break;
    }
    /*
     * CLAP_EVENT_NOTE_ON and OFF simply deliver the event to the note creators below,
     * which find (probably) and activate a spare or playing voice. Our 'voice stealing'
     * algorithm here is 'just don't play a note 65 if 64 are ringing. Remember this is an
     * example synth!
     */
    case CLAP_EVENT_NOTE_ON:
    {
        auto nevt = reinterpret_cast<const clap_event_note *>(evt);
        voiceManager.processNoteOnEvent(nevt->port_index, nevt->channel, nevt->key, nevt->note_id,
                                        nevt->velocity, 0.f);
    }
    break;
    case CLAP_EVENT_NOTE_OFF:
    {
        auto nevt = reinterpret_cast<const clap_event_note *>(evt);
        voiceManager.processNoteOffEvent(nevt->port_index, nevt->channel, nevt->key, nevt->note_id,
                                         nevt->velocity);
    }
    break;
    /*
     * CLAP_EVENT_PARAM_VALUE sets a value. What happens if you change a parameter
     * outside a modulation context. We simply update our engine value and, if an editor
     * is attached, send an editor message.
     */
    case CLAP_EVENT_PARAM_VALUE:
    {
        auto v = reinterpret_cast<const clap_event_param_value *>(evt);
        updateParamInPatch(v);
        pushParamsToVoices();
    }
    break;
    /*
     * CLAP_EVENT_PARAM_MOD provides both monophonic and polyphonic modulation.
     * We do this by seeing which parameter is modulated then adjusting the
     * side-by-side modulation values in a voice.
     */
    case CLAP_EVENT_PARAM_MOD:
    {
        auto pevt = reinterpret_cast<const clap_event_param_mod *>(evt);

        voiceManager.routePolyphonicParameterModulation(pevt->port_index, pevt->channel, pevt->key,
                                                        pevt->note_id, pevt->param_id,
                                                        pevt->amount);
    }
    break;
    /*
     * Note expression handling is similar to polymod. Traverse the voices - in note expression
     * indexed by channel / key / port - and adjust the modulation slot in each.
     */
    case CLAP_EVENT_NOTE_EXPRESSION:
    {
        auto pevt = reinterpret_cast<const clap_event_note_expression *>(evt);
        voiceManager.routeNoteExpression(pevt->port_index, pevt->channel, pevt->key, pevt->note_id,
                                         pevt->expression_id, pevt->value);
    }
    break;
    }
}

PolysynthVoice *ConduitPolysynth::initializeVoice(uint16_t port, uint16_t channel, uint16_t key,
                                                  int32_t noteId, float velocity, float retune)
{
    for (auto &v : voices)
    {
        if (!v.active)
        {
            activateVoice(v, port, channel, key, noteId, velocity);

            if (clapJuceShim->isEditorAttached())
            {
                auto r = ToUI();
                r.type = ToUI::MIDI_NOTE_ON;
                r.id = (uint32_t)key;
                uiComms.toUiQ.push(r);
            }

            return &v;
        }
    }
    return nullptr;
}

void ConduitPolysynth::releaseVoice(PolysynthVoice *sdv, float velocity)
{
    if (sdv)
    {
        sdv->release();

        if (clapJuceShim->isEditorAttached())
        {
            auto r = ToUI();
            r.type = ToUI::MIDI_NOTE_OFF;
            r.id = (uint32_t)sdv->key;
            uiComms.toUiQ.push(r);
        }
    }
}

void ConduitPolysynth::activateVoice(PolysynthVoice &v, int port_index, int channel, int key,
                                     int noteid, double velocity)
{
    v.start(port_index, channel, key, noteid, velocity);
}

/*
 * If the processing loop isn't running, the call to requestParamFlush from the UI will
 * result in this being called on the main thread, and generating all the appropriate
 * param updates.
 */
void ConduitPolysynth::paramsFlush(const clap_input_events *in,
                                   const clap_output_events *out) noexcept
{
    auto sz = in->size(in);

    // This pointer is the sentinel to our next event which we advance once an event is processed
    for (auto e = 0U; e < sz; ++e)
    {
        auto nextEvent = in->get(in, e);
        handleInboundEvent(nextEvent);
    }

    auto ct = handleEventsFromUIQueue(out);

    if (ct)
        pushParamsToVoices();

    // We will never generate a note end event with processing active, and we have no midi
    // output, so we are done.
}

void ConduitPolysynth::pushParamsToVoices() {}

void ConduitPolysynthConfig::PatchExtension::initialize()
{
    modMatrixConfig = std::make_unique<ModMatrixConfig>();
}

void ConduitPolysynth::handleSpecializedFromUI(const FromUI &r)
{
    CNDOUT << "handleSpecialized " << (int)r.type << std::endl;
}
} // namespace sst::conduit::polysynth
