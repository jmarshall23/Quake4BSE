﻿/*
===========================================================================

QUAKE 4 BSE CODE RECREATION EFFORT - (c) 2025 by Justin Marshall(IceColdDuke).

QUAKE 4 BSE CODE RECREATION EFFORT is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QUAKE 4 BSE CODE RECREATION EFFORT is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QUAKE 4 BSE CODE RECREATION EFFORT.  If not, see <http://www.gnu.org/licenses/>.

In addition, the QUAKE 4 BSE CODE RECREATION EFFORT is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "bse.h"


/*
===============================================================================

    Lifetime helpers
    (small ones are placed first so they are easy to inline)

===============================================================================
*/
bool rvParticleTemplate::UsesEndOrigin(void) const {
    // Uses the line segment “origin -> endOrigin” for spawn / length domains?
    return (mSpawnPosition.mFlags & 2) != 0 ||
        (mSpawnLength.mFlags & 2) != 0;
}

/*
===============================================================================

    Parameter-count derivation  ( size / rotate envelopes )

    mType table
    ---------------------------------------------------------
      0 = sprite/quad    1 = beam          2 = billboard
      3 = axis-aligned   4 = ribbon        5 = ribbon-extruded
      6 = mesh-copy      7 = mesh-random   8 = decal
      9 = point-light “glare” helper
===============================================================================
*/
void rvParticleTemplate::SetParameterCounts(void) {
    int sizeParms = 0;
    int rotateParms = 0;
    int sizeSpawn = 7;   // default = “vector” envelope
    /*---------------------------------------------------------------------
        The original x86 code used two packed bit-fields:
            lower 2 bits – dimension count (0,1,2,3 axes)
            upper bits   – shape variants (point, unit, scaled, etc.)
    ---------------------------------------------------------------------*/
    switch (mType) {
    case 1:                     // beam
    case 4:                     // ribbon
        sizeParms = 2;
        rotateParms = 1;
        sizeSpawn = 6;        // two-component envelope
        break;

    case 2:                     // billboard
    case 7:                     // mesh-random
    case 8:                     // decal
        sizeParms = 1;
        rotateParms = 0;
        sizeSpawn = 5;        // one-component envelope
        break;

    case 3:                     // axis-aligned (cone/cylinder)
        sizeParms = 2;
        rotateParms = 3;
        sizeSpawn = 6;
        break;

    case 5:                     // ribbon-extruded
        sizeParms = 3;
        rotateParms = 3;
        sizeSpawn = 7;
        break;

    case 6:                     // mesh-copy
        sizeParms = 3;
        rotateParms = 0;
        sizeSpawn = 7;
        break;

    case 9:                     // glare helper
        sizeParms = 0;
        rotateParms = 3;
        sizeSpawn = 7;
        break;

    default:                    // sprites (0) – counts already correct
        return;
    }

    mNumSizeParms = sizeParms;
    mNumRotateParms = rotateParms;

    // Preserve editor intent – spawn/death domains must carry the same
    // “shape” meta-information the envelope expects.
    mSpawnSize.mSpawnType = sizeSpawn;
    mDeathSize.mSpawnType = sizeSpawn;
    mSpawnRotate.mSpawnType = rotateParms;
    mDeathRotate.mSpawnType = rotateParms;
}

/*
===============================================================================

    Runtime helpers
===============================================================================
*/
float rvParticleTemplate::GetSpawnVolume(rvBSE* fx) const {
    // Compute the effective diagonal extents ( ≈ bounding-box diagonal ).
    float xExtent;

    if (mSpawnPosition.mFlags & 2 /*END_ORIGIN*/) {
        idVec3 delta = fx->mOriginalEndOrigin - fx->mOriginalOrigin;
        xExtent = delta.Length() - mSpawnPosition.mMins.x;
    }
    else {
        xExtent = mSpawnPosition.mMaxs.x - mSpawnPosition.mMins.x;
    }

    const float yExtent = mSpawnPosition.mMaxs.y - mSpawnPosition.mMins.y;
    const float zExtent = mSpawnPosition.mMaxs.z - mSpawnPosition.mMins.z;

    // Historical: the 0.01 factor dates back to the old “cm” → “m” scale.
    return (xExtent + yExtent + zExtent) * 0.01f;
}

// --------------------------------------------------------------------------

float rvParticleTemplate::CostTrail(float baseCost) const {
    switch (mTrailType) {
    case 1: // burn
        return baseCost * mTrailCount.y * 2.0f;

    case 2: // motion blur
        return baseCost * mTrailCount.y * 1.5f + 20.0f;

    default:
        return baseCost;
    }
}

/*
===============================================================================

    Parameter sanitiser
    (rewrites spawnType so the renderer can make fast decisions)

===============================================================================
*/
void rvParticleTemplate::FixupParms(rvParticleParms& p) {
    const int axisBits = p.mSpawnType & 3;      // 0..3
    const int shapeBits = (p.mSpawnType & ~3);    // 0,4,8,(...)  etc.

    // Explicit editor values “point” (0), “unit” (4) and the two tapered
    // cone/cylinder variants (43,47) are left untouched.
    if (shapeBits == 0 || shapeBits == 4 ||
        p.mSpawnType == 43 || p.mSpawnType == 47) {
        return;
    }

    // ---------------------------------------------------------------------
    //  Detect degenerate cases where mins == maxs on the active axes.
    //  These collapse to either POINT / UNIT / SCALE depending on size.
    // ---------------------------------------------------------------------
    const idVec3& mins = p.mMins;
    const idVec3& maxs = p.mMaxs;

    const bool equalX = (maxs.x == mins.x);
    const bool equalY = (axisBits < 2) || (maxs.y == mins.x);
    const bool equalZ = (axisBits != 3) || (maxs.z == mins.x);

    if (shapeBits == 8 && equalX && equalY && equalZ) {      // a “box”
        if (mins.x == 0.0f) {
            p.mSpawnType = axisBits;           // true POINT
        }
        else if (idMath::Fabs(mins.x - 1.0f) < idMath::FLT_EPSILON) {
            p.mSpawnType = axisBits + 4;       // UNIT-sized
        }
        else {
            p.mSpawnType = axisBits + 8;       // general SCALE
        }
    }
    else if (shapeBits == 8) {
        // Mixed extents – still a SCALE box, but make sure the type’s
        // “shape” bits say so even when the editor didn’t.
        p.mSpawnType = axisBits + 8;
    }

    // ---------------------------------------------------------------------
    //  Remove unused components ( renderer relies on 0 to skip work )
    // ---------------------------------------------------------------------
    if (p.mSpawnType >= 8) {
        if (axisBits == 1) {                 // X/Y only
            p.mMins.y = p.mMaxs.y = 0.0f;
        }
        else if (axisBits == 2) {          // X/Z only
            p.mMins.z = p.mMaxs.z = 0.0f;
        }
    }
    else {                                   // pure POINT / UNIT
        p.mMins = vec3_origin;
        p.mMaxs = vec3_origin;
    }

    // Enforce maxs == mins for POINT / UNIT / SCALE
    if (p.mSpawnType <= 11) {
        p.mMaxs = p.mMins;
    }

    // If this parameter references endOrigin, upgrade it to the
    // corresponding “vector” (12-15) shape so the renderer knows it varies.
    if ((p.mFlags & 2) && p.mSpawnType <= 12) {
        p.mSpawnType = axisBits + 12;
    }
}

/*
===============================================================================

    Master initialiser  (called by the default ctor)

===============================================================================
*/
void rvParticleTemplate::Init(void) {
    // ---------------------------------------------------------------------
    //  Basic meta
    // ---------------------------------------------------------------------
    mFlags = 0;
    mType = 0;

    mMaterialName = "_default";
    mMaterial = declManager->FindMaterial("_default", false);

    mModelName = "_default";
    mTraceModelIndex = -1;

    mGravity.Zero();
    mSoundVolume.Zero();
    mFreqShift.Zero();
    mDuration.Set(0.002f, 0.002f);

    mBounce = 0.0f;
    mTiling = 8.0f;

    // ---------------------------------------------------------------------
    //  Trail parameters
    // ---------------------------------------------------------------------
    mTrailType = 0;
    mTrailMaterial = declManager->FindMaterial(
        "gfx/effects/particles_shapes/motionblur", false);
    mTrailTime.Zero();
    mTrailCount.Zero();

    // ---------------------------------------------------------------------
    //  Fork / jitter
    // ---------------------------------------------------------------------
    mNumForks = 0;
    mForkSizeMins.Set(-20, -20, -20);
    mForkSizeMaxs = -mForkSizeMins;
    mJitterSize.Set(3, 7, 7);
    mJitterRate = 0.0f;
    mJitterTable = static_cast<const idDeclTable*>(
        declManager->FindType(DECL_TABLE,
            "halfsintable",
            false));

    // ---------------------------------------------------------------------
    //  Derived constants
    // ---------------------------------------------------------------------
    mNumSizeParms = 2;
    mNumRotateParms = 1;
    mVertexCount = 4;
    mIndexCount = 6;
    mCentre = vec3_origin;

    // ---------------------------------------------------------------------
    //  Helper to reset rvParticleParms blocks
    // ---------------------------------------------------------------------
    auto InitParms = [](rvParticleParms& p, int spawnType)
        {
            p.mSpawnType = spawnType;
            p.mFlags = 0;
            p.mRange = 0.0f;
            p.mMisc = 0;
            p.mMins = vec3_origin;
            p.mMaxs = vec3_origin;
        };

    // ---------------------------------------------------------------------
    //  Spawn domains
    // ---------------------------------------------------------------------
    InitParms(mSpawnPosition, 3);
    InitParms(mSpawnDirection, 3);
    InitParms(mSpawnVelocity, 3);
    InitParms(mSpawnAcceleration, 3);
    InitParms(mSpawnFriction, 3);
    InitParms(mSpawnTint, 7);
    InitParms(mSpawnFade, 5);
    InitParms(mSpawnSize, 7);
    InitParms(mSpawnRotate, 3);
    InitParms(mSpawnAngle, 3);
    InitParms(mSpawnOffset, 3);
    InitParms(mSpawnLength, 3);

    // ---------------------------------------------------------------------
    //  Per-frame envelopes
    // ---------------------------------------------------------------------
    mTintEnvelope.Init();
    mFadeEnvelope.Init();
    mSizeEnvelope.Init();
    mRotateEnvelope.Init();
    mAngleEnvelope.Init();
    mOffsetEnvelope.Init();
    mLengthEnvelope.Init();

    // ---------------------------------------------------------------------
    //  Death domains
    // ---------------------------------------------------------------------
    InitParms(mDeathTint, 3);
    InitParms(mDeathFade, 1);
    InitParms(mDeathSize, 7);
    InitParms(mDeathRotate, 3);
    InitParms(mDeathAngle, 3);
    InitParms(mDeathOffset, 3);
    InitParms(mDeathLength, 3);

    // ---------------------------------------------------------------------
    //  Impact / timeout effects
    // ---------------------------------------------------------------------
    mNumImpactEffects = 0;
    mNumTimeoutEffects = 0;
    memset(mImpactEffects, 0, sizeof(mImpactEffects));
    memset(mTimeoutEffects, 0, sizeof(mTimeoutEffects));
}

/*
===============================================================================

    Misc query helpers

===============================================================================
*/
idTraceModel* rvParticleTemplate::GetTraceModel(void) const {
    return (mTraceModelIndex >= 0)
        ? bseLocal.traceModels[mTraceModelIndex]
        : nullptr;
}

// --------------------------------------------------------------------------

int rvParticleTemplate::GetTrailCount(void) const {
    const int count = static_cast<int>(rvRandom::flrand(
        mTrailCount.x,
        mTrailCount.y));
    return count < 0 ? 0 : count;
}

static inline void UpdateExtents(const idVec3& p, idVec3& minE, idVec3& maxE) {
    minE.x = min(minE.x, p.x);
    minE.y = min(minE.y, p.y);
    minE.z = min(minE.z, p.z);
    maxE.x = max(maxE.x, p.x);
    maxE.y = max(maxE.y, p.y);
    maxE.z = max(maxE.z, p.z);
}

void rvParticleTemplate::EvaluateSimplePosition(
    idVec3* pos,
    float time,
    float lifeTime,
    const idVec3* initPos,
    const idVec3* velocity,
    const idVec3* acceleration,
    const idVec3* friction
) {
    // 1) Linear motion + constant acceleration term: x = x₀ + v·t + ½·a·t²
    float t = time;
    float t2 = t * t;
    float halfT2 = 0.5f * t2;

    pos->x = initPos->x + velocity->x * t + acceleration->x * halfT2;
    pos->y = initPos->y + velocity->y * t + acceleration->y * halfT2;
    pos->z = initPos->z + velocity->z * t + acceleration->z * halfT2;

    // 2) Friction/damping integration term:
    //    Derived from original: v12 = halfT2 * ((2^v9 - 1) * halfT2) / 3
    //    where 2^v9 == exp((lifeTime - halfT2)/lifeTime)
    float expFactor = std::exp((lifeTime - halfT2) / lifeTime) - 1.0f;
    float frictionScalar = (halfT2 * halfT2 * expFactor) / 3.0f;

    pos->x += friction->x * frictionScalar;
    pos->y += friction->y * frictionScalar;
    pos->z += friction->z * frictionScalar;
}

float rvParticleTemplate::GetFurthestDistance() const {
    // 1) Gather min/max for each spawn parameter
    idVec3 minPos, maxPos;
    mSpawnPosition.GetMinsMaxs(minPos, maxPos);

    idVec3 minVel, maxVel;
    mSpawnVelocity.GetMinsMaxs(minVel, maxVel);

    idVec3 minAccel, maxAccel;
    mSpawnAcceleration.GetMinsMaxs(minAccel, maxAccel);

    idVec3 minFric, maxFric;
    mSpawnFriction.GetMinsMaxs(minFric, maxFric);

    // 2) Choose appropriate gravity
    float grav = game->IsMultiplayer()
        ? cvarSystem->GetCVarFloat("g_mp_gravity")
        : cvarSystem->GetCVarFloat("g_gravity");

    // 3) Build gravity offset vector and scale by template gravity.x
    idVec3 gravVec(0.0f, 0.0f, -grav);
    float gx = mGravity.x;
    gravVec.x *= gx;
    gravVec.y *= gx;
    gravVec.z *= gx;

    // 4) Apply gravity offset to acceleration bounds
    minAccel.x -= gravVec.x;  minAccel.y -= gravVec.y;  minAccel.z -= gravVec.z;
    maxAccel.x -= gravVec.x;  maxAccel.y -= gravVec.y;  maxAccel.z -= gravVec.z;

    // 5) Prepare overall min/max trackers
    idVec3 overallMin(FLT_MAX, FLT_MAX, FLT_MAX);
    idVec3 overallMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    // 6) Sample 8 time steps and all 16 combinations of parameter extremes
    const float duration = mDuration.y;
    const float step = duration * 0.125f;  // eighths
    idVec3 pos;

    for (int i = 0; i < 8; ++i) {
        float t = i * step;
        for (int p = 0; p < 2; ++p) {
            const idVec3& initP = (p ? maxPos : minPos);
            for (int v = 0; v < 2; ++v) {
                const idVec3& vel = (v ? maxVel : minVel);
                for (int a = 0; a < 2; ++a) {
                    const idVec3& acc = (a ? maxAccel : minAccel);
                    for (int f = 0; f < 2; ++f) {
                        const idVec3& fr = (f ? maxFric : minFric);

                        // Evaluate position for this combination
                        this->EvaluateSimplePosition(&pos, t, duration,
                            &initP, &vel, &acc, &fr);
                        UpdateExtents(pos, overallMin, overallMax);
                    }
                }
            }
        }
    }

    // 7) Compute half‐extents distances
    double distMin = overallMin.Length() * 0.5;
    double distMax = overallMax.Length() * 0.5;

    return (distMax > distMin) ? distMax : distMin;
}

/*
===============================================================================

    Static lexer helpers

===============================================================================
*/
bool rvParticleTemplate::GetVector(idLexer* src, int components,
    idVec3& out) {
    assert(components >= 1 && components <= 3);

    out.x = src->ParseFloat();

    if (components > 1) {
        if (!src->ExpectTokenString(","))
            return false;
        out.y = src->ParseFloat();

        if (components > 2) {
            if (!src->ExpectTokenString(","))
                return false;
            out.z = src->ParseFloat();
        }
    }
    return true;
}

/*
===============================================================================

    Motion-envelope parser   ( {   envelope <table>
                                   rate     <vec>
                                   count    <vec>
                                   offset   <vec>
                                 } )
===============================================================================
*/
bool rvParticleTemplate::ParseMotionParms(idLexer* src,
    int           vecCount,
    rvEnvParms& env)
{
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    while (src->ReadToken(&tok)) {
        if (!idStr::Cmp(tok, "}"))
            break;

        if (!idStr::Icmp(tok, "envelope")) {

            src->ReadToken(&tok);
            env.mTable = static_cast<const idDeclTable*>(
                declManager->FindType(DECL_TABLE,
                    tok,
                    false));

        }
        else if (!idStr::Icmp(tok, "rate")) {

            if (!GetVector(src, vecCount, env.mRate))
                return false;
            env.mIsCount = false;

        }
        else if (!idStr::Icmp(tok, "count")) {

            if (!GetVector(src, vecCount, env.mRate))
                return false;
            env.mIsCount = true;

        }
        else if (!idStr::Icmp(tok, "offset")) {

            if (!GetVector(src, vecCount, env.mEnvOffset))
                return false;

        }
        else {
            common->Warning("^4BSE:^1 Invalid motion parameter '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), src->GetFileName(), src->GetLineNum());
            src->SkipBracedSection(true);
        }
    }

    return true;
}

// ==========================================================================
//  Motion-domain block
// ==========================================================================
bool rvParticleTemplate::ParseMotionDomains(rvDeclEffect* effect,
    idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        //--------------------------------------------------------------
        //  Dispatch to the right envelope; vector-count depends on type
        //--------------------------------------------------------------
        if (!idStr::Icmp(tok, "tint")) ParseMotionParms(src, 3, mTintEnvelope);
        else if (!idStr::Icmp(tok, "fade")) ParseMotionParms(src, 1, mFadeEnvelope);
        else if (!idStr::Icmp(tok, "size")) ParseMotionParms(src, mNumSizeParms,
            mSizeEnvelope);
        else if (!idStr::Icmp(tok, "rotate")) ParseMotionParms(src, mNumRotateParms,
            mRotateEnvelope);
        else if (!idStr::Icmp(tok, "angle")) ParseMotionParms(src, 3, mAngleEnvelope);
        else if (!idStr::Icmp(tok, "offset")) ParseMotionParms(src, 3, mOffsetEnvelope);
        else if (!idStr::Icmp(tok, "length")) ParseMotionParms(src, 3, mLengthEnvelope);
        else {
            // Unknown keyword – skip nested section so parsing can continue
            common->Warning("^4BSE:^1 Invalid motion domain '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(),
                effect->GetName(),
                src->GetFileName(),
                src->GetLineNum());
            src->SkipBracedSection(true);
        }
    }

    return true;
}

/*
========================
rvParticleTemplate::GetMaxParmValue
========================
*/
float rvParticleTemplate::GetMaxParmValue(rvParticleParms* spawn, rvParticleParms* death, rvEnvParms* envelope) {
    float     minScale, maxScale;   // <- what Hex-Rays called “min” and (wrongly) “spawn”
    idBounds  sBounds, dBounds;

    // Raw bounds for the particle at spawn
    spawn->GetMinsMaxs(sBounds.b[0], sBounds.b[1]);

    // Envelope supplies two scalar multipliers (usually 0–1)
    if (envelope->GetMinMax(minScale, maxScale))
    {
        // Scale the spawn bounds
        sBounds.b[0] *= minScale;
        sBounds.b[1] *= maxScale;

        // Bounds at death, scaled in the same way
        death->GetMinsMaxs(dBounds.b[0], dBounds.b[1]);
        dBounds.b[0] *= minScale;
        dBounds.b[1] *= maxScale;

        // Expand sBounds so it encloses dBounds
        for (int axis = 0; axis < 3; ++axis)
        {
            if (dBounds.b[0][axis] < sBounds.b[0][axis]) sBounds.b[0][axis] = dBounds.b[0][axis];
            if (dBounds.b[1][axis] > sBounds.b[1][axis]) sBounds.b[1][axis] = dBounds.b[1][axis];
        }
    }

    // Length of the two opposite corners from the origin
    float cornerMin = sBounds.b[0].Length();
    float cornerMax = sBounds.b[1].Length();

    return idMath::Max(cornerMin, cornerMax);
}

// ==========================================================================
//  Flag parsing shared by *every* spawn shape
// ==========================================================================
bool rvParticleTemplate::CheckCommonParms(idLexer* src,
    rvParticleParms& p)
{
    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        if (!idStr::Icmp(tok, "surface")) p.mFlags |= 0x01;
        else if (!idStr::Icmp(tok, "useEndOrigin")) p.mFlags |= 0x02;
        else if (!idStr::Icmp(tok, "cone")) p.mFlags |= 0x04;
        else if (!idStr::Icmp(tok, "relative")) p.mFlags |= 0x08;
        else if (!idStr::Icmp(tok, "linearSpacing")) p.mFlags |= 0x10;
        else if (!idStr::Icmp(tok, "attenuate")) p.mFlags |= 0x20;
        else if (!idStr::Icmp(tok, "inverseAttenuate")) p.mFlags |= 0x40;
        else {
            common->Warning("^4BSE:^1 Unknown spawn flag '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), src->GetFileName(), src->GetLineNum());
        }
    }
    return true;        // nothing fatal here – bad keywords are soft-warnings
}


// ==========================================================================
//  Spawn-parameter parser  (point, line, box … model)
// ==========================================================================
bool rvParticleTemplate::ParseSpawnParms(rvDeclEffect* effect,
    idLexer* src,
    rvParticleParms& p,
    int            vecCount)
{
    //----------------------------------------------------------
    //  Helper for shape keyword errors (minimises boilerplate)
    //----------------------------------------------------------
    auto WarnBad = [&](const char* what)
        {
            common->Warning("^4BSE:^1 Invalid %s parameter in '%s' "
                "(file: %s, line: %d)",
                what, effect->GetName(),
                src->GetFileName(), src->GetLineNum());
        };

    //----------------------------------------------------------
    //  Opening brace + first keyword
    //----------------------------------------------------------
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    if (!src->ReadToken(&tok))
        return false;

    //======================================================================
    //          1.  POINT  (axis-aligned point or offset)
    //======================================================================
    if (!idStr::Icmp(tok, "point"))
    {
        p.mSpawnType = vecCount + 8;                           // 1-D/2-D/3-D
        GetVector(src, vecCount, p.mMins);                   // mins == point

        if (!CheckCommonParms(src, p))
            WarnBad("point");
    }

    //======================================================================
    //          2.  LINE  (pairs of points)
    //======================================================================
    else if (!idStr::Icmp(tok, "line"))
    {
        p.mSpawnType = vecCount + 12;
        GetVector(src, vecCount, p.mMins);
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);

        if (!CheckCommonParms(src, p))
            WarnBad("line");
    }

    //======================================================================
    //          3.  BOX  (AABB)
    //======================================================================
    else if (!idStr::Icmp(tok, "box"))
    {
        p.mSpawnType = vecCount + 16;
        GetVector(src, vecCount, p.mMins);
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);

        if (!CheckCommonParms(src, p))
            WarnBad("box");

        // If “surface” flag set – upgrade to “hollow box”
        if (p.mFlags & 0x01) {
            p.mSpawnType = vecCount + 20;
            FixupParms(p);
        }
    }

    //======================================================================
    //          4.  SPHERE
    //======================================================================
    else if (!idStr::Icmp(tok, "sphere"))
    {
        p.mSpawnType = vecCount + 24;
        GetVector(src, vecCount, p.mMins);                   // centre
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);                   // radii per-axis

        if (!CheckCommonParms(src, p))
            WarnBad("sphere");

        if (p.mFlags & 0x01) {          // surface == “shell”
            p.mSpawnType = vecCount + 28;
            FixupParms(p);
        }
    }

    //======================================================================
    //          5.  CYLINDER
    //======================================================================
    else if (!idStr::Icmp(tok, "cylinder"))
    {
        p.mSpawnType = vecCount + 32;
        GetVector(src, vecCount, p.mMins);                   // base
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);                   // top

        if (!CheckCommonParms(src, p))
            WarnBad("cylinder");

        if (p.mFlags & 0x01) {          // surface only
            p.mSpawnType = vecCount + 36;
            FixupParms(p);
        }
    }

    //======================================================================
    //          6.  SPIRAL  (mins / maxs / pitch)
    //======================================================================
    else if (!idStr::Icmp(tok, "spiral"))
    {
        p.mSpawnType = vecCount + 40;
        GetVector(src, vecCount, p.mMins);
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);
        src->ExpectTokenString(",");
        p.mRange = src->ParseFloat();                          // pitch Δ

        if (!CheckCommonParms(src, p))
            WarnBad("spiral");

        FixupParms(p);   // surfaces always collapse to SCALE variants
    }

    //======================================================================
    //          7.  MODEL  (arbitrary mesh sampling)
    //======================================================================
    else if (!idStr::Icmp(tok, "model"))
    {
        p.mSpawnType = vecCount + 44;

        // model name
        src->ReadToken(&tok);
        idRenderModel* mdl = renderModelManager->FindModel(tok);

        if (mdl->NumSurfaces() == 0) {
            common->Warning("^4BSE:^1 No surfaces in model '%s' "
                "– falling back to _default",
                tok.c_str());
            mdl = renderModelManager->FindModel("_default");
        }
        p.mMisc = mdl;

        // mins / maxs
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMins);
        src->ExpectTokenString(",");
        GetVector(src, vecCount, p.mMaxs);

        if (!CheckCommonParms(src, p))
            WarnBad("model");
    }

    //======================================================================
    //          8.  Unknown keyword
    //======================================================================
    else {
        common->Warning("^4BSE:^1 Unknown spawn keyword '%s' in '%s' "
            "(file: %s, line: %d)",
            tok.c_str(), effect->GetName(),
            src->GetFileName(), src->GetLineNum());
        src->SkipBracedSection(true);
        return false;
    }

    //----------------------------------------------------------------------
    //  Canonicalise for fast runtime use
    //----------------------------------------------------------------------
    FixupParms(p);
    return true;
}

// ==========================================================================
//  1.  SPAWN-DOMAIN BLOCK
// ==========================================================================
bool rvParticleTemplate::ParseSpawnDomains(rvDeclEffect* effect,
    idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        //----------------------------------------------------------
        //  Dispatch – every keyword maps to one rvParticleParms slot
        //----------------------------------------------------------
        if (!idStr::Icmp(tok, "position")) ParseSpawnParms(effect, src, mSpawnPosition, 3);
        else if (!idStr::Icmp(tok, "direction")) {
            ParseSpawnParms(effect, src, mSpawnDirection, 3);
            mFlags |= 0x00000040;      /* marks “hasDir” */
        }
        else if (!idStr::Icmp(tok, "velocity")) ParseSpawnParms(effect, src, mSpawnVelocity, 3);
        else if (!idStr::Icmp(tok, "acceleration")) ParseSpawnParms(effect, src, mSpawnAcceleration, 3);
        else if (!idStr::Icmp(tok, "friction")) ParseSpawnParms(effect, src, mSpawnFriction, 3);
        else if (!idStr::Icmp(tok, "tint")) ParseSpawnParms(effect, src, mSpawnTint, 3);
        else if (!idStr::Icmp(tok, "fade")) ParseSpawnParms(effect, src, mSpawnFade, 1);
        else if (!idStr::Icmp(tok, "size")) ParseSpawnParms(effect, src, mSpawnSize, mNumSizeParms);
        else if (!idStr::Icmp(tok, "rotate")) ParseSpawnParms(effect, src, mSpawnRotate, mNumRotateParms);
        else if (!idStr::Icmp(tok, "angle")) ParseSpawnParms(effect, src, mSpawnAngle, 3);
        else if (!idStr::Icmp(tok, "offset")) ParseSpawnParms(effect, src, mSpawnOffset, 3);
        else if (!idStr::Icmp(tok, "length")) ParseSpawnParms(effect, src, mSpawnLength, 3);
        else {
            common->Warning("^4BSE:^1 Invalid spawn keyword '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
            src->SkipBracedSection(true);
        }
    }
    return true;
}


// ==========================================================================
//  2.  DEATH-DOMAIN BLOCK   (plus automatic envelope fallbacks)
// ==========================================================================
bool rvParticleTemplate::ParseDeathDomains(rvDeclEffect* effect,
    idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        auto SetDefaultEnv = [](rvEnvParms& env) { env.SetDefaultType(); };

        if (!idStr::Icmp(tok, "tint")) { ParseSpawnParms(effect, src, mDeathTint, 3); SetDefaultEnv(mTintEnvelope); }
        else if (!idStr::Icmp(tok, "fade")) { ParseSpawnParms(effect, src, mDeathFade, 1); SetDefaultEnv(mFadeEnvelope); }
        else if (!idStr::Icmp(tok, "size")) { ParseSpawnParms(effect, src, mDeathSize, mNumSizeParms); SetDefaultEnv(mSizeEnvelope); }
        else if (!idStr::Icmp(tok, "rotate")) { ParseSpawnParms(effect, src, mDeathRotate, mNumRotateParms); SetDefaultEnv(mRotateEnvelope); }
        else if (!idStr::Icmp(tok, "angle")) { ParseSpawnParms(effect, src, mDeathAngle, 3); SetDefaultEnv(mAngleEnvelope); }
        else if (!idStr::Icmp(tok, "offset")) { ParseSpawnParms(effect, src, mDeathOffset, 3); SetDefaultEnv(mOffsetEnvelope); }
        else if (!idStr::Icmp(tok, "length")) { ParseSpawnParms(effect, src, mDeathLength, 3); SetDefaultEnv(mLengthEnvelope); }
        else {
            common->Warning("^4BSE:^1 Invalid end keyword '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
            src->SkipBracedSection(true);
        }
    }
    return true;
}


// ==========================================================================
//  3.  IMPACT BLOCK     (bounce / remove / effect)
// ==========================================================================
bool rvParticleTemplate::ParseImpact(rvDeclEffect* effect,
    idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    mFlags |= 0x00000002;     // “hasImpact” bit

    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        if (!idStr::Icmp(tok, "effect"))            // ----- effect list
        {
            src->ReadToken(&tok);                     // grab effect name
            if (mNumImpactEffects >= 4) {
                common->Warning("^4BSE:^1 Too many impact effects '%s' in '%s' "
                    "(file: %s, line: %d)",
                    tok.c_str(), effect->GetName(),
                    src->GetFileName(), src->GetLineNum());
            }
            else {
                mImpactEffects[mNumImpactEffects++] =
                    declManager->FindEffect(tok, false);
            }
        }
        else if (!idStr::Icmp(tok, "remove"))       // ----- remove flag
        {
            const bool remove = src->ParseInt() != 0;
            if (remove) mFlags |= 0x00000004; else mFlags &= ~0x00000004;
        }
        else if (!idStr::Icmp(tok, "bounce"))       // ----- elasticity
        {
            mBounce = src->ParseFloat();
        }
        else                                            // ----- unknown key
        {
            common->Warning("^4BSE:^1 Invalid impact keyword '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
            src->SkipBracedSection(true);
        }
    }
    return true;
}


// ==========================================================================
//  4.  TIMEOUT BLOCK    (effect list only)
// ==========================================================================
bool rvParticleTemplate::ParseTimeout(rvDeclEffect* effect,
    idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    while (src->ReadToken(&tok)) {

        if (!idStr::Cmp(tok, "}"))
            break;

        if (idStr::Icmp(tok, "effect")) {
            common->Warning("^4BSE:^1 Invalid timeout keyword '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
            src->SkipBracedSection(true);
            continue;
        }

        src->ReadToken(&tok);                   // effect name
        if (mNumTimeoutEffects >= 4) {
            common->Warning("^4BSE:^1 Too many timeout effects '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
        }
        else {
            mTimeoutEffects[mNumTimeoutEffects++] =
                declManager->FindEffect(tok, false);
        }
    }
    return true;
}


// ==========================================================================
//  5.  BLEND-MODE PARSER  (currently only “add” is recognised)
// ==========================================================================
bool rvParticleTemplate::ParseBlendParms(rvDeclEffect* effect,
    idLexer* src)
{
    idToken tok;
    if (!src->ReadToken(&tok))
        return false;

    if (!idStr::Icmp(tok, "add")) {
        mFlags |= 0x00000080;       // additive flag
    }
    else {
        common->Warning("^4BSE:^1 Invalid blend type '%s' in '%s' "
            "(file: %s, line: %d)",
            tok.c_str(), effect->GetName(),
            src->GetFileName(), src->GetLineNum());
    }
    return true;
}

/* ========================================================================
   1.  Deep-equality check
   ======================================================================== */
bool rvParticleTemplate::Compare(const rvParticleTemplate& rhs) const {

    /* ------------------------------------------------------------
       Cheap tests first – bitfields, type, durations, material etc.
       ------------------------------------------------------------ */
    if (mFlags != rhs.mFlags ||
        mType != rhs.mType ||
        mDuration != rhs.mDuration ||
        idStr::Cmp(mMaterialName, rhs.mMaterialName) ||
        idStr::Cmp(mModelName, rhs.mModelName) ||
        mGravity != rhs.mGravity ||
        !idMath::Fabs(mBounce - rhs.mBounce) < idMath::FLT_EPSILON ||
        mNumSizeParms != rhs.mNumSizeParms ||
        mNumRotateParms != rhs.mNumRotateParms ||
        mVertexCount != rhs.mVertexCount ||
        mIndexCount != rhs.mIndexCount)
        return false;

    /* ------------------------------------------------------------
       Spawn domains (vector compare helpers already exist)
       ------------------------------------------------------------ */
    auto TintEqual = [](const rvParticleParms& a,
        const rvParticleParms& b)
        {
            auto C = [](float f) { return static_cast<int>(f * 255.0f); };
            return C(a.mMins.x) == C(b.mMins.x) && C(a.mMins.y) == C(b.mMins.y) &&
                C(a.mMins.z) == C(b.mMins.z) && C(a.mMaxs.x) == C(b.mMaxs.x) &&
                C(a.mMaxs.y) == C(b.mMaxs.y) && C(a.mMaxs.z) == C(b.mMaxs.z) &&
                a.mSpawnType == b.mSpawnType && a.mFlags == b.mFlags;
        };

    if (mSpawnPosition != rhs.mSpawnPosition ||
        mSpawnVelocity != rhs.mSpawnVelocity ||
        mSpawnAcceleration != rhs.mSpawnAcceleration ||
        mSpawnFriction != rhs.mSpawnFriction ||
        mSpawnDirection != rhs.mSpawnDirection ||
        !TintEqual(mSpawnTint, rhs.mSpawnTint) ||
        mSpawnFade != rhs.mSpawnFade ||
        mSpawnSize != rhs.mSpawnSize ||
        mSpawnRotate != rhs.mSpawnRotate ||
        mSpawnAngle != rhs.mSpawnAngle ||
        mSpawnOffset != rhs.mSpawnOffset ||
        mSpawnLength != rhs.mSpawnLength)
    {
        return false;
    }

    /* ------------------------------------------------------------
       Death domains are compared only when the envelope is active
       ------------------------------------------------------------ */
    auto DeathNeeds = [](const rvEnvParms& e) { return e.GetType() > 0; };

    if (DeathNeeds(mTintEnvelope) && !TintEqual(mDeathTint, rhs.mDeathTint))   return false;
    if (DeathNeeds(mFadeEnvelope) && (mDeathFade != rhs.mDeathFade))         return false;
    if (DeathNeeds(mSizeEnvelope) && (mDeathSize != rhs.mDeathSize))         return false;
    if (DeathNeeds(mRotateEnvelope) && (mDeathRotate != rhs.mDeathRotate))       return false;
    if (DeathNeeds(mAngleEnvelope) && (mDeathAngle != rhs.mDeathAngle))        return false;
    if (DeathNeeds(mOffsetEnvelope) && (mDeathOffset != rhs.mDeathOffset))       return false;
    if (DeathNeeds(mLengthEnvelope) && (mDeathLength != rhs.mDeathLength))       return false;


    /* ------------------------------------------------------------
       Impact / timeout effect arrays
       ------------------------------------------------------------ */
    if (mNumImpactEffects != rhs.mNumImpactEffects ||
        mNumTimeoutEffects != rhs.mNumTimeoutEffects)
        return false;

    for (int i = 0; i < mNumImpactEffects; ++i)
        if (mImpactEffects[i] != rhs.mImpactEffects[i])
            return false;

    for (int i = 0; i < mNumTimeoutEffects; ++i)
        if (mTimeoutEffects[i] != rhs.mTimeoutEffects[i])
            return false;

    /* ------------------------------------------------------------
       Envelope bodies
       ------------------------------------------------------------ */
    if (mRotateEnvelope != rhs.mRotateEnvelope ||
        mSizeEnvelope != rhs.mSizeEnvelope ||
        mFadeEnvelope != rhs.mFadeEnvelope ||
        mTintEnvelope != rhs.mTintEnvelope ||
        mAngleEnvelope != rhs.mAngleEnvelope ||
        mOffsetEnvelope != rhs.mOffsetEnvelope ||
        mLengthEnvelope != rhs.mLengthEnvelope)
    {
        return false;
    }

    /* ------------------------------------------------------------
       Trail data
       ------------------------------------------------------------ */
    if (mTrailType != rhs.mTrailType)
        return false;

    switch (mTrailType)
    {
    case 0: break;
    case 1: // burn
    case 2: // motion
        if (mTrailTime != rhs.mTrailTime || mTrailCount != rhs.mTrailCount)
            return false;
        break;
    case 3: // custom
        if (idStr::Cmp(mTrailTypeName, rhs.mTrailTypeName))
            return false;
        break;
    }
    return true;
}



/* ========================================================================
   2.  Finish() – post-parse sanity passes & derived-data setup
   ======================================================================== */
void rvParticleTemplate::Finish(void)
{
    mFlags |= 0x0001;                     // “parsed” bit

    /* --------------------------------------------------------------------
       2-A  trail sanitiser
       -------------------------------------------------------------------- */
    if (mTrailType == 0 || mTrailType == 3) {
        mTrailTime.Zero();
        mTrailCount.Zero();
    }

    /* --------------------------------------------------------------------
       2-B  vertex / index budgets by particle type
       -------------------------------------------------------------------- */
    switch (mType)
    {
        /* billboards / beams / ribbons / decals ................................ */
    case 1: case 2: case 4: case 6: case 8:
        mVertexCount = 4;
        mIndexCount = 6;
        if (mTrailType == 1 && mTrailCount.y > 0.0f) {
            const int maxTrail = GetMaxTrailCount();
            mVertexCount *= maxTrail;
            mIndexCount *= maxTrail;
        }
        break;

        /* ribbon-extruded mesh .................................................. */
    case 5: {
        // grab first surface of model for vert/idx counts and material
        idRenderModel* mdl = renderModelManager->FindModel(mModelName);
        if (mdl->NumSurfaces())
        {
            const modelSurface_t * s = mdl->Surface(0);
            const srfTriangles_t* tri = s->geometry;
            mVertexCount = tri ? tri->numVerts : s->geometry->numVerts;
            mIndexCount = tri ? tri->numIndexes : s->geometry->numIndexes;
            mMaterial = s->shader;
            mMaterialName = mMaterial->GetName();
        }

        /* generate AABB trace-model for ribbon collisions */
        idBounds box;
        mdl->GetBounds(box);
        auto* tm = new idTraceModel;
        tm->InitBox();
        tm->SetupBox(box);
        mTraceModelIndex = bseLocal.traceModels.Append(tm);
        break;
    }

          /* forked sprite ........................................................ */
    case 7:
        mVertexCount = 4 * (5 * mNumForks + 5);
        mIndexCount = 60 * (mNumForks + 1);
        break;

        /* glare helper .......................................................... */
    case 9:
        mVertexCount = mIndexCount = 0;
        break;

    default:
        mVertexCount = 0; mIndexCount = 0;          // should not occur
    }

    /* --------------------------------------------------------------------
       2-C  clamp & sort min/max ranges
       -------------------------------------------------------------------- */
    float y;
    if (this->mDuration.x >= (double)this->mDuration.y)
    {
        y = this->mDuration.y;
        this->mDuration.y = this->mDuration.x;
        this->mDuration.x = y;
    }
    if (this->mGravity.x >= (double)this->mGravity.y)
    {
        y = this->mGravity.y;
        this->mGravity.y = this->mGravity.x;
        this->mGravity.x = y;
    }
    if (this->mTrailTime.x >= (double)this->mTrailTime.y)
    {
        y = this->mTrailTime.y;
        this->mTrailTime.y = this->mTrailTime.x;
        this->mTrailTime.x = y;
    }
    if (this->mTrailCount.x >= (double)this->mTrailCount.y)
    {
        y = this->mTrailCount.y;
        this->mTrailCount.y = this->mTrailCount.x;
        this->mTrailCount.x = y;
    }

    /* --------------------------------------------------------------------
       2-D  pre-compute the spawn-centre for helpers that need it
       -------------------------------------------------------------------- */
    if (!(mFlags & 0x1000)) {
        switch (mSpawnPosition.mSpawnType)
        {
        case 0x0B:                               // explicit point
            mCentre = mSpawnPosition.mMins;
            break;

        case 0x0F: case 0x13: case 0x17:         // boxes
        case 0x1B: case 0x1F: case 0x23:
        case 0x27: case 0x2B: case 0x2F:
            mCentre = (mSpawnPosition.mMaxs + mSpawnPosition.mMins) * 0.5f;
            break;

        default: break;                          // others treat centre ≡ 0
        }
    }
}



/* ========================================================================
   3.  Parse() – master lexer loop
   ======================================================================== */
bool rvParticleTemplate::Parse(rvDeclEffect* effect, idLexer* src)
{
    if (!src->ExpectTokenString("{"))
        return false;

    idToken tok;
    while (src->ReadToken(&tok))
    {
        if (!idStr::Cmp(tok, "}"))
            break;

        /* ----------------------------------------------------------------
           Hand each keyword to its mini-parser or immediate setter
           ---------------------------------------------------------------- */
        if (!idStr::Icmp(tok, "start"))      ParseSpawnDomains(effect, src);
        else if (!idStr::Icmp(tok, "end"))      ParseDeathDomains(effect, src);
        else if (!idStr::Icmp(tok, "motion"))      ParseMotionDomains(effect, src);

        /*  generated* flags ------------------------------------------------*/
        else if (!idStr::Icmp(tok, "generatedNormal")) mFlags |= 0x0100;
        else if (!idStr::Icmp(tok, "generatedOriginNormal")) mFlags |= 0x0200;
        else if (!idStr::Icmp(tok, "flipNormal")) mFlags |= 0x0400;
        else if (!idStr::Icmp(tok, "generatedLine")) mFlags |= 0x0800;

        /*  persist / tiling / duration / gravity -------------------------- */
        else if (!idStr::Icmp(tok, "persist"))  mFlags |= 0x2000;

        else if (!idStr::Icmp(tok, "tiling")) {
            mFlags |= 0x1000;
            mTiling = idMath::ClampFloat(0.002f, 1024.f, src->ParseFloat());
        }
        else if (!idStr::Icmp(tok, "duration")) {
            mDuration.x = idMath::ClampFloat(0.002f, 60.f, src->ParseFloat());
            src->ExpectTokenString(",");
            mDuration.y = idMath::ClampFloat(0.002f, 60.f, src->ParseFloat());
        }
        else if (!idStr::Icmp(tok, "gravity")) {
            mGravity.x = src->ParseFloat();  src->ExpectTokenString(",");
            mGravity.y = src->ParseFloat();
        }

        /*  trail section --------------------------------------------------- */
        else if (!idStr::Icmp(tok, "trailType"))
        {
            src->ReadToken(&tok);
            if (!idStr::Icmp(tok, "burn")) mTrailType = 1;
            else if (!idStr::Icmp(tok, "motion")) mTrailType = 2;
            else { mTrailType = 3;  mTrailTypeName = tok; }
        }
        else if (!idStr::Icmp(tok, "trailMaterial")) {
            src->ReadToken(&tok);
            mTrailMaterial = declManager->FindMaterial(tok, false);
            mTrailMaterialName = tok;
        }
        else if (!idStr::Icmp(tok, "trailTime")) {
            mTrailTime.x = src->ParseFloat();  src->ExpectTokenString(",");
            mTrailTime.y = src->ParseFloat();
        }
        else if (!idStr::Icmp(tok, "trailCount")) {
            mTrailCount.x = src->ParseFloat(); src->ExpectTokenString(",");
            mTrailCount.y = src->ParseFloat();
        }

        /*  material / entityDef / fork etc. ------------------------------- */
        else if (!idStr::Icmp(tok, "material")) {
            src->ReadToken(&tok);
            mMaterial = declManager->FindMaterial(tok, true);
            mMaterialName = tok;
        }
        else if (!idStr::Icmp(tok, "entityDef")) {
            src->ReadToken(&tok);
            mEntityDefName = tok;
            // warm-cache its dict if it exists
            if (const idDecl* d = declManager->FindType(DECL_ENTITYDEF,
                mEntityDefName,
                false))
                game->CacheDictionaryMedia(static_cast<const idDict&>(d[1]));
        }
        else if (!idStr::Icmp(tok, "fork")) {
            mNumForks = idMath::ClampInt(0, 16, src->ParseInt());
        }
        else if (!idStr::Icmp(tok, "forkMins")) GetVector(src, 3, mForkSizeMins);
        else if (!idStr::Icmp(tok, "forkMaxs")) GetVector(src, 3, mForkSizeMaxs);
        else if (!idStr::Icmp(tok, "jitterSize")) GetVector(src, 3, mJitterSize);
        else if (!idStr::Icmp(tok, "jitterRate")) mJitterRate = src->ParseFloat();
        else if (!idStr::Icmp(tok, "jitterTable")) {
            src->ReadToken(&tok);
            const idDeclTable* t = declManager->FindType<idDeclTable>(tok, true);
            if (!t->IsImplicit()) mJitterTable = t;
        }

        /*  blend / shadow / specular flags -------------------------------- */
        else if (!idStr::Icmp(tok, "blend")) ParseBlendParms(effect, src);
        else if (!idStr::Icmp(tok, "shadows")) mFlags |= 0x020000;
        else if (!idStr::Icmp(tok, "specular")) mFlags |= 0x040000;

        /*  model-specific (ribbon-extrude) -------------------------------- */
        else if (!idStr::Icmp(tok, "model")) {
            src->ReadToken(&tok);
            mModelName = tok;

            if (renderModelManager->FindModel(tok)->NumSurfaces() == 0) {
                mModelName = "_default";
                common->Warning("^4BSE:^1 Model '%s' has no surfaces – using _default",
                    tok.c_str());
            }
        }

        /*  impact / timeout blocks ---------------------------------------- */
        else if (!idStr::Icmp(tok, "impact")) ParseImpact(effect, src);
        else if (!idStr::Icmp(tok, "timeout")) ParseTimeout(effect, src);

        /*  unknown keyword ------------------------------------------------- */
        else {
            common->Warning("^4BSE:^1 Invalid particle keyword '%s' in '%s' "
                "(file: %s, line: %d)",
                tok.c_str(), effect->GetName(),
                src->GetFileName(), src->GetLineNum());
            src->SkipBracedSection(true);
        }
    }

    /* --------------------------------------------------------------------
       Final consistency pass
       -------------------------------------------------------------------- */
    Finish();
    return true;
}
