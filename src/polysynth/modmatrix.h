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

#ifndef CONDUIT_SRC_POLYSYNTH_MODMATRIX_H
#define CONDUIT_SRC_POLYSYNTH_MODMATRIX_H

#include <unordered_map>

namespace sst::conduit::polysynth
{
template <typename SourceID, typename TargetID, typename CurveID> struct ModMatrix
{
    using matrix_t = ModMatrix<SourceID, TargetID, CurveID>;

    std::unordered_map<SourceID, float *> sourceValues;
    std::unordered_map<TargetID, float *> targetValues;

    struct EntryDescription
    {
        SourceID source;
        SourceID via;
        CurveID curveBy;
        float depth;
        TargetID target;
    };

    struct RealizedEntry
    {
        float *source;
        float *via;
        float *target;
        float depth;

        void fromEntryDescription(const matrix_t &mat, EntryDescription &d)
        {
            {
                auto se = mat.sourceValues.find(d.source);
                if (se == mat.sourceValues.end())
                    source = nullptr;
                else
                    source = se->second;
            }
            {
                auto ve = mat.sourceValues.find(d.via);
                if (ve == mat.sourceValues.end())
                    via = nullptr;
                else
                    via = ve->second;
            }
            {
                auto te = mat.targetValues.find(d.target);
                if (te == mat.targetValues.end())
                    target = nullptr;
                else
                    target = te->second;
            }
            depth = d.depth;
        }

        void apply()
        {
            if (!target)
                return;
            if (!source)
                return;
            auto v = *source * depth;
            if (via)
                v *= *via;
            *target = v;
        }
    };
};
} // namespace sst::conduit::polysynth

#endif // CONDUIT_MODMATRIX_H
