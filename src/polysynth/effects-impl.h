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
struct PhaserConfig
{
    using BaseClass = ConduitPolysynth;
    using GlobalStorage = ConduitPolysynth;
    using EffectStorage = ConduitPolysynth;
    using BiquadAdapter = ConduitPolysynth;
    using ValueStorage = ConduitPolysynth;
    static constexpr int blockSize{PolysynthVoice::blockSize};

    static float floatValueAt(const BaseClass *, const ValueStorage *, int idx) { return 0; }
    static int intValueAt(const BaseClass *, const ValueStorage *, int idx) { return 0; }

    static float envelopeRateLinear(GlobalStorage *g, float f)
    {
        return blockSize * sampleRateInv(g) * pow(2.f, -f);
    }
    static float temposyncRatio(GlobalStorage *g, EffectStorage *, int) { return 1.; }
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
} // namespace sst::conduit::polysynth

#endif // CONDUIT_EFFECTS_IMPL_H
