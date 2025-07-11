
class rvParticleTemplate {
public:
    // ---------------------------------------------------------------------
    //  Lifetime helpers
    // ---------------------------------------------------------------------
    rvParticleTemplate() { Init(); }
    void        Init();                               // zero + sane defaults
    void        Finish();                             // post-parse fix-ups

    // ---------------------------------------------------------------------
    //  Query helpers
    // ---------------------------------------------------------------------
    bool        UsesEndOrigin()                 const;
    void        SetParameterCounts();                 // derive parm counts
    float       GetSpawnVolume(rvBSE* fx)      const;
    float       CostTrail(float baseCost)      const;
    idTraceModel* GetTraceModel()               const;
    int         GetTrailCount()                 const;

    float       GetFurthestDistance() const;

    void        EvaluateSimplePosition(
        idVec3* pos,
        float time,
        float lifeTime,
        const idVec3* initPos,
        const idVec3* velocity,
        const idVec3* acceleration,
        const idVec3* friction
    );

    // ---------------------------------------------------------------------
    //  Parsing helpers (return true on success)
    // ---------------------------------------------------------------------
    bool        Parse(rvDeclEffect* effect, idLexer* src);
    bool        ParseSpawnDomains(rvDeclEffect* effect, idLexer* src);
    bool        ParseMotionDomains(rvDeclEffect* effect, idLexer* src);
    bool        ParseDeathDomains(rvDeclEffect* effect, idLexer* src);
    bool        ParseImpact(rvDeclEffect* effect, idLexer* src);
    bool        ParseTimeout(rvDeclEffect* effect, idLexer* src);
    bool        ParseBlendParms(rvDeclEffect* effect, idLexer* src);

    // ---------------------------------------------------------------------
    //  Comparisons / utilities
    // ---------------------------------------------------------------------
    bool        Compare(const rvParticleTemplate& rhs)   const;
    void        FixupParms(rvParticleParms& p);
    static bool GetVector(idLexer* src, int components, idVec3& out);
    static bool CheckCommonParms(idLexer* src, rvParticleParms& p);

    float       GetMaxParmValue(rvParticleParms* spawn,rvParticleParms* death,rvEnvParms* envelope);
public:
    // ── sub-parsers (internal) ────────────────────────────────────────────
    bool        ParseSpawnParms(rvDeclEffect*, idLexer*, rvParticleParms&, int vecCount);
    bool        ParseMotionParms(idLexer*, int vecCount, rvEnvParms&);
    int         GetMaxTrailCount() const;                      // helper

    // ── generic flags bitfield (Init zeros it) ────────────────────────────
    uint32      mFlags;                 // 0x00000001 == parsed, others per bits

    // ── basic type / material / model info ────────────────────────────────
    uint8       mType;                         // 0-9 primitive type enum
    idStr       mMaterialName;                 // parsed “material”
    const idMaterial* mMaterial;             // resolved pointer
    idStr       mModelName;                    // parsed “model”
    int         mTraceModelIndex;              // into trace-model pool

    // ── physics / timing & render counts ──────────────────────────────────
    idVec2      mGravity;                      // min, max
    idVec2      mSoundVolume;                  // optional
    idVec2      mFreqShift;                    // optional
    idVec2      mDuration;                     // life (min,max)
    float       mBounce;                       // elasticity 0-1
    float       mTiling;                       // UV tiling (>=.002)
    int         mVertexCount;                  // set in Finish()
    int         mIndexCount;                   // set in Finish()

    // ── trail rendering parameters ────────────────────────────────────────
    int         mTrailType;                    // 0-none,1-burn,2-motion,3-custom
    idStr       mTrailTypeName;                // custom name (type==3)
    idStr       mTrailMaterialName;            // override material
    const idMaterial* mTrailMaterial;
    idVec2      mTrailTime;                    // life of a trail segment
    idVec2      mTrailCount;                   // segments emitted / particle

    // ── fork & jitter extras ──────────────────────────────────────────────
    int         mNumForks;                     // 0-16
    idVec3      mForkSizeMins;
    idVec3      mForkSizeMaxs;
    idVec3      mJitterSize;                   // per-frame deviation
    float       mJitterRate;                   // Hz
    const idDeclTable* mJitterTable;          // lookup

    // ── centre of spawn bounds (pre-calc) ─────────────────────────────────
    idVec3      mCentre;

    // ── component counts derived from mType ───────────────────────────────
    int         mNumSizeParms;                 // 0-3
    int         mNumRotateParms;               // 0-3

    // ── SPAWN domains ─────────────────────────────────────────────────────
    rvParticleParms  mSpawnPosition;
    rvParticleParms  mSpawnDirection;
    rvParticleParms  mSpawnVelocity;
    rvParticleParms  mSpawnAcceleration;
    rvParticleParms  mSpawnFriction;
    rvParticleParms  mSpawnTint;
    rvParticleParms  mSpawnFade;
    rvParticleParms  mSpawnSize;
    rvParticleParms  mSpawnRotate;
    rvParticleParms  mSpawnAngle;
    rvParticleParms  mSpawnOffset;
    rvParticleParms  mSpawnLength;

    // ── PER-FRAME ENVELOPES ───────────────────────────────────────────────
    rvEnvParms       mTintEnvelope;
    rvEnvParms       mFadeEnvelope;
    rvEnvParms       mSizeEnvelope;
    rvEnvParms       mRotateEnvelope;
    rvEnvParms       mAngleEnvelope;
    rvEnvParms       mOffsetEnvelope;
    rvEnvParms       mLengthEnvelope;

    // ── DEATH (timeout) domains ───────────────────────────────────────────
    rvParticleParms  mDeathTint;
    rvParticleParms  mDeathFade;
    rvParticleParms  mDeathSize;
    rvParticleParms  mDeathRotate;
    rvParticleParms  mDeathAngle;
    rvParticleParms  mDeathOffset;
    rvParticleParms  mDeathLength;

    // ── impact / timeout effect hooks ─────────────────────────────────────
    int                 mNumImpactEffects;
    int                 mNumTimeoutEffects;
    const rvDeclEffect* mImpactEffects[4];
    const rvDeclEffect* mTimeoutEffects[4];

    // ── optional entityDef attachment (for spawned rigid-bodys etc.) ──────
    idStr       mEntityDefName;
};