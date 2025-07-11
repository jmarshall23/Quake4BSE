// bse_segmenttemplate.h
//

/* -------------------------------------------------  flags & enums  ------ */
enum rvSegTemplateFlags : uint32_t
{
    STF_LOCKED = 1 << 0,   // “locked”   – stays fixed to owner
    STF_CONSTANT = 1 << 1,   // “constant” – used for linked segs
    STF_EMITTER_ATTEN = 1 << 6,   // “attenuateEmitter”
    STF_EMITTER_INV_ATTEN = 1 << 7,   // “inverseAttenuateEmitter”
    STF_HAS_SOUND = 1 << 8,   // runtime – SoundShader assigned
    STF_IS_TRAIL = 1 << 9,   // runtime – seg spawns trail seg
    STF_IS_PARTICLE = 1 << 10,  // runtime – seg owns particles
    STF_IS_LIGHT = 1 << 11,  // runtime – seg owns light
    STF_DETAIL_CULL = 1 << 12,  // runtime – culled by detail thresh
    STF_MAX_DURATION = 1 << 13   // “channel”-only seg – use snd len
};

enum rvSegTemplateType
{
    SEG_PARTICLE = 0,
    SEG_SPRITE = 1,
    SEG_LINE = 2,
    SEG_ORIENTED = 3,
    SEG_DECAL = 4,
    SEG_MODEL = 5,
    SEG_LIGHT = 6,
    SEG_ELECTRIC = 7,
    SEG_LINKED = 8,
    SEG_DEBRIS = 9,
    SEG_SOUND = 10,   // “channel”-only segment
    SEG_INVALID = 255
};

class rvSegmentTemplate
{
public:
    rvSegmentTemplate() { Init(nullptr); }

    enum
    {
        SEG_NONE = 0x0,
        SEG_EFFECT = 0x1,
        SEG_EMITTER = 0x2,
        SEG_SPAWNER = 0x3,
        SEG_TRAIL = 0x4,
        SEG_SOUND = 0x5,
        SEG_DECAL = 0x6,
        SEG_LIGHT = 0x7,
        SEG_DELAY = 0x8,
        SEG_DV = 0x9,
        SEG_SHAKE = 0xA,
        SEG_TUNNEL = 0xB,
        SEG_COUNT = 0xC,
    };

    /* construction helpers ------------------------------------------------ */
    void             Init(rvDeclEffect* decl);
    bool             Parse(rvDeclEffect* effect,
        int            segmentType,
        idLexer* lexer);
    bool             Finish(rvDeclEffect* effect);
    void             EvaluateTrailSegment(rvDeclEffect* effect);

    /* particle helpers ---------------------------------------------------- */
    void             CreateParticleTemplate(rvDeclEffect* effect,
        idLexer* lexer,
        int           particleType);

    /* run-time queries ----------------------------------------------------- */
    int              GetTexelCount() const;
    bool             GetSmoker() const;
    bool             Compare(const rvSegmentTemplate& rhs) const;
    bool             GetSoundLooping() const;
    bool             DetailCull() const;

    /* lifetime helpers ---------------------------------------------------- */
    void             SetMinDuration(rvDeclEffect* effect);
    void             SetMaxDuration(rvDeclEffect* effect);

    /* shorthand access ---------------------------------------------------- */
    ID_INLINE const  idStr& GetName()      const { return mSegmentName; }
    ID_INLINE        rvSegTemplateType  GetType()   const { return static_cast<rvSegTemplateType>(mSegType); }

    float               CalculateBounds();
public:
    /* data – identical order to the dump so pointer arithmetic stays valid */
    rvDeclEffect* mDeclEffect{ nullptr };

    uint32_t               mFlags{ STF_CONSTANT };  // see enum above
    uint8_t                mSegType{ SEG_INVALID };   // rvSegTemplateType

    idVec2                 mLocalStartTime{ 0.0f, 0.0f }; // min,max
    idVec2                 mLocalDuration{ 0.0f, 0.0f };
    idVec2                 mAttenuation{ 0.0f, 0.0f };

    float                  mParticleCap{ 0.0f };
    float                  mDetail{ 0.0f };
    float                  mScale{ 1.0f };

    idVec2                 mCount{ 1.0f, 1.0f };
    idVec2                 mDensity{ 0.0f, 0.0f };

    int32_t                mTrailSegmentIndex{ -1 };

    int32_t                mNumEffects{ 0 };
    const rvDeclEffect* mEffects[4]{ nullptr, nullptr, nullptr, nullptr };

    const idSoundShader* mSoundShader{ nullptr };
    idVec2                 mSoundVolume{ 0.0f, 0.0f };
    idVec2                 mFreqShift{ 1.0f, 1.0f };

    rvParticleTemplate     mParticleTemplate;

    rvBSE*                  mBSEEffect{ nullptr };

    idStr                  mSegmentName;
};