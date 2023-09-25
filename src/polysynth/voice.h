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

#ifndef CONDUIT_SRC_POLYSYNTH_SAW_VOICE_H
#define CONDUIT_SRC_POLYSYNTH_SAW_VOICE_H

#include <array>
#include <random>
#include <unordered_map>
#include <clap/clap.h>

#include "conduit-shared/debug-helpers.h"
#include "conduit-shared/sse-include.h"

#include "sst/basic-blocks/dsp/DPWSawPulseOscillator.h"
#include "sst/basic-blocks/dsp/QuadratureOscillators.h"
#include "sst/basic-blocks/modulators/ADSREnvelope.h"
#include "sst/basic-blocks/dsp/BlockInterpolators.h"

struct MTSClient;

namespace sst::conduit::polysynth
{

struct ConduitPolysynth;

struct PolysynthVoice
{
    static constexpr int max_uni{7};
    static constexpr int blockSize{8};
    static constexpr int blockSizeOS{blockSize << 1};

    const ConduitPolysynth &synth;
    PolysynthVoice(const ConduitPolysynth &sy) : synth(sy), gen((uint64_t)(this)), urd(-1.0, 1.0), aeg(this), feg(this)  {}

    void setSampleRate(double sr)
    {
        samplerate = sr;
        aeg.onSampleRateChanged();
        feg.onSampleRateChanged();
    }

    int portid;  // clap note port index
    int channel; // midi channel
    int key;     // The midi key which triggered me
    int note_id; // and the note_id delivered by the host (used for note expressions)

    MTSClient *mtsClient{nullptr};
    void attachTo(ConduitPolysynth &p);

    struct ModulatedValue
    {
        float *base{nullptr};
        float *externalMod{nullptr};
        float *internalMod{nullptr};

        inline float value()
        {
            assert(base);
            assert(internalMod);
            assert(externalMod);
            return *base + *externalMod + *internalMod;
        }
    };

    std::unordered_map<clap_id, float> externalMods, internalMods;

    void applyExternalMod(clap_id param, float value);

    // Saw Oscillator
    int sawUnison{3};
    bool sawActive{true};
    ModulatedValue sawUnisonDetune, sawCoarse, sawFine, sawLevel;
    sst::basic_blocks::dsp::lipol<float, blockSizeOS, true> sawLevel_lipol;
    std::array<float, max_uni> sawUniPanL, sawUniPanR, sawUniVoiceDetune, sawUniLevelNorm;
    std::array<sst::basic_blocks::dsp::DPWSawOscillator<
                   sst::basic_blocks::dsp::BlockInterpSmoothingStrategy<blockSize>>,
               max_uni>
        sawOsc;

    // Pulse Oscillator
    bool pulseActive{true};
    ModulatedValue pulseWidth, pulseOctave, pulseCoarse, pulseFine, pulseLevel;
    sst::basic_blocks::dsp::lipol<float, blockSizeOS, true> pulseLevel_lipol;
    sst::basic_blocks::dsp::DPWPulseOscillator<sst::basic_blocks::dsp::BlockInterpSmoothingStrategy<blockSize>> pulseOsc;

    // Sin Oscillator
    bool sinActive{true};
    ModulatedValue sinOctave, sinCoarse, sinLevel;
    sst::basic_blocks::dsp::lipol<float, blockSizeOS, true> sinLevel_lipol;
    sst::basic_blocks::dsp::QuadratureOscillator<float> sinOsc;

    // Noise Source
    bool noiseActive{true};
    ModulatedValue noiseColor, noiseLevel;
    sst::basic_blocks::dsp::lipol<float, blockSizeOS, true> noiseLevel_lipol;
    float w0, w1;
    std::default_random_engine gen;
    std::uniform_real_distribution<float> urd;


    sst::basic_blocks::dsp::lipol_sse<blockSizeOS, true> aegPFG_lipol;
    ModulatedValue aegPFG;

    bool svfActive;
    int svfMode{StereoSimperSVF::Mode::LP};
    ModulatedValue svfCutoff, svfResonance;

    struct EnvValue
    {
        ModulatedValue attack, decay, sustain, release;
    } aegValues, fegValues;
    ModulatedValue fegToSvfCutoff;

    // Two values can modify pitch, the note expression and the bend wheel.
    // After adjusting these, call 'recalcPitch'
    float pitchNoteExpressionValue{0.f}, pitchBendWheel{0.f};

    // Finally, please set my sample rate at voice on. Thanks!
    float samplerate{0};

    using env_t = sst::basic_blocks::modulators::ADSREnvelope<PolysynthVoice, blockSizeOS>;
    env_t aeg, feg;
    bool gated{false};
    bool active{false};

    void processBlock();

    float outputOS alignas(16)[2][blockSizeOS];

    void start(int16_t port, int16_t channel, int16_t key, int32_t noteid);
    void release();

    void recalcPitch();
    void recalcFilter();

    void receiveNoteExpression(int expression, double value);

    // Sigh - fix this to a table of course
    inline float envelope_rate_linear_nowrap(float f) { return blockSizeOS * srInv * pow(2.f, -f); }

    inline bool isPlaying() const { return aeg.stage < env_t::s_eoc; }

    struct StereoSimperSVF // thanks to urs @ u-he and andy simper @ cytomic
    {
        float ic1eq[2]{0.f, 0.f}, ic2eq[2]{0.f, 0.f};
        float g{0.f}, k{0.f}, gk{0.f}, a1{0.f}, a2{0.f}, a3{0.f}, ak{0.f};
        enum Mode
        {
            LP,
            HP,
            BP,
            NOTCH,
            PEAK,
            ALL
        } mode{LP};

        float low[2], band[2], high[2], notch[2], peak[2], all[2];
        void setCoeff(float key, float res, float srInv);
        void step(float &L, float &R);
        void init();
    } svfImpl;

  private:
    double baseFreq{440.0};
    double srInv{1.0 / 44100.0};
};
} // namespace sst::conduit::polysynth
#endif
