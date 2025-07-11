#pragma once

class rvSegment;

//‑‑‑ Base particle --------------------------------------------------------------------
class rvParticle {
public:
    /* life‑cycle -------------------------------------------------------------*/
    virtual ~rvParticle() = default;
    virtual void FinishSpawn(rvBSE* effect, rvSegment* segment,
        float birthTime, float fraction,
        const idVec3& initOffset,
        const idMat3& initAxis);

    virtual		void			GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate) { assert(0); }

    /* evaluation & simulation -----------------------------------------------*/
    void            EvaluateVelocity(const rvBSE* effect, idVec3& out, float time) const;
    void            EvaluatePosition(const rvBSE* effect, idVec3& out, float time);
    bool            RunPhysics(rvBSE* effect, rvSegmentTemplate* st, float time);
    void            Bounce(rvBSE* effect, rvParticleTemplate* pt,
        idVec3 endPos, idVec3 normal, float time);

    /* attenuation helpers ----------------------------------------------------*/
    void Attenuate(float a, rvParticleParms& p, rvEnvParms1& env);
    void Attenuate(float a, rvParticleParms& p, rvEnvParms2& env);
    void Attenuate(float a, rvParticleParms& p, rvEnvParms3& env);

    /* spawn‑time utilities ---------------------------------------------------*/
    void HandleEndOrigin(rvBSE* effect, rvParticleTemplate* pt,
        idVec3* normal, idVec3* centre);
    void HandleOrientation(const rvAngles* angles);    // stub (not shown)

    /* trail / timeout --------------------------------------------------------*/
    void EmitSmokeParticles(rvBSE* effect, rvSegment* child, float time);
    void CheckTimeoutEffect(rvBSE* effect, rvSegmentTemplate* st, float time);

    /* math helpers -----------------------------------------------------------*/
    void CalcImpactPoint(idVec3& impact,
        const idVec3& origin,
        const idVec3& motion,
        const idBounds& bounds,
        const idVec3& planeNormal);

    virtual void rvParticle::SetOriginUsingEndOrigin(rvBSE* effect, rvParticleTemplate* pt, idVec3* normal, idVec3* centre);

    virtual void rvParticle::RenderMotion(rvBSE* effect, srfTriangles_s* tri, const renderEffect_s* owner, float time);

    /* virtual array helpers (overridden by derived fixed‑size pools) ---------*/
    virtual rvParticle* GetArrayEntry(int index) const { return index < 0 ? nullptr : const_cast<rvParticle*>(this) + index; }
    virtual int         GetArrayIndex(rvParticle* p) const { return p ? int(reinterpret_cast<uint8_t*>(p) - reinterpret_cast<uint8_t const*>(this)) / sizeof(rvParticle) : -1; }
    virtual void RenderQuadTrail(const rvBSE* effect, srfTriangles_s* tri, const idVec3& offset, float fraction, const idVec4& colour, const idVec3& pos, bool firstSegment);
    virtual int  HandleTint(const rvBSE* effect, const idVec4& colour, float alpha) const;
    virtual bool GetEvaluationTime(float time, float& evalTime, bool infinite) const;

    /* accessor shims used in original code -----------------------------------*/
    idVec3* GetInitSize();      idVec3* GetDestSize();
    float* GetInitRotation();  float* GetDestRotation();

    /* length / direction helpers (rvLineParticle etc.) -----------------------*/
    virtual float* GetInitLength() { return nullptr; }
    virtual float* GetDestLength() { return nullptr; }
    virtual void   AttenuateLength(float, rvParticleParms*) {}

    /* transform helpers injected by compiler‑generated thunks ----------------*/
    void ScaleAngle(float s);    void ScaleRotation(float s);
    void ScaleLength(float s);   void TransformLength(int lx, int ly, int lz);

    virtual		bool			InitLight(rvBSE* effect, rvSegmentTemplate* st, float time) { return(false); }
    virtual		bool			PresentLight(rvBSE* effect, rvParticleTemplate* pt, float time, bool infinite) { return(false); }
    virtual		bool			Destroy(void) { return(false); }
    virtual		void			SetModel(const idRenderModel* model) {}
    virtual		void			SetupElectricity(rvParticleTemplate* pt) {}
    virtual		void			Refresh(rvBSE* effect, rvSegmentTemplate* st, rvParticleTemplate* pt) {}

    virtual		void			InitSizeEnv(rvEnvParms& env, float duration) { assert(0); }
    virtual		void			InitRotationEnv(rvEnvParms& env, float duration) { assert(0); }
    virtual		void			InitLengthEnv(rvEnvParms& env, float duration) { assert(0); }

    virtual		bool			Render(const rvBSE* effect, rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override = 1.0f) { return(false); }
    virtual		int				Update(rvParticleTemplate* pt, float time) { return(1); }

    virtual		void			EvaluateSize(const float time, float* dest) { assert(0); }
    virtual		void			EvaluateRotation(const float time, float* dest) { assert(0); }
    virtual		void			EvaluateLength(const float time, idVec3& dest) { assert(0); }

    void __thiscall rvParticle::SetLengthUsingEndOrigin(
        rvBSE* effect,
        rvParticleParms* parms,
        float* length)
    {
        rvParticleParms::spawnFunctions[parms->mSpawnType](length, *parms, nullptr, nullptr); // jmarshall <-- no idea hex rays HATTES this call
    }
public:
    // --- data copied verbatim from dump (trim/rename as needed) -------------
    rvParticle* mNext;
    float mMotionStartTime;
    float mLastTrailTime;
    int mFlags;
    float mStartTime;
    float mEndTime;
    float mTrailTime;
    int mTrailCount;
    float mFraction;
    float mTextureScale;
    idVec3 mInitEffectPos;
    idMat3 mInitAxis;
    idVec3 mInitPos;
    idVec3 mNormal;
    idVec3 mVelocity;
    idVec3 mAcceleration;
    idVec3 mFriction;
    rvEnvParms3 mTintEnv;
    rvEnvParms1 mFadeEnv;
    rvEnvParms3 mAngleEnv;
    rvEnvParms3 mOffsetEnv;
};

//‑‑‑ Specialized particles ------------------------------------------------------------
class rvLineParticle : public rvParticle {
public:
    /* overrides */
    void        HandleTiling(rvParticleTemplate* pt);
    rvLineParticle* GetArrayEntry(int) const override;
    int         GetArrayIndex(rvParticle* p) const override;
    void        FinishSpawn(rvBSE*, rvSegment*, float, float, const idVec3&, const idMat3&) override;
    void        Refresh(rvBSE*, rvSegmentTemplate*, rvParticleTemplate*);
    void        GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate) override;

    virtual bool Render(const rvBSE* effect, rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override) override;

    /* length helpers */
    float* GetInitLength() override;
    float* GetDestLength() override;
    void        AttenuateLength(float atten, rvParticleParms* parms) override;

    void __thiscall rvLineParticle::EvaluateLength(float time, idVec3* dest)
    {
        mLengthEnv.Evaluate(time, &dest->x);
    }

private:
    rvEnvParms1 mSizeEnv;
    rvEnvParms3 mLengthEnv;
};

class rvLinkedParticle : public rvParticle {
public:
    void        HandleTiling(rvParticleTemplate* pt);
    rvLinkedParticle* GetArrayEntry(int) const override;
    int         GetArrayIndex(rvParticle* p) const override;
    void        FinishSpawn(rvBSE*, rvSegment*, float, float, const idVec3&, const idMat3&) override;
    virtual bool Render(const rvBSE* effect, rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override) override;
};

class rvDecalParticle : public rvParticle {
public:
    rvDecalParticle* GetArrayEntry(int) const override;
    int         GetArrayIndex(rvParticle* p) const override;
    virtual bool Render(const rvBSE* effect, rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override) override;
};

class rvModelParticle : public rvParticle {
public:
    virtual bool Render(const rvBSE* effect, rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override) override;
};
class rvOrientedParticle : public rvModelParticle {
public:
    rvOrientedParticle* GetArrayEntry(int) const override;
    int         GetArrayIndex(rvParticle* p) const override;
    void        GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate) override;
    virtual bool Render(const rvBSE* effect, rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override) override;
};

class rvSpriteParticle : public rvParticle {
public:
    void        GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate) override;

    virtual bool Render(const rvBSE* effect, rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override = 1.0f) override;
};

class rvDebrisParticle : public rvParticle {
public:
    rvDebrisParticle* GetArrayEntry(int) const override;
    int         GetArrayIndex(rvParticle* p) const override;
    void        FinishSpawn(rvBSE*, rvSegment*, float, float, const idVec3&, const idMat3&) override;
    virtual bool Render(const rvBSE* effect, rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override) override;
};

