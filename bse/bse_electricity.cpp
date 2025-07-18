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

static ID_INLINE void  rvEP_BuildQuad(idDrawVert* v,
    const idVec3& centre,
    const idVec3& offset,
    float         vCoord,
    unsigned int  colour)
{
    // upper edge
    v[0].xyz = centre + offset;
    v[0].st.x = vCoord;
    v[0].st.y = 0.0f;
    *reinterpret_cast<unsigned*>(v[0].color) = colour;

    // lower edge
    v[1].xyz = centre - offset;
    v[1].st.x = vCoord;
    v[1].st.y = 1.0f;
    *reinterpret_cast<unsigned*>(v[1].color) = colour;
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::GetBoltCount                                        *|
\*─────────────────────────────────────────────────────────────────────────────*/
int rvElectricityParticle::GetBoltCount(float length)
{
    const int bolts = static_cast<int>(std::ceil(length * 0.0625f));
    return idMath::ClampInt(3, 200, bolts);
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::Update                                              *|
\*─────────────────────────────────────────────────────────────────────────────*/
int rvElectricityParticle::Update(float time)
{
    const float elapsed = time - mStartTime;

    idVec3 len{};
    EvaluateLength(elapsed, &len);

    mNumBolts = GetBoltCount(len.LengthFast());
    return mNumBolts;
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::RenderLineSegment                                   *|
\*─────────────────────────────────────────────────────────────────────────────*/
void rvElectricityParticle::RenderLineSegment(const rvBSE* effect,SElecWork* work,const idVec3& start, float         startFraction)
{
    idVec3 left = work->length.Cross(work->viewPos);
    float  len2 = left.LengthSqr();
    if (len2 > 1e-6f) {
        left *= idMath::InvSqrt(len2) * work->size;
    }

    unsigned colour = rvParticle::HandleTint(this, effect, &work->tint, work->alpha);

    idDrawVert* v = &work->tri->verts[work->tri->numVerts];
    rvEP_BuildQuad(v, start, left, startFraction * work->step + work->fraction, colour);

    work->tri->numVerts += 2;
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::ApplyShape                                          *|
\*─────────────────────────────────────────────────────────────────────────────*/
void rvElectricityParticle::ApplyShape(const rvBSE* effect,
    SElecWork* work,
    const idVec3& start,
    const idVec3& end,
    int           recurse,
    float         startFrac,
    float         endFrac)
{
    if (recurse <= 0) {
        RenderLineSegment(effect, work, start, startFrac);
        return;
    }

    /* -------- randomised split parameters (mimic original tables) -------- */
    const float randA = rvRandom::flrand(0.05f, 0.09f);
    const float randB = rvRandom::flrand(0.05f, 0.09f);
    const float shape = rvRandom::flrand(0.56f, 0.76f);

    const idVec3 dir = end - start;
    const float  length = dir.LengthFast() * 0.70f;
    const idVec3 forward = dir.Normalized();
    idVec3       left = forward.Cross(idVec3::zAxis);
    if (left.LengthSqr() < 1e-6f) {          // forward ≈ Z, choose arbitrary axis
        left = idVec3::xAxis;
    }
    left.Normalize();
    const idVec3 down = forward.Cross(left);

    /* -------- generate two perturbed points along the segment -------- */
    const float len1 = rvRandom::flrand(-randA - 0.02f, 0.02f - randA) * length;
    const float len2 = rvRandom::flrand(-randB - 0.02f, 0.02f - randB) * length;

    const idVec3 point1 = start * shape + end * (1.0f - shape) +
        left * len1 + down * rvRandom::flrand(0.23f, 0.43f) * length;

    const float t2 = rvRandom::flrand(0.23f, 0.43f);
    const idVec3 point2 = start * t2 + end * (1.0f - t2) +
        left * len2 + down * rvRandom::flrand(-0.02f, 0.02f) * length;

    const float mid0 = startFrac * 0.6666667f + endFrac * 0.3333333f;
    const float mid1 = startFrac * 0.3333333f + endFrac * 0.6666667f;

    ApplyShape(effect, work, start, point1, recurse - 1, startFrac, mid0);
    ApplyShape(effect, work, point1, point2, recurse - 1, mid0, mid1);
    ApplyShape(effect, work, point2, end, recurse - 1, mid1, endFrac);
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::RenderBranch                                        *|
\*─────────────────────────────────────────────────────────────────────────────*/
void rvElectricityParticle::RenderBranch(const rvBSE* effect,
    SElecWork* work,
    const idVec3& start,
    const idVec3& end)
{
    idVec3 forward = end - start;
    const float len = forward.NormalizeSafely();
    if (len < 1e-6f) return;

    // Build an orthonormal basis (left, up) around the branch direction.
    idVec3 left = (fabs(forward.x) < 0.99f) ?
        idVec3(0, 0, 1).Cross(forward).Normalized() :
        idVec3(0, 1, 0).Cross(forward).Normalized();
    idVec3 up = forward.Cross(left);

    /* Pre-allocate a temporary coordinate buffer on the stack.
       The caller (Render) guarantees enough space. */
    int   segVertStart = work->tri->numVerts;
    int   outCount = 0;
    float frac = 0.0f;
    idVec3 current = start;

    work->coords[outCount++] = current;

    while (frac < 1.0f - work->step * 0.5f) {
        frac += work->step;

        /* sample 1-D noise table to get smoothly varying offset weight */
        const float noise = mJitterTable ? mJitterTable->TableLookup(frac) : 0.0f;

        idVec3 jitter(
            rvRandom::flrand(-mJitterSize.x, mJitterSize.x),
            rvRandom::flrand(-mJitterSize.y, mJitterSize.y),
            rvRandom::flrand(-mJitterSize.z, mJitterSize.z));

        /* decompose jitter across the local basis */
        idVec3 offset = forward * jitter.x + left * jitter.y + up * jitter.z;

        current = start + forward * (len * frac) + offset * noise;
        work->coords[outCount++] = current;
    }

    work->coords[outCount++] = end;

    /* ------------------------------------------------------------------ */
    /* Build quads + indices                                              */
    /* ------------------------------------------------------------------ */
    float vCoord = 0.0f;
    for (int i = 0; i < outCount - 1; ++i) {
        RenderLineSegment(effect, work, work->coords[i], vCoord);
        vCoord += work->step;
    }

    // Turn the generated vertex strip into a solid ribbon.
    for (int base = segVertStart;
        base < work->tri->numVerts - 2;
        base += 2)
    {
        const int idx = work->tri->numIndexes;
        work->tri->indexes[idx + 0] = base;
        work->tri->indexes[idx + 1] = base + 1;
        work->tri->indexes[idx + 2] = base + 2;
        work->tri->indexes[idx + 3] = base;
        work->tri->indexes[idx + 4] = base + 2;
        work->tri->indexes[idx + 5] = base + 3;
        work->tri->numIndexes += 6;
    }
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::SetupElectricity                                    *|
\*─────────────────────────────────────────────────────────────────────────────*/
void rvElectricityParticle::SetupElectricity(rvParticleTemplate* pt)
{
    mNumForks = pt->mNumForks;
    mSeed = rvRandom::Init();
    mForkSizeMins = pt->mForkSizeMins;
    mForkSizeMaxs = pt->mForkSizeMaxs;
    mJitterSize = pt->mJitterSize;
    mLastJitter = 0.0f;
    mJitterRate = pt->mJitterRate;
    mJitterTable = pt->mJitterTable;
}

/*─────────────────────────────────────────────────────────────────────────────*\
|*  rvElectricityParticle::Render                                              *|
\*─────────────────────────────────────────────────────────────────────────────*/
bool rvElectricityParticle::Render(const rvBSE* effect,
    const idMat3& view,
    srfTriangles_s* tri,
    float           time,
    float           overrideAlpha)
{
    /* ───── lifetime / env-parm evaluation ───── */
    float evalTime;
    if (!rvParticle::GetEvaluationTime(this, time, &evalTime, false))
        return false;

    SElecWork work{};
    rvEnvParms3::Evaluate(&mTintEnv, evalTime, &work.tint.x);
    rvEnvParms1::Evaluate(&mFadeEnv, evalTime, &work.tint.w);

    (this->*mEvalSizePtr)(evalTime, &work.size);
    (this->*mEvalLengthPtr)(evalTime, &work.length);

    /* ───── transform to effect-space if requested ───── */
    if (!(mFlags & 2)) {
        work.length = effect->mCurrentAxis * (mInitAxis * work.length);
    }

    /* ───── velocity-driven length (optional) ───── */
    if (mFlags & 0x10000) {
        idVec3 vel;
        rvParticle::EvaluateVelocity(this, effect,
            &vel, time - mMotionStartTime);
        vel.NormalizeSafely();
        const float len = work.length.LengthFast();
        work.length = vel * len;
    }

    const float mainLength = work.length.LengthFast();
    if (mainLength < 0.1f)
        return false;

    /* ───── jitter reseeding ───── */
    if (mLastJitter + mJitterRate <= time) {
        mLastJitter = time;
        mSeed = rvRandom::Init();
    }
    unsigned int restoreSeed = rvRandom::mSeed;
    rvRandom::mSeed = mSeed;

    /* ───── per-frame scratch initialisation ───── */
    work.viewPos.x = view.mat[0].x;
    work.viewPos.y = view.mat[0].y;
    work.viewPos.z = view.mat[0].z;

    work.tri = tri;
    work.alpha = overrideAlpha;
    work.forward = work.length;
    work.step = mTextureScale / static_cast<float>(mNumBolts);

    /* Co-ordinates buffer for this branch (main + forks share the same stack) */
    idVec3 tmpCoords[256];
    work.coords = tmpCoords;

    /* ───── evaluate particle origin in world-space ───── */
    idVec3 position;
    rvParticle::EvaluatePosition(this, effect,
        &position, time - mMotionStartTime);

    const idVec3 endPos = position + work.length;

    /* ───── main bolt ───── */
    RenderBranch(effect, &work, position, endPos);

    /* ───── build list of fork start points ───── */
    idVec3 forkBases[16];
    for (int i = 0; i < mNumForks; ++i) {
        const int idx = rvRandom::irand(1, mNumBolts - 1);
        forkBases[i] = work.coords[idx];
    }

    /* ───── render forks ───── */
    for (int i = 0; i < mNumForks; ++i)
    {
        /* choose a point roughly halfway between fork-base and main end */
        const idVec3 mid = (forkBases[i] + endPos) * 0.5f;

        const idVec3 forkEnd = mid + idVec3(
            rvRandom::flrand(mForkSizeMins.x, mForkSizeMaxs.x),
            rvRandom::flrand(mForkSizeMins.y, mForkSizeMaxs.y),
            rvRandom::flrand(mForkSizeMins.z, mForkSizeMaxs.z));

        idVec3 dir = forkEnd - forkBases[i];
        const float forkLen = dir.LengthFast();
        if (forkLen < 1.0f || forkLen >= mainLength)
            continue;

        const int forkBolts = GetBoltCount(forkLen);

        work.forward = dir;
        work.step = 1.0f / static_cast<float>(forkBolts);
        RenderBranch(effect, &work, forkBases[i], forkEnd);
    }

    /* ───── restore global RNG state ───── */
    rvRandom::mSeed = restoreSeed;
    return true;
}