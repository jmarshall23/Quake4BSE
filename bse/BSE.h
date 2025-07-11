#include "../renderer/tr_local.h"
#include "../renderer/Model_local.h"

static idVec2 idVec2_zero(0, 0);

#define SEGMENT_NO_SHADOWS 0x20000
#define SEGMENT_NO_SPECULAR 0x40000
#define F_STOP_REQUESTED 8
#define PTF_LINKED_TRAIL 8

class rvParticleParms
{
public:
    //— SpawnType enumeration mirrors the game ticker values ————————
    enum SpawnType : uint32_t
    {
        SPAWN_NONE = 0x00,
        SPAWN_CONSTANT_ONE = 0x05,
        SPAWN_AT_POINT = 0x09,
        SPAWN_LINEAR = 0x0D,
        SPAWN_BOX = 0x11,
        SPAWN_SURFACE_BOX = 0x15,
        SPAWN_SPHERE = 0x19,
        SPAWN_SURFACE_SPHERE = 0x1D,
        SPAWN_CYLINDER = 0x21,
        SPAWN_SURFACE_CYLINDER = 0x25,
        SPAWN_SPIRAL = 0x29,
        SPAWN_MODEL = 0x2D
    };

    //— Public data kept identical to the Hex-Rays layout ————————
    uint32_t   mSpawnType{ SPAWN_NONE };
    uint32_t   mFlags{ 0 };           // bit-field (see code)
    idVec3     mMins{ };             // local bounding box
    idVec3     mMaxs{ };
    float      mRange{ 1.0f };        // spiral radius, etc.
    void* mMisc{ nullptr };     // model / extra payload

    bool operator!=(const rvParticleParms& rhs) const
    {
        if (mSpawnType != rhs.mSpawnType) return true;
        if (mFlags != rhs.mFlags)     return true;
        if (mMisc != rhs.mMisc)      return true;      // pointer identity

        // idVec3 already has an epsilon-aware Compare()
        if (!mMins.Compare(rhs.mMins, kEpsilon))  return true;
        if (!mMaxs.Compare(rhs.mMaxs, kEpsilon))  return true;
        if (fabs(mRange - rhs.mRange) > kEpsilon) return true;

        return false; // everything matched
    }

    using SpawnFunc = void (*)(float*               /*scratch*/,
        const rvParticleParms& /*template parms*/,
        idVec3*              /*in-out position*/,
        const idVec3*        /*reference*/);

    static rvParticleParms::SpawnFunc spawnFunctions[48];

    //— Small helpers ————————————————————————————————————————————————
    bool  Compare(const rvParticleParms& rhs) const;
    void  HandleRelativeParms(float* death, float* init, int count);
    void  GetMinsMaxs(idVec3& mins, idVec3& maxs) const;

    static constexpr float kEpsilon = 0.001f;
};

#include "bse_effect.h"
#include "bse_effecttemplate.h"
#include "bse_envelope.h"
#include "bse_segment.h"
#include "bse_particle.h"
#include "bse_light.h"
#include "bse_parseparticle2.h"
#include "bse_segmenttemplate.h"
#include "bse_spawndomains.h"
#include "bse_electricity.h"

//──────────────────────────────────────────────────────────────────────────────
//  Constants & helpers
//──────────────────────────────────────────────────────────────────────────────
namespace {
    constexpr float   kDecayPerFrame = 0.1f;      // how fast rate-limit credits decay
    constexpr size_t  kMaxCategories = 3;         // EC_IGNORE, EC_SMALL, EC_LARGE (example)
}

class rvBSEManagerLocal final : public rvBSEManager {
public:
    //──────────────────────────────
    // Lifetime
    //──────────────────────────────
    rvBSEManagerLocal() = default;
    ~rvBSEManagerLocal()  override = default;

    bool            Init()                      override;
    bool            Shutdown()                      override;

    //──────────────────────────────
    // Per-frame & level control
    //──────────────────────────────
    void            BeginLevelLoad()                      override { /* nop */ }
    void            EndLevelLoad()                      override;              // clear rates
    void            StartFrame()                      override;              // reset perf HUD
    void            EndFrame()                      override;              // push perf HUD
    void            UpdateRateTimes()                      override;              // decay credits

    //──────────────────────────────
    // Effect API
    //──────────────────────────────
    bool            PlayEffect(rvRenderEffectLocal* def, float now) override;
    bool            ServiceEffect(rvRenderEffectLocal* def, float now) override;
    void            StopEffect(rvRenderEffectLocal* def)            override;
    void            FreeEffect(rvRenderEffectLocal* def)            override;
    float           EffectDuration(const rvRenderEffectLocal* def)      override;

    bool            CheckDefForSound(const renderEffect_t* def)           override;

    // view-kick helpers exposed to script
    void            SetDoubleVisionParms(float time, float scale);
    void            SetShakeParms(float time, float scale);
    void            SetTunnelParms(float time, float scale);

    // rate-limit helpers
    bool            Filtered(const char* name, effectCategory_t c) override;
    bool            CanPlayRateLimited(effectCategory_t c)                 override;

    // trace models
    int             AddTraceModel(idTraceModel* m)                override;
    idTraceModel* GetTraceModel(int idx)                        override;
    void            FreeTraceModel(int idx)                        override;

    // debug console commands
    static  void    Cmd_Stats(const idCmdArgs& args);
    static  void    Cmd_Log(const idCmdArgs& args);

public:
    //──────────────────────────────
    // Internal helpers / data
    //──────────────────────────────
    bool DebugHudActive() const { return false; }

    idBlockAlloc<rvBSE, 256, 26>          effects_;             // effect pool
    static idMat3                          mModelToBSE;
    idList<idTraceModel*>          traceModels;         // loose heap-allocated models
};

extern rvBSEManagerLocal bseLocal;