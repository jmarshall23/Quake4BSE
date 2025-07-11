#pragma once

/*
===============================================================================

    rvDeclEffect
    ------------
    Declarative description of a complex visual effect.  Each effect
    is built from one or more rvSegmentTemplate objects (particles,
    emitters, decals, lights, trails, etc.).

===============================================================================
*/
class rvDeclEffect final : public idDecl {
public:
    // --------------------------------------------------------------------- //
    //  Life-cycle
    // --------------------------------------------------------------------- //
    rvDeclEffect();                         // constructor
    virtual        ~rvDeclEffect() override;               // destructor

    // --------------------------------------------------------------------- //
    //  idDecl interface overrides
    // --------------------------------------------------------------------- //
    const char* DefaultDefinition() const override;    // implicit text used by the material editor
    bool            SetDefaultText()       override;       // injects the default definition into this->base
    bool            Parse(const char* text, const int textLength)        override;
    virtual size_t	Size(void) const          override;       // idDecl virtual
    void            FreeData()             override;       // idDecl virtual

    // --------------------------------------------------------------------- //
    //  Public helpers
    // --------------------------------------------------------------------- //
    void            Init();                                // reset to initial “empty” state
    void            Finish();                              // final-pass after Parse()
    void            CopyData(const rvDeclEffect& src);
    void            Revert();                              // restore mEditorOriginal
    void            CreateEditorOriginal();
    bool            CompareToEditorOriginal() const;
    void            DeleteEditorOriginal();

    // segment helpers
    rvSegmentTemplate* GetSegmentTemplate(const char* name);
    rvSegmentTemplate* GetSegmentTemplate(int index);
    int                 GetTrailSegmentIndex(const idStr& name) const;

    // duration helpers
    void            SetMinDuration(float duration);
    void            SetMaxDuration(float duration);

public:
    // internal comparison used by the editor diff logic
    bool            Compare(const rvDeclEffect& rhs) const;
    float           CalculateBounds() const;               // expensive – walk segments and find radius

    // --------------------------------------------------------------------- //
    //  Private data
    // --------------------------------------------------------------------- //
    idList< rvSegmentTemplate >   mSegmentTemplates{ 16 }; // granularity 16 by default
    rvDeclEffect* mEditorOriginal = nullptr;          // deep-copied “snapshot” shown in the editor
public:
    int              mFlags = 0;                 // bit-field (see Finish())
    float            mMinDuration = 0.0f;
    float            mMaxDuration = 0.0f;
    float            mSize = 512.0f;            // editor preview bounds
    int              mPlayCount = 0;                 // how many times we’ve played (runtime, not parsed)
    int              mLoopCount = 0;                 // loops before auto-stop
};
