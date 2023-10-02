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

#ifndef CONDUIT_SRC_POLYSYNTH_EFFECTS_IMPL_H
#define CONDUIT_SRC_POLYSYNTH_EFFECTS_IMPL_H

#include "polysynth.h"

namespace sst::conduit::polysynth
{
namespace details
{
struct FXUtilityBase
{
    ConduitPolysynth *synth{nullptr};
    FXUtilityBase(ConduitPolysynth *p, ConduitPolysynth *, ConduitPolysynth *) : synth(p) {}
};
} // namespace details

struct SharedConfig
{
    using BaseClass = details::FXUtilityBase;
    using GlobalStorage = ConduitPolysynth;
    using EffectStorage = ConduitPolysynth;
    using BiquadAdapter = SharedConfig;
    using ValueStorage = ConduitPolysynth;
    static constexpr int blockSize{PolysynthVoice::blockSize};

    static float envelopeRateLinear(GlobalStorage *g, float f)
    {
        return blockSize * sampleRateInv(g) * pow(2.f, -f);
    }
    static bool isDeactivated(EffectStorage *, int) { return false; }
    static bool isExtended(EffectStorage *, int) { return false; }
    static float rand01(GlobalStorage *) { return 0; }
    static double sampleRate(GlobalStorage *g) { return g->sampleRate; }
    static double sampleRateInv(GlobalStorage *g) { return g->sampleRateInv; }
    static float noteToPitch(GlobalStorage *g, float note)
    {
        return g->note_to_pitch_ignoring_tuning(note);
    }
    static float noteToPitchInv(GlobalStorage *g, float note) { return 1.f / noteToPitch(g, note); }
    static float noteToPitchIgnoringTuning(GlobalStorage *g, float note)
    {
        return g->note_to_pitch_ignoring_tuning(note);
    }
    static float dbToLinear(GlobalStorage *g, float val) { return g->dbToLinear(val); }
};

struct PhaserConfig : SharedConfig
{
    static constexpr std::array<std::array<double, 12>, 3> presets{
        {{-0.35, -0.75, 0.75, 1.32193, 1, 0.493306, 0.5, 0, 6, 0.35, 3, 0},
         {0, -0.2, 1, 0, 1, 0.5, 0.5, 0, 4, 0.4, 1, 0},
         {-0.75, 0.797292, -0.3, -1, 0.994367, 1, 0.4, 0, 2, 0.7, 0, 0}}};

    static int presetIndex(const BaseClass *bc)
    {
        auto s = bc->synth;
        auto pn = s->paramToValue[ConduitPolysynth::pmModFXPreset];
        return (int)std::round(*pn);
    }

    static float temposyncRatio(GlobalStorage *g, EffectStorage *, int) { return 1.; }

    static float floatValueAt(const BaseClass *bc, const ValueStorage *, int idx)
    {
        if (idx == PhaserFX::ph_mix)
        {
            return *(bc->synth->paramToValue[ConduitPolysynth::pmModFXMix]);
        }
        return presets[presetIndex(bc)][idx];
    }
    static int intValueAt(const BaseClass *bc, const ValueStorage *, int idx)
    {
        return (int)presets[presetIndex(bc)][idx];
    }
};

struct FlangerConfig : SharedConfig
{
    static constexpr std::array<std::array<double, 11>, 3> presets{
        {{0, 3, 0.892556, 0.725957, 2, 65, 4.86, 0.12, 0, 0, 0.5},
         {0, 1, 0, 0.415452, 3, 70, 10.6554, 0.18, 0.12, 0, 0.7},
         {2, 0, -.241504, 0.737768, 4, 55, 8, 0.72, 0.2, 0, -0.9}}};

    static float temposyncRatio(GlobalStorage *g, EffectStorage *, int) { return 1.; }

    static int presetIndex(const BaseClass *bc)
    {
        auto s = bc->synth;
        auto pn = s->paramToValue[ConduitPolysynth::pmModFXPreset];
        return (int)std::round(*pn);
    }
    static float floatValueAt(const BaseClass *bc, const ValueStorage *, int idx)
    {
        if (idx == FlangerFX ::fl_mix)
        {
            return *(bc->synth->paramToValue[ConduitPolysynth::pmModFXMix]);
        }
        return presets[presetIndex(bc)][idx];
    }
    static int intValueAt(const BaseClass *bc, const ValueStorage *, int idx)
    {
        return (int)presets[presetIndex(bc)][idx];
    }
};

struct Reverb1Config : SharedConfig
{
    static constexpr std::array<std::array<double, 11>, 3> presets{
        {{-6.59756, 2, 0.346016, 0.921308, 0.247126, -21.462, -12.3269, -7.86461, 70, 0.221484, 0},
         {-5.74933, 1, 0.953453, 1.38905, 0.0735602, -25.0596, -6.88824, 1.06879, 70, 0.213031,
          3.2536},
         {-8, 3, 0.445348, 1.61731, 0.444403, -21.3982, 14.0079, 14.4087, 70, 0.193794, 0}}};

    static float temposyncRatio(GlobalStorage *g, EffectStorage *, int) { return 1.; }

    static int presetIndex(const BaseClass *bc)
    {
        auto s = bc->synth;
        auto pn = s->paramToValue[ConduitPolysynth::pmRevFXPreset];
        return (int)std::round(*pn);
    }

    static float floatValueAt(const BaseClass *bc, const ValueStorage *, int idx)
    {
        if (idx == ReverbFX ::rev1_mix)
        {
            return *(bc->synth->paramToValue[ConduitPolysynth::pmRevFXMix]);
        }
        return presets[presetIndex(bc)][idx];
    }
    static int intValueAt(const BaseClass *bc, const ValueStorage *, int idx)
    {
        return (int)presets[presetIndex(bc)][idx];
    }
};
} // namespace sst::conduit::polysynth

#endif // CONDUIT_EFFECTS_IMPL_H
