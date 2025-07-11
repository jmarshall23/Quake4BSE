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

//==========================================================================//
//  Local helpers
//==========================================================================//

namespace {

    enum class eSegType : uint8_t {
        kInvalid = 0,
        kEffect = 1,
        kEmitter = 2,
        kSpawner = 3,
        kTrail = 4,
        kSound = 5,
        kDecal = 6,
        kLight = 7,
        kDelay = 8,
        kDoubleVision = 9,
        kShake = 10,
        kTunnel = 11
    };

    // convert “emitter”, “sound”, … token → enum
    static eSegType TokenToSegType(const idStr& tok) {
        if (!tok.Icmp("effect")) return eSegType::kEffect;
        if (!tok.Icmp("emitter")) return eSegType::kEmitter;
        if (!tok.Icmp("spawner")) return eSegType::kSpawner;
        if (!tok.Icmp("trail")) return eSegType::kTrail;
        if (!tok.Icmp("sound")) return eSegType::kSound;
        if (!tok.Icmp("decal")) return eSegType::kDecal;
        if (!tok.Icmp("light")) return eSegType::kLight;
        if (!tok.Icmp("delay")) return eSegType::kDelay;
        if (!tok.Icmp("doubleVision")) return eSegType::kDoubleVision;
        if (!tok.Icmp("shake")) return eSegType::kShake;
        if (!tok.Icmp("tunnel")) return eSegType::kTunnel;
        return eSegType::kInvalid;
    }

} // anonymous namespace

//==========================================================================//
//  rvDeclEffect – life-cycle
//==========================================================================//

rvDeclEffect::rvDeclEffect() {
    // base is assigned by idDecl::Create after construction
    Init();
}

//------------------------------------------------------------------------

rvDeclEffect::~rvDeclEffect() {
    DeleteEditorOriginal();         // frees mEditorOriginal
    FreeData();                     // releases mSegmentTemplates, sets them empty
}

//==========================================================================//
//  idDecl overrides
//==========================================================================//

const char* rvDeclEffect::DefaultDefinition() const {
    // the editor inserts “effect myEffect // IMPLICITLY GENERATED”
    static const char* sDefault = "{\n}\n";
    return sDefault;
}

//------------------------------------------------------------------------

bool rvDeclEffect::SetDefaultText() {
    char    generated[1024];
    idStr::snPrintf(generated, sizeof(generated),
        "effect %s // IMPLICITLY GENERATED\n%s",
        base->GetName(), DefaultDefinition());

    base->SetText(generated);
    return true;
}

//------------------------------------------------------------------------

size_t rvDeclEffect::Size() const {
    // sizeof(rvSegmentTemplate)==0x5CC (1484) in original dump
    return static_cast<unsigned>(52 + mSegmentTemplates.Size() * 1484);
}

//------------------------------------------------------------------------

void rvDeclEffect::FreeData() {
    // Clear all segment templates
    for (int i = 0; i < mSegmentTemplates.Num(); ++i) {
        mSegmentTemplates[i].~rvSegmentTemplate();
    }
    mSegmentTemplates.Clear();
}

//==========================================================================//
//  helpers
//==========================================================================//

void rvDeclEffect::Init() {
    DeleteEditorOriginal();

    mFlags = 0;
    mMinDuration = 0.0f;
    mMaxDuration = 0.0f;
    mSize = 512.0f;
    mPlayCount = 0;
    mLoopCount = 0;

    FreeData();                     // clears the list
}

//------------------------------------------------------------------------

void rvDeclEffect::DeleteEditorOriginal() {
    if (mEditorOriginal) {
        delete mEditorOriginal;
        mEditorOriginal = nullptr;
    }
}

//------------------------------------------------------------------------

void rvDeclEffect::CreateEditorOriginal() {
    DeleteEditorOriginal();
    mEditorOriginal = new rvDeclEffect(*this);          // uses copy-ctor
}

//------------------------------------------------------------------------

void rvDeclEffect::CopyData(const rvDeclEffect& src) {
    Init();                                         // empty first

    // shallow-copy POD
    mFlags = src.mFlags;
    mSize = src.mSize;
    mMinDuration = src.mMinDuration;
    mMaxDuration = src.mMaxDuration;

    // deep-copy segments
    mSegmentTemplates.SetGranularity(16);
    mSegmentTemplates.SetNum(src.mSegmentTemplates.Num());
    for (int i = 0; i < src.mSegmentTemplates.Num(); ++i) {
        mSegmentTemplates[i] = src.mSegmentTemplates[i];
    }
}

//------------------------------------------------------------------------

bool rvDeclEffect::Compare(const rvDeclEffect& rhs) const {
    if (mSegmentTemplates.Num() != rhs.mSegmentTemplates.Num())
        return false;

    for (int i = 0; i < mSegmentTemplates.Num(); ++i) {
        if (!mSegmentTemplates[i].Compare(rhs.mSegmentTemplates[i]))
            return false;
    }
    return true;
}

//------------------------------------------------------------------------

bool rvDeclEffect::CompareToEditorOriginal() const {
    return mEditorOriginal ? Compare(*mEditorOriginal) : false;
}

//------------------------------------------------------------------------

void rvDeclEffect::SetMinDuration(float duration) {
    if (duration > mMinDuration)
        mMinDuration = duration;
}

void rvDeclEffect::SetMaxDuration(float duration) {
    if (duration > mMaxDuration)
        mMaxDuration = duration;
}

//------------------------------------------------------------------------

rvSegmentTemplate* rvDeclEffect::GetSegmentTemplate(const char* name) {
    for (int i = 0; i < mSegmentTemplates.Num(); ++i) {
        if (!idStr::Icmp(mSegmentTemplates[i].GetName(), name))
            return &mSegmentTemplates[i];
    }
    return nullptr;
}

rvSegmentTemplate* rvDeclEffect::GetSegmentTemplate(int index) {
    return (index >= 0 && index < mSegmentTemplates.Num())
        ? &mSegmentTemplates[index]
        : nullptr;
}

//------------------------------------------------------------------------

int rvDeclEffect::GetTrailSegmentIndex(const idStr& name) const {
    for (int i = 0; i < mSegmentTemplates.Num(); ++i) {
        const rvSegmentTemplate& seg = mSegmentTemplates[i];
        if (!idStr::Icmp(name, seg.GetName()))
            return i;
    }

    common->Warning("^4BSE:^1 Unable to find trail segment '%s'", name.c_str());
    return -1;
}

//------------------------------------------------------------------------

float rvDeclEffect::CalculateBounds() const {
    int v2; // esi
    int v3; // ebx
    long double v4; // st7
    float size; // [esp+10h] [ebp-4h]

    v2 = 0;
    size = 0.0;
    if (this->mSegmentTemplates.Num() > 0)
    {
        v3 = 0;
        do
        {
            v4 = mSegmentTemplates[v3].CalculateBounds();// rvSegmentTemplate::CalculateBounds(&this->mSegmentTemplates.list[v3]);
            if (v4 > size)
                size = v4;
            ++v2;
            ++v3;
        } while (v2 < this->mSegmentTemplates.Num());
    }
    return ceil(size);
}

//------------------------------------------------------------------------

void rvDeclEffect::Finish() {
    // clear flags first
    mFlags &= ~(1 | 2 | 4);
    mMinDuration = 0.0f;
    mMaxDuration = 0.0f;

    // second pass over every segment
    for (int i = 0; i < mSegmentTemplates.Num(); ++i) {
        rvSegmentTemplate& seg = mSegmentTemplates[i];

        seg.Finish(this);

        // track aggregate flags
        if (seg.GetType() == rvSegmentTemplate::SEG_SOUND)
            mFlags |= 1;
        if (seg.mParticleTemplate.UsesEndOrigin())
            mFlags |= 2;
        if ((seg.mFlags & 0x40) != 0)
            mFlags |= 4;

        // min/ max duration across all segments
        seg.SetMinDuration(this);
        seg.SetMaxDuration(this);
    }

    mSize = CalculateBounds();
}

//------------------------------------------------------------------------

void rvDeclEffect::Revert() {
    if (!mEditorOriginal) {
        common->Warning("rvDeclEffect::Revert – no editorOriginal to apply");
        return;
    }

    CopyData(*mEditorOriginal);
    CreateEditorOriginal();         // keep an up-to-date snapshot
}

//==========================================================================//
//  Parsing
//==========================================================================//

bool rvDeclEffect::Parse(const char* text, int textLength) {
    idLexer lexer;
    lexer.LoadMemory(text, textLength, base->GetFileName(), base->GetLineNum());
    lexer.SetFlags(LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWBACKSLASHSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS);

    // the first “{”
    if (!lexer.SkipUntilString("{"))
        return false;

    idToken tok;
    while (lexer.ReadToken(&tok)) {

        if (!tok.Icmp("}")) {
            break;                                      // end of decl
        }

        //--------------------------------------------------------------//
        //  Single-valued keywords (“size”)
        //--------------------------------------------------------------//
        if (!tok.Icmp("size")) {
            mSize = lexer.ParseFloat();
            continue;
        }

        //--------------------------------------------------------------//
        //  Segment creation
        //--------------------------------------------------------------//
        eSegType segType = TokenToSegType(tok);
        if (segType == eSegType::kInvalid) {
            common->Warning("^4BSE:^1 Invalid segment type '%s' (file: %s, line: %d)",
                tok.c_str(), lexer.GetFileName(), lexer.GetLineNum());
            lexer.SkipBracedSection();                  // skip junk
            continue;
        }

        rvSegmentTemplate& seg = mSegmentTemplates.Alloc();
        seg.Init(this);                               // default values
        seg.Parse(this, static_cast<int>(segType), &lexer);

        if (!seg.Finish(this)) {
            // invalid – roll back the push
            mSegmentTemplates.RemoveIndex(mSegmentTemplates.Num() - 1);
        }
    }

    Finish();                                           // second pass
    return true;
}
