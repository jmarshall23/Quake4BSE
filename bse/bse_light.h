class rvLightParticle : public rvParticle {
public:
    rvLightParticle() = default;
    ~rvLightParticle() override = default;

    // ───── rvParticle interface ────────────────────────────────────────────
    virtual bool Destroy() override;   // kills render-light and frees handle
    virtual bool InitLight(rvBSE* effect,rvSegmentTemplate* st,float time) override;
    virtual bool PresentLight(rvBSE* effect, rvParticleTemplate* pt, float time, bool infinite) override;

private:
    // helpers
    void        ClampRadius();                          // ensures xyz ≥ 1
    void        SetOriginFromLocal(const idVec3& p);  // world-space origin
    void        SetAxis(const idMat3& m);    // orient light

    // ───── members ---------------------------------------------------------
    rvEnvParms3 mSizeEnv;
    int mLightDefHandle;
    renderLight_s mLight;
};