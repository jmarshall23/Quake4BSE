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

In addition, the QUAKE 4 BSE CODE RECREATION EFFORT is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the QUAKE 4 BSE CODE RECREATION EFFORT.  

If you have questions concerning this license or the applicable additional terms, you may contact in writing justinmarshall20@gmail.com 

===========================================================================
*/

#include "bse.h"

/* --------------------------------------------------------------------- */
void rvSegmentTemplate::Init(rvDeclEffect* decl)
{
    mDeclEffect = decl;
    mFlags = STF_CONSTANT;          // default “locked” bit off
    mSegType = SEG_INVALID;

    mLocalStartTime.Zero();
    mLocalDuration.Zero();
    mAttenuation.Zero();

    mParticleCap = 0.0f;
    mDetail = 0.0f;
    mScale = 1.0f;

    mCount.Set(1.0f, 1.0f);
    mDensity.Zero();

    mTrailSegmentIndex = -1;
    mNumEffects = 0;
    memset(mEffects, 0, sizeof(mEffects));

    mSoundShader = nullptr;
    mSoundVolume.Zero();
    mFreqShift.Set(1.0f, 1.0f);

    mParticleTemplate.Init();
    mBSEEffect = nullptr;

    mSegmentName = "";
}

/* --------------------------------------------------------------------- */
void rvSegmentTemplate::CreateParticleTemplate(rvDeclEffect* effect,
    idLexer* lexer,
    int           particleType)
{
    mParticleTemplate.Init();
    mParticleTemplate.mType = particleType;
    mParticleTemplate.SetParameterCounts();
    mParticleTemplate.Parse(effect, lexer);
}

/* --------------------------------------------------------------------- */
int rvSegmentTemplate::GetTexelCount() const
{
    if (mParticleTemplate.mMaterial) {
        return mParticleTemplate.mMaterial->GetTexelCount();
    }
    return 0;
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::GetSmoker() const
{
    return mParticleTemplate.mTrailType == 3;     // ‘smoker’ trail
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::GetSoundLooping() const
{
    return mSoundShader && (mSoundShader->parms.soundShaderFlags & SSF_LOOPING);
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::DetailCull() const
{
    return (mDetail > 0.0f) &&
        (bse_scale.GetFloat() < mDetail);    // cvar comparison
}

float rvSegmentTemplate::CalculateBounds() 
{
    rvParticleTemplate* p_mParticleTemplate; // esi
    int mType; // eax
    double result; // st7
    float maxLength; // [esp+0h] [ebp-Ch]
    float maxDist; // [esp+4h] [ebp-8h]
    float maxSize; // [esp+8h] [ebp-4h]

    switch (this->mSegType)
    {
    case 2:
    case 3:
    case 7:
        p_mParticleTemplate = &this->mParticleTemplate;
        maxSize = mParticleTemplate.GetMaxParmValue(
            &this->mParticleTemplate.mSpawnSize,
            &this->mParticleTemplate.mDeathSize,
            &this->mParticleTemplate.mSizeEnvelope);
        maxDist = p_mParticleTemplate->GetFurthestDistance();
        mType = p_mParticleTemplate->mType;
        if (mType == 2 || mType == 7)
            maxLength = p_mParticleTemplate->GetMaxParmValue(
                &p_mParticleTemplate->mSpawnLength,
                &p_mParticleTemplate->mDeathLength,
                &p_mParticleTemplate->mLengthEnvelope);
        else
            maxLength = 0.0;
        result = p_mParticleTemplate->GetMaxParmValue(
            &p_mParticleTemplate->mSpawnOffset,
            &p_mParticleTemplate->mDeathOffset,
            &p_mParticleTemplate->mOffsetEnvelope)
            + maxLength
            + maxDist
            + maxSize;
        break;
    case 6:
        result = mParticleTemplate.GetMaxParmValue(
            &this->mParticleTemplate.mSpawnSize,
            &this->mParticleTemplate.mDeathSize,
            &this->mParticleTemplate.mSizeEnvelope);
        break;
    default:
        result = 8.0;
        break;
    }
    return result;
}

/* --------------------------------------------------------------------- */
void rvSegmentTemplate::SetMaxDuration(rvDeclEffect* effect)
{
    if (!(mFlags & STF_MAX_DURATION)) {
        effect->SetMaxDuration(mLocalStartTime.x + mLocalDuration.x);

        if (mParticleTemplate.mType != 0) {
            effect->SetMaxDuration(mLocalStartTime.x + mLocalDuration.x + mParticleTemplate.mDuration.y);
        }
    }
}

/* --------------------------------------------------------------------- */
void rvSegmentTemplate::SetMinDuration(rvDeclEffect* effect)
{
    if ((mFlags & STF_MAX_DURATION) != 0)
        return;

    if (!mSoundShader || !(mSoundShader->parms.soundShaderFlags & SSF_LOOPING)) {
        effect->SetMinDuration(mLocalStartTime.x + mLocalDuration.x);
    }
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::Compare(const rvSegmentTemplate& a) const
{
    /* cheap reject ------------------------------------------------------ */
    if (mSegmentName.Icmp(a.mSegmentName) != 0)
        return false;

    if (((mFlags ^ a.mFlags) & ~STF_LOCKED) != 0)
        return false;

    if (mSegType != a.mSegType)
        return false;

    /* timeline ---------------------------------------------------------- */
    if (mSegType != SEG_SOUND) {   // sound segs ignore timings
        if (mLocalStartTime != a.mLocalStartTime ||
            mLocalDuration != a.mLocalDuration)
            return false;
    }

    /* scale/detail/attenuation ----------------------------------------- */
    if (mScale != a.mScale ||
        mDetail != a.mDetail ||
        mAttenuation != a.mAttenuation)
        return false;

    /* count vs density variant ----------------------------------------- */
    if (mDensity.y == 0.0f) {
        if (mCount != a.mCount)
            return false;
    }
    else {
        if (mDensity != a.mDensity ||
            mParticleCap != a.mParticleCap)
            return false;
    }

    /* trail / effect list ---------------------------------------------- */
    if (mTrailSegmentIndex != a.mTrailSegmentIndex)
        return false;

    if (mNumEffects != a.mNumEffects)
        return false;

    for (int i = 0; i < mNumEffects; ++i) {
        if (mEffects[i] != a.mEffects[i])
            return false;
    }

    /* sound / freq ------------------------------------------------------ */
    if (mSoundShader != a.mSoundShader ||
        mSoundVolume != a.mSoundVolume ||
        mFreqShift != a.mFreqShift)
        return false;

    /* particle template deep compare ----------------------------------- */
    return mParticleTemplate.Compare(a.mParticleTemplate);
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::Finish(rvDeclEffect* effect)
{
    // 1) Ensure each “.x/.y” range has min ≤ max (manual swap, no std::swap)
    if (mLocalStartTime.x > mLocalStartTime.y) {
        float tmp = mLocalStartTime.x;
        mLocalStartTime.x = mLocalStartTime.y;
        mLocalStartTime.y = tmp;
    }
    if (mLocalDuration.x > mLocalDuration.y) {
        float tmp = mLocalDuration.x;
        mLocalDuration.x = mLocalDuration.y;
        mLocalDuration.y = tmp;
    }
    if (mCount.x > mCount.y) {
        float tmp = mCount.x;
        mCount.x = mCount.y;
        mCount.y = tmp;
    }
    if (mDensity.x > mDensity.y) {
        float tmp = mDensity.x;
        mDensity.x = mDensity.y;
        mDensity.y = tmp;
    }
    if (mAttenuation.x > mAttenuation.y) {
        float tmp = mAttenuation.x;
        mAttenuation.x = mAttenuation.y;
        mAttenuation.y = tmp;
    }

    // 2) Finish nested particle template, if any
    if (mParticleTemplate.mType) {
        mParticleTemplate.Finish();
        // clear the “intermediate-trail” bit (third byte)
        mParticleTemplate.mFlags &= ~(0x08 << 16);
    }

    // 3) Segment‐type‐specific setup
    switch (mSegType) {
    case 2: // single‐shot
        mFlags |= 0x04;
        if (!mParticleTemplate.mType) return false;
        if (mFlags & 0x20) return false;
        break;

    case 3: // conditional
        mFlags |= 0x04;
        if (!mParticleTemplate.mType) return false;
        break;

    case 4: // burst
        mFlags |= 0x04;
        // reset any local time/duration
        mLocalStartTime.x = mLocalStartTime.y = 0.0f;
        mLocalDuration.x = mLocalDuration.y = 0.0f;
        if (!mParticleTemplate.mType) return false;
        // set the “final-trail” bit
        mParticleTemplate.mFlags |= (0x08 << 16);
        break;

    case 5: // continuous
        mFlags |= 0x10;
        break;

    case 6: // delayed
        mFlags &= ~0x04;    // clear active
        mFlags |= 0x0100;  // set byte1 bit0
        break;

    case 9:  // attenuation-only
    case 10:
    case 11:
        if (mAttenuation.y > 0.0f) {
            mFlags |= 0x40;
        }
        // fall through to default
    default:
        mFlags &= ~0x04;
        break;
    }

    // 4) Common post-setup logic
    if (mParticleTemplate.mType == 9) {
        mFlags &= ~0x04;
        mFlags |= 0x0100;
    }

    if ((mFlags & 0x20) ||
        mParticleTemplate.mTrailType == 3 ||
        (mParticleTemplate.mFlags & 0x0200) ||
        mParticleTemplate.mNumTimeoutEffects) {
        mFlags |= 0x0200;  // set byte1 bit1
    }

    if (mParticleTemplate.mType == 6 || mParticleTemplate.mType == 7) {
        mFlags |= 0x0200;
    }

    return true;
}

/* --------------------------------------------------------------------- */
void rvSegmentTemplate::EvaluateTrailSegment(rvDeclEffect* et)
{
    if (mParticleTemplate.mTrailTypeName.Icmp(entityFilter) != 0 &&
        mParticleTemplate.mTrailType != 0)
    {
        mTrailSegmentIndex = et->GetTrailSegmentIndex(mParticleTemplate.mTrailTypeName);
    }
}

/* --------------------------------------------------------------------- */
bool rvSegmentTemplate::Parse(rvDeclEffect* effect,
    int            segmentType,
    idLexer* lexer)
{
    idToken token;
    mSegType = segmentType;

    /* -------- optional explicit segment name -------------------------- */
    if (!lexer->ReadToken(&token))
        return false;

    if (token.Icmp("{") != 0) {
        mSegmentName = token;
    }
    else {
        mSegmentName = va("unnamed%d", effect->mSegmentTemplates.Num());
        lexer->UnreadToken(&token);                // put the “{” back
    }

    /* -------- open brace ------------------------------------------------ */
    if (!lexer->ExpectTokenString("{"))
        return false;

    /* -------- parameter loop ------------------------------------------- */
    while (lexer->ReadToken(&token))
    {
        if (token == "}")
            break;

        /* --- numeric pairs -------------------------------------------- */
#       define READ_VEC2( dst )                 \
            do {                                \
                (dst).x = lexer->ParseFloat();  \
                lexer->ExpectTokenString( "," );\
                (dst).y = lexer->ParseFloat();  \
            } while(0)

        /* --- dispatch table ------------------------------------------- */
        if (token.Icmp("count") == 0 ||
            token.Icmp("rate") == 0) {
            READ_VEC2(mCount);
        }
        else if (token.Icmp("density") == 0) { READ_VEC2(mDensity); }
        else if (token.Icmp("particleCap") == 0) { mParticleCap = lexer->ParseFloat(); }
        else if (token.Icmp("start") == 0) { READ_VEC2(mLocalStartTime); }
        else if (token.Icmp("duration") == 0) { READ_VEC2(mLocalDuration); }
        else if (token.Icmp("detail") == 0) { mDetail = lexer->ParseFloat(); }
        else if (token.Icmp("scale") == 0) { mScale = lexer->ParseFloat(); }
        else if (token.Icmp("attenuation") == 0) { READ_VEC2(mAttenuation); }
        else if (token.Icmp("attenuateEmitter") == 0) { mFlags |= STF_EMITTER_ATTEN; }
        else if (token.Icmp("inverseAttenuateEmitter") == 0) { mFlags |= STF_EMITTER_INV_ATTEN; }
        else if (token.Icmp("locked") == 0) { mFlags |= STF_LOCKED; }
        else if (token.Icmp("constant") == 0) { mFlags |= STF_CONSTANT; }
        /* --- sound ---------------------------------------------------- */
        else if (token.Icmp("soundShader") == 0)
        {
            lexer->ReadToken(&token);
            mSoundShader = declManager->FindSound(token, true);
            const float len = mSoundShader->GetTimeLength();
            mLocalDuration.x = mLocalDuration.y = len;
            mFlags |= STF_HAS_SOUND;
        }
        else if (token.Icmp("volume") == 0) { READ_VEC2(mSoundVolume); }
        else if (token.Icmp("freqShift") == 0) { READ_VEC2(mFreqShift); }
        /* --- nested effect refs --------------------------------------- */
        else if (token.Icmp("effect") == 0)
        {
            lexer->ReadToken(&token);
            if (mNumEffects >= 4) {
                common->Warning("^4BSE:^1 Too many sub-effects in segment '%s'",
                    mSegmentName.c_str());
            }
            else {
                mEffects[mNumEffects++] = declManager->FindEffect(token, true);
            }
        }
        /* --- particle primitive keywords ------------------------------ */
        else if (token.Icmp("sprite") == 0) CreateParticleTemplate(effect, lexer, 1);
        else if (token.Icmp("line") == 0) CreateParticleTemplate(effect, lexer, 2);
        else if (token.Icmp("oriented") == 0) CreateParticleTemplate(effect, lexer, 3);
        else if (token.Icmp("decal") == 0) CreateParticleTemplate(effect, lexer, 4);
        else if (token.Icmp("model") == 0) CreateParticleTemplate(effect, lexer, 5);
        else if (token.Icmp("light") == 0) CreateParticleTemplate(effect, lexer, 6);
        else if (token.Icmp("electricity") == 0) CreateParticleTemplate(effect, lexer, 7);
        else if (token.Icmp("linked") == 0) CreateParticleTemplate(effect, lexer, 8);
        else if (token.Icmp("debris") == 0) CreateParticleTemplate(effect, lexer, 9);
        /* --- channel-only keyword ------------------------------------- */
        else if (token.Icmp("channel") == 0) { /* nothing – marker */ }
        /* --- unknown token -------------------------------------------- */
        else
        {
            common->Warning("^4BSE:^1 Invalid segment parameter '%s' (file: %s, line: %d)",
                token.c_str(), lexer->GetFileName(), lexer->GetLineNum());
        }
#       undef READ_VEC2
    }

    return true;
}