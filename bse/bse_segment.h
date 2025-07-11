/* ===========================================================================
   rvSegment.h ― cleaned declaration
   ========================================================================== */
#pragma once

class rvBSE;
class rvDeclEffect;
class rvParticle;
class rvParticleTemplate;
class rvSegmentTemplate;
class rvRenderModelBSE;

/* -------------------------------------------------------------------------
   rvSegment
   ------------------------------------------------------------------------- */
class rvSegment {
public:
    /* --- construction --------------------------------------------------- */
    rvSegment() = default;
    ~rvSegment();                                             /* dtor below */

    /* --- initialisation ------------------------------------------------- */
    void            Init(rvBSE* effect,
        rvDeclEffect* decl,
        int segmentTemplateHandle,
        float timeNow);

    void            Rewind(rvBSE* effect);

    void            InitParticles(rvBSE* effect);          /* alloc array */
    void            InitTime(rvBSE* effect,
        rvSegmentTemplate* st,
        float timeNow);
    void            ResetTime(rvBSE* effect, float timeNow);
    void            Advance(rvBSE* effect);          /* one loop   */

    /* --- per-frame update ----------------------------------------------- */
    bool            Check(rvBSE* effect, float timeNow);
    bool            UpdateParticles(rvBSE* effect, float timeNow);
    void            CalcCounts(rvBSE* effect, float timeNow);

    /* --- rendering ------------------------------------------------------ */
    void            Render(rvBSE* effect,
        const renderEffect_s* owner,
        rvRenderModelBSE* model,
        float timeNow);
    void            RenderTrail(rvBSE* effect,
        const renderEffect_s* owner,
        rvRenderModelBSE* model,
        float timeNow);

    /* --- misc helpers --------------------------------------------------- */
    float           EvaluateCost() const;
    bool            Active() const;
    bool            GetLocked() const;

    /* --- particle helpers ---------------------------------------------- */
    rvParticle* SpawnParticle(rvBSE* effect,
        rvSegmentTemplate* st,
        float birthTime,
        const idVec3* localOffset = &vec3_origin,
        const idMat3* localAxis = &mat3_identity);
    void            SpawnParticles(rvBSE* effect,
        rvSegmentTemplate* st,
        float birthTime,
        int count);
    void            InsertParticle(rvParticle* p,
        rvSegmentTemplate* st);

    /* --- attenuation utilities ----------------------------------------- */
    float           AttenuateDuration(rvBSE* effect, rvSegmentTemplate* st);
    float           AttenuateInterval(rvBSE* effect, rvSegmentTemplate* st);
    float           AttenuateCount(rvBSE* effect,
        rvSegmentTemplate* st,
        float min, float max);

    const rvSegmentTemplate* rvSegment::GetSegmentTemplate() const;

    /* --- static data ---------------------------------------------------- */
    static float    s_segmentBaseCost[11];   /* indexed by SegType enum */

private:
    /* rule-of-five disabled */
    rvSegment(const rvSegment&) = delete;
    rvSegment& operator= (const rvSegment&) = delete;

    /* --- internal helpers ---------------------------------------------- */
    void            ValidateSpawnRates();
    void            GetSecondsPerParticle(rvBSE* effect,
        rvSegmentTemplate* st,
        rvParticleTemplate* pt);
    void            AddToParticleCount(rvBSE* effect,
        int  count,
        int  loopCount,
        float duration);
    void            CalcTrailCounts(rvBSE* effect,
        rvSegmentTemplate* st,
        rvParticleTemplate* pt,
        float duration);
    void            Handle(rvBSE* effect,
        rvSegmentTemplate* st);
    void            Handle(rvBSE* effect, float timeNow);
    void            PlayEffect(rvBSE* effect,
        rvSegmentTemplate* st);
    void            RefreshParticles(rvBSE* effect,
        rvSegmentTemplate* st);
    void            UpdateSimpleParticles(float timeNow);
    void            UpdateElectricityParticles(float timeNow);
    void            UpdateGenericParticles(rvBSE* effect,
        rvSegmentTemplate* st,
        float timeNow);
    rvParticle* InitParticleArray(rvBSE* effect);
    void            RenderMotion(rvBSE* effect,
        const renderEffect_s* owner,
        rvRenderModelBSE* model,
        rvParticleTemplate* pt,
        float timeNow);
    /* ultra-long decal routine kept separate for clarity */
    void            CreateDecal(rvBSE* effect, float timeNow);
public:
    /* --- data members --------------------------------------------------- */
    int mSegmentTemplateHandle;
    const rvDeclEffect* mEffectDecl;
    float mSegStartTime;
    float mSegEndTime;
    idVec2 mSecondsPerParticle;
    idVec2 mCount;
    int mParticleCount;
    int mLoopParticleCount;
    float mSoundVolume;
    float mFreqShift;
    int mFlags;
    float mLastTime;
    int mActiveCount;
    rvParticle* mFreeHead;
    rvParticle* mUsedHead;
    rvParticle* mParticles;

    /* clamp constants */
    static constexpr float kMinSpawnRate = 0.002f;
    static constexpr float kMaxSpawnRate = 300.0f;
};

