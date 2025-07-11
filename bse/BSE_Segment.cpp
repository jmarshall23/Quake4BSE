/*
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


/* ------------------------------------------------------------ helpers --- */
namespace {
    template <typename T>
    ID_INLINE T Clamp(T v, T lo, T hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }
}

/* ------------------------------------------------------------ statics --- */
float rvSegment::s_segmentBaseCost[11] = {
    /* 0-10 seg types */ 0, 0, 50, 50, 0, 20, 10, 10, 0, 30, 40
};

/* ------------------------------------------------------------- dtor ---- */
rvSegment::~rvSegment() {
    if (!mParticles)
        return;

    const rvSegmentTemplate* st = mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
    if (!st)
        return;

    for (int i = 0; i < mParticleCount; ++i) {
        rvParticle* p = &mParticles[i];
        p->~rvParticle();
    }
    Memory::Free(mParticles);
    mParticles = nullptr;
}

void rvSegment::Rewind(rvBSE* effect)
{
    double v2; // st7

    if (effect->mDuration == this->mSegEndTime - this->mSegStartTime)
    {
        v2 = this->mSegStartTime - (effect->mDuration + effect->mDuration);
        this->mSegStartTime = v2;
        this->mLastTime = v2;
    }
}

/* ------------------------------------------------------- ValidateSpawn -- */
void rvSegment::ValidateSpawnRates() {
    float& lo = mSecondsPerParticle.x;
    float& hi = mSecondsPerParticle.y;

    hi = Clamp(hi, kMinSpawnRate, kMaxSpawnRate);
    lo = Clamp(lo, hi, kMaxSpawnRate);   // lo may not exceed hi
}

/* ------------------------------------------------ GetSecondsPerParticle -- */
void rvSegment::GetSecondsPerParticle(rvBSE* effect,
    rvSegmentTemplate* st,
    rvParticleTemplate* pt)
{
    if (st->mDensity.y == 0.0f) {            // fixed-count segment?
        mCount = st->mCount;
    }
    else {
        /* density * volume, clamped ------------------------------------ */
        float volume = idMath::ClampFloat(kMinSpawnRate, 1000.0f, pt->GetSpawnVolume(effect));

        mCount.x = volume * st->mDensity.x;
        mCount.y = volume * st->mDensity.y;

        /* overall cap -------------------------------------------------- */
        if (st->mParticleCap != 0.0f) {
            mCount.x = Clamp(mCount.x, 1.0f, st->mParticleCap);
            mCount.y = Clamp(mCount.y, 1.0f, st->mParticleCap);
        }
    }

    /* convert count → seconds-per-particle for LOOPING / CONTINUOUS ---- */
    if (st->mSegType == 2 || st->mSegType == 4) {
        if (mCount.x != 0.0f) mSecondsPerParticle.x = 1.0f / mCount.x;
        if (mCount.y != 0.0f) mSecondsPerParticle.y = 1.0f / mCount.y;
        ValidateSpawnRates();
    }
}

/* ------------------------------------------------------------- InitTime -- */
void rvSegment::InitTime(rvBSE* effect,
    rvSegmentTemplate* st,
    float timeNow)
{
    mFlags &= ~1;   // clear “done” bit
    mSegStartTime = rvRandom::flrand(st->mLocalStartTime.x,
        st->mLocalStartTime.y)
        + timeNow;

    mSegEndTime = rvRandom::flrand(st->mLocalDuration.x,
        st->mLocalDuration.y)
        + mSegStartTime;

    /* if this segment dictates BSE duration --------------------------- */
    const bool segDefinesDuration =
        (st->mFlags & 0x10) == 0 ||
        ((effect->mFlags & 1) == 0 &&
            !st->GetSoundLooping());

    if (segDefinesDuration) {
        effect->SetDuration(mSegEndTime - timeNow);
    }
}

/* ------------------------------------------------------------ Init ---- */
void rvSegment::Init(rvBSE* effect,
    rvDeclEffect* decl,
    int templateHandle,
    float timeNow)
{
    mFlags = 0;
    mEffectDecl = decl;
    mSegmentTemplateHandle = templateHandle;

    const auto* st = decl->GetSegmentTemplate(templateHandle);
    if (!st) return;

    mLastTime = timeNow;
    mActiveCount = 0;
    mSecondsPerParticle = idVec2(0.0f, 0.0f);
    mCount = idVec2(1.0f, 1.0f);
    mSoundVolume = 0.0f;
    mFreqShift = 1.0f;
    mParticles = nullptr;

    InitTime(effect, const_cast<rvSegmentTemplate*>(st), effect->mStartTime);
    GetSecondsPerParticle(effect, const_cast<rvSegmentTemplate*>(st),
        const_cast<rvParticleTemplate*>(&st->mParticleTemplate));
    const_cast<rvSegmentTemplate*>(st)->mBSEEffect = effect;
}

/* --------------------------------------------------------- InsertParticle */
void rvSegment::InsertParticle(rvParticle* p,
    rvSegmentTemplate* st)
{
    if (st->mFlags & 0x100)                 // invisible?
        return;

    ++mActiveCount;

    /* SRTF_SORTBYENDTIME flag? ---------------------------------------- */
    if (st->mFlags & 0x200) {
        p->mNext = mUsedHead;
        mUsedHead = p;
        return;
    }

    /* otherwise keep list sorted by EndTime --------------------------- */
    rvParticle* prev = nullptr;
    rvParticle* cur = mUsedHead;
    while (cur && p->mEndTime > cur->mEndTime) {
        prev = cur;
        cur = cur->mNext;
    }
    p->mNext = cur;
    if (prev) prev->mNext = p;
    else        mUsedHead = p;
}

/* ----------------------------------------------------------- SpawnParticle */
rvParticle* rvSegment::SpawnParticle(rvBSE* effect,
    rvSegmentTemplate* st,
    float birthTime,
    const idVec3* offset,
    const idMat3* axis)
{
    rvParticle* p = nullptr;

    if (st->mFlags & 0x100) {           // re-use internal array slot
        p = mParticles;
    }
    else {
        p = mFreeHead;
        if (p) mFreeHead = p->mNext;
    }
    if (!p) return nullptr;

    p->FinishSpawn(effect, this, birthTime, birthTime, *offset, *axis);
    InsertParticle(p, st);
    return p;
}

/* ------------------------------------------------------ AttenuateDuration */
float rvSegment::AttenuateDuration(rvBSE* effect,
    rvSegmentTemplate* st)
{
    return effect->GetAttenuation(st) * (mSegEndTime - mSegStartTime);
}


/* ---------------------------------------------------------------------------
   ❶  Attenuation helpers
   --------------------------------------------------------------------------- */
float rvSegment::AttenuateInterval(rvBSE* effect,
    rvSegmentTemplate* st)
{
    const float minRate = mSecondsPerParticle.x;
    const float maxRate = mSecondsPerParticle.y;

    float rate = idMath::Lerp(maxRate, minRate, bse_scale.GetFloat());
    rate = Clamp(rate, maxRate, minRate);

    if (!(st->mFlags & 0x40))
        return rate;                                 // no attenuation flag

    float att = effect->GetAttenuation(st);
    if (st->mFlags & 0x80)
        att = 1.0f - att;                            // invert?

    return (att >= kMinSpawnRate) ? rate / att     // avoid div-0
        : 1.0f;
}

const rvSegmentTemplate* rvSegment::GetSegmentTemplate() const
{
    return mEffectDecl->GetSegmentTemplate(this->mSegmentTemplateHandle);
}

float rvSegment::AttenuateCount(rvBSE* effect,
    rvSegmentTemplate* st,
    float min, float max)
{
    const float scaledMax = idMath::Lerp(min, max, bse_scale.GetFloat());
    float count = rvRandom::flrand(min, scaledMax);
    count = Clamp(count, min, max);

    if (!(st->mFlags & 0x40))
        return count;

    float att = effect->GetAttenuation(st);
    if (st->mFlags & 0x80)
        att = 1.0f - att;

    return att * count;
}

/* ---------------------------------------------------------------------------
   ❷  Simple per-frame particle list maintenance
   --------------------------------------------------------------------------- */
void rvSegment::UpdateSimpleParticles(float timeNow)
{
    while (mUsedHead &&
        mUsedHead->mEndTime - kMinSpawnRate <= timeNow)
    {
        rvParticle* dead = mUsedHead;
        mUsedHead = dead->mNext;

        dead->mNext = mFreeHead;
        mFreeHead = dead;
        --mActiveCount;
    }
}

void rvSegment::UpdateElectricityParticles(float timeNow)
{
    mActiveCount = 0;
    for (rvParticle* p = mUsedHead; p; p = p->mNext) {
        mActiveCount += p->Update(timeNow);
    }
}

void rvSegment::RefreshParticles(rvBSE* effect,
    rvSegmentTemplate* st)
{
    if (!rvParticleTemplate::UsesEndOrigin(&st->mParticleTemplate))
        return;

    for (rvParticle* p = mUsedHead; p; p = p->mNext) {
        p->Refresh(effect, st, &st->mParticleTemplate);
    }
}

/* ---------------------------------------------------------------------------
   ❸  Generic physics + lifetime handling
   --------------------------------------------------------------------------- */
void rvSegment::UpdateGenericParticles(rvBSE* effect,
    rvSegmentTemplate* st,
    float timeNow)
{
    const bool smoker = rvSegmentTemplate::GetSmoker(st);
    const bool looping = (st->mFlags & 0x20) != 0;

    rvParticle* prev = nullptr;
    rvParticle* cur = mUsedHead;

    while (cur) {
        rvParticle* next = cur->mNext;
        bool kill = false;

        if (looping) {
            cur->RunPhysics(effect, st, timeNow);
            if (effect->mFlags & 8)
                kill = true;
        }
        else {
            if (cur->mEndTime - kMinSpawnRate <= timeNow) {
                cur->CheckTimeoutEffect(effect, st, timeNow);
                kill = true;
            }
            else {
                kill = cur->RunPhysics(effect, st, timeNow);
            }
        }

        if ((effect->mFlags & 8) && !(cur->mFlags & 0x200000))
            kill = true;

        if (smoker && st->mTrailSegmentIndex >= 0)
            cur->EmitSmokeParticles(
                effect,
                &effect->mSegments.list[st->mTrailSegmentIndex],
                timeNow);

        if (kill) {
            if (prev) prev->mNext = next;
            else        mUsedHead = next;

            cur->Destroy();
            cur->mNext = mFreeHead;
            mFreeHead = cur;
            --mActiveCount;
        }
        else {
            prev = cur;
        }
        cur = next;
    }
}

/* ---------------------------------------------------------------------------
   ❹  Segment-type-specific handling & updates
   --------------------------------------------------------------------------- */
void rvSegment::PlayEffect(rvBSE* effect,
    rvSegmentTemplate* st)
{
    const int idx =
        rvRandom::irand(0, st->mNumEffects - 1);

    game->PlayEffect(
        st->mEffects[idx],
        effect->mCurrentOrigin,
        effect->mCurrentAxis,
        /*joint*/ 0,
        vec3_origin,
        /*surfId*/ 0,
        EC_IGNORE,
        vec4_one);
}

void rvSegment::Handle(rvBSE* effect,
    rvSegmentTemplate* st,
    float timeNow)
{
    switch (st->mSegType) {
    case 2:      // continous
    case 3:      // burst
        if (effect->mFlags & 4)
            RefreshParticles(effect, st);
        break;

    case 5:      // sound
        rvBSE::UpdateSoundEmitter(effect, st, this);
        break;

    case 7:      // light
        if (st->mFlags & 1)
            HandleLight(effect, st, timeNow);   // existing engine fn
        break;

    default:    /* nothing */ break;
    }
}

void rvSegment::Handle(rvBSE* effect, float timeNow)
{
    auto* st =
        rvDeclEffect::GetSegmentTemplate(mEffectDecl, mSegmentTemplateHandle);
    if (!st || timeNow < mSegStartTime)
        return;

    Handle(effect, st, timeNow);
}

/* ---------------------------------------------------------------------------
   ❺  Per-frame UpdateParticles entry
   --------------------------------------------------------------------------- */
bool rvSegment::UpdateParticles(rvBSE* effect, float timeNow)
{
    auto* st =
        rvDeclEffect::GetSegmentTemplate(mEffectDecl, mSegmentTemplateHandle);
    if (!st) return false;

    Handle(effect, timeNow);

    const bool forceGeneric =
        (effect->mFlags & 8) || (st->mFlags & 0x200);

    if (forceGeneric)
        UpdateGenericParticles(effect, st, timeNow);
    else
        UpdateSimpleParticles(timeNow);

    if (st->mParticleTemplate.mType == 7)
        UpdateElectricityParticles(timeNow);

    /* debug HUD stats -------------------------------------------------- */
    if (com_debugHudActive) {
        dword_1137DDB0 += mActiveCount;
        if (mUsedHead)
            dword_1137DDB4 +=
            rvSegmentTemplate::GetTexelCount(st);
    }
    return mUsedHead != nullptr;
}

/* ---------------------------------------------------------------------------
   ❻  Rendering helpers
   --------------------------------------------------------------------------- */
void rvSegment::RenderMotion(rvBSE* effect,
    const renderEffect_s* owner,
    rvRenderModelBSE* model,
    rvParticleTemplate* pt,
    float timeNow)
{
    const int segments = mActiveCount *
        (static_cast<int>(ceilf(pt->mTrailCount.y)) + 1);

    srfTriangles_s* tri = R_AllocStaticTriSurf();
    R_AllocStaticTriSurfVerts(tri, 2 * segments + 2);
    R_AllocStaticTriSurfIndexes(tri, 12 * segments);

    const idMaterial* mat = pt->mTrailMaterial;

    for (rvParticle* p = mUsedHead; p; p = p->mNext)
        p->RenderMotion(effect, tri, owner, timeNow);

    R_BoundTriSurf(tri);
    model->AddSurface( /*id*/0, mat, tri, 0);
}

void rvSegment::RenderTrail(rvBSE* effect,
    const renderEffect_s* owner,
    rvRenderModelBSE* model,
    float timeNow)
{
    auto* st =
        rvDeclEffect::GetSegmentTemplate(mEffectDecl, mSegmentTemplateHandle);
    if (!st) return;

    auto* pt = &st->mParticleTemplate;
    if (ceilf(pt->mTrailCount.y) < 0 ||
        pt->mTrailTime.y < kMinSpawnRate ||
        pt->mTrailType != 2)
        return;

    RenderMotion(effect, owner, model, pt, timeNow);
}

void rvSegment::Render(rvBSE* effect,
    const renderEffect_s* owner,
    rvRenderModelBSE* model,
    float timeNow)
{
    auto* st =
        rvDeclEffect::GetSegmentTemplate(mEffectDecl, mSegmentTemplateHandle);
    if (!st) return;

    const size_t bytesNeeded =
        (mActiveCount * st->mParticleTemplate.mVertexCount) * sizeof(idDrawVert);

    if (bytesNeeded > 1 * 1024 * 1024) {     // >1 MiB safety
        common->Warning("^4BSE:^1 '%s'\nMore than a MiB of vertex data",
            rvBSE::GetDeclName(effect));
        return;
    }

    srfTriangles_s* tri = R_AllocStaticTriSurf();
    R_AllocStaticTriSurfVerts(tri, mActiveCount * st->mParticleTemplate.mVertexCount);
    R_AllocStaticTriSurfIndexes(tri, mActiveCount * st->mParticleTemplate.mIndexCount);
    tri->shader = st->mParticleTemplate.mMaterial;

    /* build view-aligned axes once per call --------------------------- */
    idMat3 viewAxis;
    idMat3 modelMat;
    R_AxisToModelMatrix(&owner->axis, &owner->origin, modelMat.ToFloatPtr());
    R_GlobalVectorToLocal(modelMat.ToFloatPtr(), &effect->mViewAxis[1], &viewAxis[1]);
    R_GlobalVectorToLocal(modelMat.ToFloatPtr(), &effect->mViewAxis[2], &viewAxis[2]);
    idVec3 toEye = effect->mViewOrg - owner->origin;
    viewAxis[0].x = toEye * owner->axis[0];
    viewAxis[0].y = toEye * owner->axis[1];
    viewAxis[0].z = toEye * owner->axis[2];

    /* draw each particle --------------------------------------------- */
    for (rvParticle* p = mUsedHead; p; p = p->mNext) {
        if (st->mFlags & 0x20)
            p->mEndTime = timeNow + 1.0f;

        if (p->Render(effect, &viewAxis, tri, timeNow) &&
            st->mParticleTemplate.mTrailType == 1)
        {
            p->RenderBurnTrail(effect, &viewAxis, tri, timeNow);
        }
    }

    R_BoundTriSurf(tri);
    model->AddSurface( /*id*/0, tri->shader, tri, 0);
}

/* ---------------------------------------------------------------------------
   ❼  Book-keeping / queries
   --------------------------------------------------------------------------- */
float rvSegment::EvaluateCost() const
{
    const auto* st =
        rvDeclEffect::GetSegmentTemplate(mEffectDecl, mSegmentTemplateHandle);
    if (!st || !(st->mFlags & 1))
        return 0.0f;

    const int  segType = st->mSegType;
    float cost = s_segmentBaseCost[segType];

    if (st->mParticleTemplate.mType) {
        cost += rvParticleTemplate::CostTrail(&st->mParticleTemplate,
            static_cast<float>(mActiveCount));
        if (st->mParticleTemplate.mFlags & 0x200)
            cost += mActiveCount * 80.0f;
    }
    return cost;
}

bool rvSegment::Active() const
{
    const auto* st =
        rvDeclEffect::GetSegmentTemplate(mEffectDecl, mSegmentTemplateHandle);

    return st && (st->mFlags & 4) && mActiveCount;
}

bool rvSegment::GetLocked() const
{
    const auto* st =
        rvDeclEffect::GetSegmentTemplate(mEffectDecl, mSegmentTemplateHandle);
    return st ? (st->mFlags & 2) != 0 : false;
}

/* ---------------------------------------------------------------------------
   ❽  Particle array allocation
   --------------------------------------------------------------------------- */
rvParticle* rvSegment::InitParticleArray(rvBSE* effect)
{
    /* decide how many -------------------------------------------------- */
    const auto* st =
        rvDeclEffect::GetSegmentTemplate(mEffectDecl, mSegmentTemplateHandle);
    if (!st) return nullptr;

    const int requested =
        (effect->mFlags & 1) ? mLoopParticleCount : mParticleCount;

    const int maxAllowed = bse_maxParticles.GetInteger();
    int count = idMath::ClampInt(0, maxAllowed, requested);

    if (requested > maxAllowed) {
        common->Warning("^4BSE:^1 '%s'\nMore than %d particles required (%d)",
            rvBSE::GetDeclName(effect),
            maxAllowed, requested);
    }

    if (count == 0) return nullptr;

    /* size / ctor table ------------------------------------------------ */
    struct Table { intptr_t type, bytes, ctor, dtor; } tbl[] = {
        { 2, 416, (intptr_t)&rvLineParticle::rvLineParticle,
                    (intptr_t)&rvLineParticle::~rvLineParticle },
        { 3, 456, (intptr_t)&rvOrientedParticle::rvOrientedParticle,
                    (intptr_t)&rvOrientedParticle::~rvLineParticle },
        { 4, 400, (intptr_t)&rvDecalParticle::rvDecalParticle,
                    (intptr_t)&rvLineParticle::~rvLineParticle },
        { 5, 456, (intptr_t)&rvModelParticle::rvModelParticle,
                    (intptr_t)&rvLineParticle::~rvLineParticle },
        { 6, 616, (intptr_t)&rvLightParticle::rvLightParticle,
                    (intptr_t)&rvLightParticle::~rvLightParticle },
        { 7, 476, (intptr_t)&rvElectricityParticle::rvElectricityParticle,
                    (intptr_t)&rvLineParticle::~rvLineParticle },
        { 8, 360, (intptr_t)&rvLinkedParticle::rvLinkedParticle,
                    (intptr_t)&rvLineParticle::~rvLineParticle },
        { 9, 396, (intptr_t)&rvDebrisParticle::rvDebrisParticle,
                    (intptr_t)&rvLineParticle::~rvLineParticle },
        { -1,400, (intptr_t)&rvSpriteParticle::rvSpriteParticle,
                    (intptr_t)&rvLineParticle::~rvLineParticle }
    };

    const int pType = st->mParticleTemplate.mType;
    const Table* row = nullptr;
    for (const auto& r : tbl)
        if (r.type == pType || r.type == -1) { row = &r; break; }

    byte* mem = (byte*)Memory::Allocate(row->bytes * count + 4);
    if (!mem) return nullptr;

    *reinterpret_cast<int*>(mem) = count;          // store length
    byte* base = mem + 4;

    /* construct array in place ---------------------------------------- */
    for (int i = 0; i < count; ++i) {
        rvParticle* p = reinterpret_cast<rvParticle*>(base + i * row->bytes);
        ((void(__thiscall*)(rvParticle*)) row->ctor)(p);
        if (i < count - 1)
            p->mNext = reinterpret_cast<rvParticle*>(base + (i + 1) * row->bytes);
        else
            p->mNext = nullptr;
    }

    return reinterpret_cast<rvParticle*>(base);
}

void rvSegment::InitParticles(rvBSE* effect)
{
    if (rvDeclEffect::GetSegmentTemplate(mEffectDecl, mSegmentTemplateHandle)) {
        mParticles = InitParticleArray(effect);
        mUsedHead = nullptr;
        mFreeHead = mParticles;
        mActiveCount = 0;
    }
}

ID_INLINE static bool BSE_DecalDebug() {
    return bse_debug.GetBool();
}

void rvSegment::CreateDecal(rvBSE* effect, float timeNow)
{
    if (!bse_render.GetBool())
        return;

    rvSegmentTemplate* st =
        rvDeclEffect::GetSegmentTemplate(mEffectDecl, mSegmentTemplateHandle);
    if (!st) return;

    /* only once ------------------------------------------------------- */
    if (mFlags & 1)
        return;

    /* -------------------------------------------------------------- dbg */
    if (BSE_DecalDebug()) {
        common->Printf("BSE: Decal from segment %d (%s)\n",
            mSegmentTemplateHandle,
            st->mParticleTemplate.mMaterial ?
            st->mParticleTemplate.mMaterial->GetName() :
            "<no material>");
    }

    /* ------------------------------------------------------ parameters */
    idVec3  origin = effect->mCurrentOrigin;
    idMat3  axis = effect->mCurrentAxis;

    idVec3  size;      // filled by template callback
    idVec3  rotation;  // Euler (deg)
    idVec4  tint;      // RGBA 0-1

    st->DispatchDecalParms(tint, size, rotation); /* ← wrapper around
                                                       script callbacks in
                                                       original code */

                                                       /* --------------------------------------------------- build quad    */
    const float hw = size.x * 0.5f;
    const float hh = size.y * 0.5f;

    /* local-space corners (XY plane) */
    idVec3 local[4] = {
        idVec3(hw,  hh, 0),
        idVec3(-hw,  hh, 0),
        idVec3(-hw, -hh, 0),
        idVec3(hw, -hh, 0)
    };

    /* apply Z-rotation (only rotation.z mattered in original) */
    const float ang = DEG2RAD(rotation.z);
    const float c = idMath::Cos(ang);
    const float s = idMath::Sin(ang);

    for (auto& v : local) {
        const float x = v.x * c - v.y * s;
        const float y = v.x * s + v.y * c;
        v.x = x; v.y = y;
        v = origin + v * axis;             // to world
    }

    /* ------------------------------------------------ winding & decal */
    idFixedWinding w;
    w.Resize(4);
    for (int i = 0; i < 4; ++i)
        w.p[i] = local[i];

    /* thickness == size.z, direction == axis[2] (model “forward”) */
    const idVec3 projectionDir = axis[2];
    const idVec3 projectionOrg = origin - projectionDir * size.z * 0.5f;
    const float  projectionDepth = size.z;

    session->rw->ProjectDecalOntoWorld(
        &w,
        projectionOrg,
        projectionDir,
        projectionDepth,
        st->mParticleTemplate.mMaterial,
        tint);

    /* ---------------------------------------- mark done so it won’t
       repeat; also tick effect duration if this segment dictates it. */
    mFlags |= 1;
    if (!(st->mFlags & 0x20))
        rvBSE::SetDuration(effect,
            (mSegEndTime - timeNow) + st->mParticleTemplate.mDuration.y);
}