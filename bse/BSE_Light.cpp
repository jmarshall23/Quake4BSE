// BSP_Light.cpp
//

#pragma hdrstop
#include "precompiled.h"

#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_Particle.h"

/*
=====================
rvLightParticle::Destroy
=====================
*/
bool rvLightParticle::Destroy(void)
{
	if (mLightDefHandle != -1)
	{		
		common->RW()->FreeLightDef(mLightDefHandle);
		mLightDefHandle = -1;
	}
	return 1;
}

/*
=====================
rvSegment::InitLight
=====================
*/
void rvSegment::InitLight(rvBSE* effect, rvSegmentTemplate* st, float time)
{
	if (!mUsedHead)
	{
		SpawnParticle(effect, st, time, vec3_origin, mat3_identity);
		mUsedHead->InitLight(effect, st, time);		
	}
}

/*
=====================
rvLightParticle::InitLight
=====================
*/
bool rvLightParticle::InitLight(rvBSE* effect, rvSegmentTemplate* st, float time)
{
	bool result; // al
	rvEnvParms* v8; // esi
	rvEnvParms* v9; // ecx
	rvParticleTemplate* v10; // ebp
	float* v11; // esi
	double v12; // st7
	const idMaterial* v13; // ecx
	float evalTime; // [esp+24h] [ebp-48h] BYREF
	float v16; // [esp+28h] [ebp-44h]
	idVec3 v17; // [esp+30h] [ebp-3Ch]
	idVec3 v18; // [esp+3Ch] [ebp-30h]
	idVec3 pos; // [esp+48h] [ebp-24h] BYREF
	idVec3 v20; // [esp+54h] [ebp-18h] BYREF
	idVec3 v21; // [esp+60h] [ebp-Ch] BYREF
	float effectb; // [esp+74h] [ebp+8h]
	float effecta; // [esp+74h] [ebp+8h]
	float a6a; // [esp+7Ch] [ebp+10h]

	result = GetEvaluationTime(time, evalTime, 0);
	if (result)
	{
		memset(&this->mLight, 0, sizeof(this->mLight));
		v8 = st->GetParticleTemplate()->mpFadeEnvelope;
		effectb = this->mEndTime - this->mStartTime;
		v9 = st->GetParticleTemplate()->mpTintEnvelope;
		v10 = st->GetParticleTemplate();
		effecta = 1.0 / effectb;
		v16 = evalTime;
		v9->Evaluate(this->mTintEnv, evalTime, *(float*)&effecta, v20.ToFloatPtr());
		v8->Evaluate(this->mFadeEnv, v16, *(float*)&effecta, v21.ToFloatPtr());

		EvaluateSize(v10->mpSizeEnvelope, evalTime, effecta, pos.ToFloatPtr());
		EvaluatePosition(effect, v10, pos, time);

		// 0 1 2 3
		// 4 5 6 7
		// 8 9 10 11
		// 12 13 14 15

// jmarshall - eval
		v17.x = effect->GetCurrentAxis()[2].x * pos.z
			+ effect->GetCurrentAxis()[0].x * pos.x
			+ effect->GetCurrentAxis()[1].x * pos.y;
		v17.y = effect->GetCurrentAxis()[1].y * pos.y
			+ effect->GetCurrentAxis()[0].y * pos.x
			+ effect->GetCurrentAxis()[2].y * pos.z;
		v17.z = pos.x * effect->GetCurrentAxis()[0].z
			+ pos.y * effect->GetCurrentAxis()[1].z
			+ pos.z * effect->GetCurrentAxis()[2].z;
		v18.x = v17.x + effect->GetCurrentOrigin().x;
		v18.y = effect->GetCurrentOrigin().y + v17.y;
		v18.z = effect->GetCurrentOrigin().z + v17.z;
		this->mLight.origin = v18;
// jmarshall end

		this->mLight.origin = v18;
		this->mLight.lightRadius = v20;
		if (this->mLight.lightRadius.x < 1.0)
			this->mLight.lightRadius.x = 1.0;
		if (this->mLight.lightRadius.y < 1.0)
			this->mLight.lightRadius.y = 1.0;
		if (this->mLight.lightRadius.z < 1.0)
			this->mLight.lightRadius.z = 1.0;
		v12 = v21.x;
// jmarshall
		//qmemcpy(&this->mLight, &effecta->mCurrentAxis, 0x24u);
// jmarshall end
		this->mLight.shaderParms[0] = v12;
		this->mLight.shaderParms[1] = v21.y;
		this->mLight.shaderParms[2] = v21.z;
		//this->mLight.maxVisDist = 4096;
		v13 = v10->mMaterial;
		//*(_BYTE*)&this->mLight.flags |= 4u;
		this->mLight.shader = v10->GetMaterial();
		//v14 = (renderLight_t::renderLightFlags_t)(*(_BYTE*)&this->mLight.flags ^ (*(_BYTE*)&this->mLight.flags ^ ~(unsigned __int8)((unsigned int)v10->mFlags >> 17)) & 1);
		//this->mLight.flags = v14;
		//this->mLight.flags = (renderLight_t::renderLightFlags_t)(*(_BYTE*)&v14 ^ (*(_BYTE*)&v14 ^ ~(2
		//	* ((unsigned int)v10->mFlags >> 18))) & 2);
		//this->mLight.manualPriority = 1;
		//this->mLight.lightId = (int)&effecta->mCurrentAxis;
		this->mLightDefHandle = common->RW()->AddLightDef(&mLight);
		result = 1;
	}
	return result;
}

bool rvLightParticle::PresentLight(rvBSE* effect, rvParticleTemplate* pt, float time, bool infinite)
{
	int v6; // edi
	int v7; // esi MAPDST
	bool result; // al
	rvEnvParms* v11; // ecx
	rvEnvParms* v12; // edi
	double v14; // st7
	int v15; // [esp+14h] [ebp-54h]
	float oneOverDuration[3]; // [esp+24h] [ebp-44h] BYREF
	idVec3 v18; // [esp+30h] [ebp-38h]
	idVec3 position; // [esp+3Ch] [ebp-2Ch]
	idVec3 size; // [esp+48h] [ebp-20h] BYREF
	idVec4 tint; // [esp+54h] [ebp-14h] BYREF
	float dest; // [esp+64h] [ebp-4h] BYREF
	float retaddr; // [esp+68h] [ebp+0h]
	float pta; // [esp+70h] [ebp+8h]
	float ooduration; // [esp+78h] [ebp+10h]
	float oodurationa; // [esp+78h] [ebp+10h]

	result = GetEvaluationTime(time, oneOverDuration[0], infinite);
	if (result)
	{
		v11 = pt->mpTintEnvelope;
		ooduration = this->mEndTime - this->mStartTime;
		v15 = v6;
		v12 = pt->mpFadeEnvelope;
		oodurationa = 1.0 / ooduration;
		pta = oneOverDuration[0];
		v11->Evaluate(this->mTintEnv, oodurationa, oneOverDuration[0], tint.ToFloatPtr());
		v12->Evaluate(this->mFadeEnv, oodurationa, pta, &dest);
		EvaluateSize(pt->mpSizeEnvelope, oneOverDuration[0], oodurationa, size.ToFloatPtr());
	
		EvaluatePosition(effect, pt, size, time);
		v18.x = effect->GetCurrentAxis()[2].x * size.z
			+ effect->GetCurrentAxis()[0].x * size.x
			+ effect->GetCurrentAxis()[1].x * size.y;
		v18.y = effect->GetCurrentAxis()[1].y * size.y
			+ effect->GetCurrentAxis()[0].y * size.x
			+ effect->GetCurrentAxis()[2].y * size.z;
		v18.z = size.x * effect->GetCurrentAxis()[0].z
			+ size.y * effect->GetCurrentAxis()[1].z
			+ size.z * effect->GetCurrentAxis()[2].z;
		position.x = v18.x + effect->GetCurrentOrigin().x;
		position.y = effect->GetCurrentOrigin().y + v18.y;
		position.z = effect->GetCurrentOrigin().z + v18.z;
		this->mLight.origin = position;
		this->mLight.lightRadius.x = tint.x;
		this->mLight.lightRadius.y = tint.y;
		this->mLight.lightRadius.z = tint.z;
		if (this->mLight.lightRadius.x < 1.0)
			this->mLight.lightRadius.x = 1.0;
		if (this->mLight.lightRadius.y < 1.0)
			this->mLight.lightRadius.y = 1.0;
		if (this->mLight.lightRadius.z < 1.0)
			this->mLight.lightRadius.z = 1.0;
		v14 = tint.w;
		//qmemcpy(&this->mLight, &time->mCurrentAxis, 0x24u);
		this->mLight.shaderParms[0] = v14;
		this->mLight.shaderParms[1] = dest;
		this->mLight.shaderParms[2] = retaddr;
		this->mLight.suppressLightInViewID = effect->GetSuppressLightsInViewID();		
		common->RW()->UpdateLightDef(mLightDefHandle, &mLight);
		result = 1;
	}
	return result;
}