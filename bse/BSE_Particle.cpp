// BSE_Particle.cpp
//

#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"

bool rvParticle::GetEvaluationTime(float time, float& evalTime, bool infinite) {
	evalTime = time - this->mStartTime;
	if (this->mEndTime - 0.002 <= time) {
		evalTime = this->mEndTime - this->mStartTime - 0.002;
	}

	if (infinite) {
		return true;
	}

	return (this->mStartTime - 0.002 < time && this->mEndTime > time);
}

void rvParticle::EvaluatePosition(const rvBSE* effect, rvParticleTemplate* pt, idVec3& pos, float time) {
	float deltaTime = time - this->mFriction * 0.5f * time * time; // Adjust time by friction

	if (this->mFlags & 1) {
		pos = this->mInitPos; // Use initial position directly if specified
	}
	else {		
		idVec3 velocityDelta = this->mVelocity * deltaTime;
		pos = this->mInitPos + velocityDelta; // Basic motion equation

		if (this->mFlags & 4) {
			float halfTimeSquared = 0.5f * deltaTime * deltaTime;
			idVec3 accelerationDelta = this->mAcceleration * halfTimeSquared;
			pos += accelerationDelta; // Apply acceleration
		}
	}

	if (!(this->mFlags & 2)) {
		// Transform position from local to world space based on effect's current origin and axis
		pos = effect->mCurrentAxis * pos + effect->mCurrentOrigin;

		float evalTime = deltaTime; // Assuming deltaTime is the correct adjusted time
		float oneOverDuration = 1.0f / (this->mEndTime - this->mStartTime);
		idVec3 offset, angle;
		pt->mpOffsetEnvelope->Evaluate(mOffsetEnv, evalTime, oneOverDuration, &offset.x);
		pt->mpAngleEnvelope->Evaluate(mAngleEnv, evalTime, oneOverDuration, &angle.x);
		idMat3 rotationMat = idAngles(angle.x, angle.y, angle.z).ToMat3();
		pos = rotationMat * (pos + offset);
	}


	// Update internal position state for the particle
	this->mPosition = pos;
}

void rvParticle::Attenuate(float atten, rvParticleParms& parms, rvEnvParms1& result) {
	// Check if attenuation or inverse attenuation flags are set
	if (parms.mFlags & 0x60) { // Assuming ATTEN_FLAG = 0x20, INVERSE_ATTEN_FLAG = 0x40
		// Apply inverse attenuation if the flag is set
		if (parms.mFlags & 0x40) {
			atten = 1.0f - atten;
		}

		// Apply attenuation to the start and end values of the result
		result.mStart *= atten;
		result.mEnd *= atten;
	}
}

void rvParticle::Attenuate(float atten, rvParticleParms& parms, rvEnvParms1Particle& result) {
	if (parms.mFlags & 0x60) { // Check if attenuation flags are set
		if (parms.mFlags & 0x40) { // Inverse attenuation
			atten = 1.0f - atten;
		}
		result.mStart *= atten;
		result.mEnd *= atten;
	}
}

void rvParticle::Attenuate(float atten, rvParticleParms& parms, rvEnvParms2Particle& result) {
	if (parms.mFlags & 0x60) { // Check if attenuation flags are set
		if (parms.mFlags & 0x40) { // Inverse attenuation
			atten = 1.0f - atten;
		}
		result.mStart.x *= atten;
		result.mStart.y *= atten;
		result.mEnd.x *= atten;
		result.mEnd.y *= atten;
	}
}

void rvParticle::Attenuate(float atten, rvParticleParms& parms, rvEnvParms3Particle& result) {
	if (parms.mFlags & 0x60) { // Check if attenuation flags are set
		if (parms.mFlags & 0x40) { // Inverse attenuation
			atten = 1.0f - atten;
		}
		result.mStart.x *= atten;
		result.mStart.y *= atten;
		result.mStart.z *= atten;
		result.mEnd.x *= atten;
		result.mEnd.y *= atten;
		result.mEnd.z *= atten;
	}
}

void rvParticle::FinishSpawn(rvBSE* effect, rvSegment* segment, float birthTime, float fraction, const idVec3& initOffset, const idMat3& initAxis)
{
	/*
	void __thiscall rvParticle::FinishSpawn(rvParticle *this, rvBSE *effect, rvSegment *segment, float birthTime, float fraction, idVec3 *initOffset, idMat3 *initAxis)
{
  rvParticle *v7; // ebp
  rvSegmentTemplate *v8; // eax
  rvSegmentTemplate *v9; // esi
  int v10; // ebx
  unsigned __int8 *v11; // eax
  int v12; // edx
  float *v13; // edi
  void (__cdecl *v14)(VBRState *); // eax
  rvBSE *v15; // esi
  double v16; // st7
  float v17; // ecx
  float v18; // edx
  float v19; // ecx
  idVec3 *v20; // eax
  unsigned int v21; // ecx
  char v22; // dl
  void (__cdecl *v23)(VBRState *); // ecx
  double v24; // st7
  rvAngles *v25; // eax
  float v26; // ecx
  float *v27; // eax
  double v28; // st7
  double v29; // st6
  double v30; // st7
  float *v31; // eax
  float *v32; // ecx
  double v33; // st7
  idVec3 *v34; // eax
  unsigned __int8 *v35; // eax
  int v36; // edx
  rvParticleVtbl *v37; // edx
  VBRState *v38; // eax
  void (__cdecl *v39)(VBRState *); // edx
  rvParticleVtbl *v40; // edx
  VBRState *v41; // eax
  void (__cdecl *v42)(VBRState *); // edx
  void (__cdecl *v43)(VBRState *); // eax
  void (__cdecl *v44)(VBRState *); // eax
  unsigned __int8 *v45; // eax
  int v46; // edx
  float *(__thiscall *v47)(rvParticle *); // eax
  VBRState *v48; // eax
  void (__cdecl *v49)(VBRState *); // edx
  rvParticleVtbl *v50; // edx
  VBRState *v51; // eax
  void (__cdecl *v52)(VBRState *); // edx
  rvParticleVtbl *v53; // edx
  float v54; // eax
  rvParticleVtbl *v55; // edx
  float *v56; // eax
  float *(__thiscall *v57)(rvParticle *); // eax
  float v58; // eax
  rvParticleVtbl *v59; // edx
  float *v60; // eax
  int v61; // eax
  int v62; // eax
  void (__thiscall *v63)(rvParticle *, idRenderModel *); // edx
  double v64; // st7
  double v65; // st7
  int v66; // ecx
  float v67; // ecx
  double v68; // st7
  void (__thiscall *v69)(rvParticle *, rvEnvParms *, float); // edx
  float v70; // ecx
  void (__thiscall *v71)(rvParticle *, rvEnvParms *, float); // edx
  float v72; // ecx
  float v73; // ecx
  double v74; // st6
  double v75; // st5
  float *v76; // eax
  float v77; // [esp+30h] [ebp-B4h]
  float atten; // [esp+34h] [ebp-B0h]
  float min; // [esp+38h] [ebp-ACh]
  float halfOpeningAngle; // [esp+3Ch] [ebp-A8h]
  int v81; // [esp+40h] [ebp-A4h]
  float f; // [esp+50h] [ebp-94h]
  idVec3 normal; // [esp+54h] [ebp-90h]
  float v84; // [esp+60h] [ebp-84h]
  float v85; // [esp+64h] [ebp-80h]
  float v86; // [esp+68h] [ebp-7Ch]
  float v87; // [esp+6Ch] [ebp-78h]
  float v88; // [esp+70h] [ebp-74h]
  sdRenderProgram *v89; // [esp+74h] [ebp-70h]
  float t1; // [esp+78h] [ebp-6Ch]
  float v91; // [esp+7Ch] [ebp-68h] OVERLAPPED
  char v92; // [esp+82h] [ebp-62h]
  char v93; // [esp+83h] [ebp-61h]
  float *v94; // [esp+84h] [ebp-60h]
  float v95; // [esp+88h] [ebp-5Ch]
  float v96; // [esp+8Ch] [ebp-58h]
  float v97; // [esp+90h] [ebp-54h]
  float windStrength; // [esp+94h] [ebp-50h]
  rvAngles angles; // [esp+98h] [ebp-4Ch]
  float v100; // [esp+A4h] [ebp-40h]
  idVec3 result; // [esp+A8h] [ebp-3Ch]
  char v102; // [esp+B4h] [ebp-30h]
  idMat3 v103; // [esp+C0h] [ebp-24h]

  v7 = this;
  *(float *)&v8 = COERCE_FLOAT(rvSegment::GetSegmentTemplate(segment));
  v9 = v8;
  v91 = *(float *)&v8;
  if ( *(float *)&v8 == 0.0 )
	return;
  v10 = (int)&v8->mParticleTemplate;
  v7->mFlags = v8->mParticleTemplate.mFlags;
  if ( (unsigned __int8)rvSegment::GetLocked(segment) )
	v7->mFlags |= 2u;
  else
	v7->mFlags &= 0xFFFFFFFD;
  if ( ((unsigned int)v9->mFlags >> 9) & 1 )
	v7->mFlags |= (unsigned int)&unk_1000000;
  else
	v7->mFlags &= 0xFEFFFFFF;
  v11 = (unsigned __int8 *)v9->mParticleTemplate.mpSpawnVelocity;
  v12 = *v11;
  halfOpeningAngle = 0.0;
  min = 0.0;
  atten = *(float *)&v11;
  v13 = &v7->mVelocity.x;
  ((void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[v12])((VBRState *)&v7->mVelocity);
  v14 = (void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[v9->mParticleTemplate.mpSpawnAcceleration->mSpawnType];
  v94 = &v7->mAcceleration.x;
  v14((VBRState *)&v7->mAcceleration);
  ((void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[v9->mParticleTemplate.mpSpawnWindStrength->mSpawnType])((VBRState *)&angles);
  v15 = effect;
  v16 = 0.0;
  if ( angles.pitch != 0.0 )
  {
	*(float *)&v89 = effect->mCurrentWindVector.y * effect->mCurrentWindVector.y
				   + effect->mCurrentWindVector.x * effect->mCurrentWindVector.x
				   + effect->mCurrentWindVector.z * effect->mCurrentWindVector.z;
	if ( *(float *)&v89 > 0.0001 )
	{
	  v17 = effect->mCurrentWindVector.x;
	  v18 = effect->mCurrentWindVector.y;
	  v89 = *(sdRenderProgram **)(v10 + 92);
	  t1 = angles.pitch;
	  halfOpeningAngle = *(float *)&v89;
	  v77 = v17;
	  v19 = effect->mCurrentWindVector.z;
	  atten = v18;
	  min = v19;
	  v20 = idRandom::RandomVectorInCone(&idRandom::staticRandom, &result, *(idVec3 *)&v77, *(float *)&v89);
	  v86 = t1 * v20->x;
	  v87 = v20->y * t1;
	  v88 = t1 * v20->z;
	  *v13 = v86 + *v13;
	  v7->mVelocity.y = v87 + v7->mVelocity.y;
	  v7->mVelocity.z = v7->mVelocity.z + v88;
	  v16 = 0.0;
	}
  }
  if ( (*(_DWORD *)v10 >> 25) & 1 )
  {
	f = effect->mCurrentAxis.mat[2].z;
	v97 = effect->mCurrentAxis.mat[1].z;
	normal.x = effect->mCurrentAxis.mat[0].z;
	v85 = effect->mCurrentAxis.mat[2].y;
	v95 = effect->mCurrentAxis.mat[1].y;
	t1 = effect->mCurrentAxis.mat[0].y;
	v96 = effect->mCurrentAxis.mat[2].x;
	windStrength = effect->mCurrentAxis.mat[1].x;
	v89 = (sdRenderProgram *)LODWORD(effect->mCurrentAxis.mat[0].x);
	v86 = *(float *)&v89 * effect->mCurrentVelocity.x
		+ t1 * effect->mCurrentVelocity.y
		+ normal.x * effect->mCurrentVelocity.z;
	v87 = windStrength * effect->mCurrentVelocity.x
		+ v95 * effect->mCurrentVelocity.y
		+ v97 * effect->mCurrentVelocity.z;
	v88 = v96 * effect->mCurrentVelocity.x + v85 * effect->mCurrentVelocity.y + f * effect->mCurrentVelocity.z;
	*v13 = v86 + *v13;
	v7->mVelocity.y = v87 + v7->mVelocity.y;
	v7->mVelocity.z = v7->mVelocity.z + v88;
  }
  v7->mFraction = fraction;
  v7->mTextureScale = 1.0;
  v7->mTextureOffset = v16;
  if ( *(_BYTE *)(v10 + 103) )
  {
	LODWORD(f) = *(unsigned __int8 *)(v10 + 103);
	normal.x = 1.0 / (double)SLODWORD(f);
	v7->mTextureScale = normal.x;
	f = COERCE_FLOAT(rvRandom::irand(0, *(unsigned __int8 *)(v10 + 103) - 1));
	v7->mTextureOffset = (double)SLODWORD(f) * normal.x;
	v16 = 0.0;
  }
  v21 = *(_DWORD *)v10;
  v22 = (*(_DWORD *)v10 >> 11) & 1;
  v92 = (*(_DWORD *)v10 >> 12) & 1;
  v93 = v22;
  if ( v92 )
  {
	halfOpeningAngle = 0.0;
	min = COERCE_FLOAT((idVec3 *)((char *)&normal + 4));
  }
  else if ( v22 )
  {
	LODWORD(halfOpeningAngle) = v10 + 68;
	min = COERCE_FLOAT((idVec3 *)((char *)&normal + 4));
  }
  else
  {
	halfOpeningAngle = 0.0;
	min = 0.0;
	if ( (v21 >> 14) & 1 )
	{
	  v23 = (void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[**(unsigned __int8 **)(v10 + 116)];
	  atten = *(float *)(v10 + 116);
	  v23((VBRState *)&normal.y);
	  v24 = effect->mCurrentAxisTransposed.mat[1].x;
	  halfOpeningAngle = 0.0;
	  min = 0.0;
	  f = effect->mCurrentAxisTransposed.mat[2].x * v84
		+ v24 * normal.z
		+ normal.y * effect->mCurrentAxisTransposed.mat[0].x;
	  v85 = effect->mCurrentAxisTransposed.mat[1].y * normal.z
		  + effect->mCurrentAxisTransposed.mat[0].y * normal.y
		  + effect->mCurrentAxisTransposed.mat[2].y * v84;
	  v84 = v84 * effect->mCurrentAxisTransposed.mat[2].z
		  + normal.z * effect->mCurrentAxisTransposed.mat[1].z
		  + normal.y * effect->mCurrentAxisTransposed.mat[0].z;
	  normal.y = f;
	  normal.z = v85;
	}
	else
	{
	  normal.y = 1.0;
	  normal.z = v16;
	  v84 = v16;
	}
  }
  rvParticle::HandleEndOrigin(
	v7,
	effect,
	(rvParticleTemplate *)v10,
	(idVec3 *)LODWORD(min),
	(idVec3 *)LODWORD(halfOpeningAngle));
  if ( (*(_DWORD *)v10 >> 19) & 1 )
  {
	f = initAxis->mat[1].x * v7->mVelocity.y
	  + v7->mVelocity.x * initAxis->mat[0].x
	  + initAxis->mat[2].x * v7->mVelocity.z;
	v85 = initAxis->mat[0].y * v7->mVelocity.x
		+ initAxis->mat[1].y * v7->mVelocity.y
		+ initAxis->mat[2].y * v7->mVelocity.z;
	v7->mVelocity.z = initAxis->mat[0].z * v7->mVelocity.x
					+ initAxis->mat[1].z * v7->mVelocity.y
					+ initAxis->mat[2].z * v7->mVelocity.z;
	*v13 = f;
	v7->mVelocity.y = v85;
  }
  v25 = idVec3::ToRadians((idVec3 *)((char *)&normal + 4), (rvAngles *)&result);
  angles.yaw = v25->pitch;
  angles.roll = v25->yaw;
  v100 = v25->roll;
  if ( v93 || v92 || (LODWORD(v26) = *(_DWORD *)v10 >> 14, LOBYTE(v26) & 1) )
  {
	f = v84 * v84 + normal.z * normal.z + normal.y * normal.y;
	f = sqrt(f);
	if ( f >= 0.00000011920929 )
	{
	  f = 1.0 / f;
	  normal.y = f * normal.y;
	  normal.z = f * normal.z;
	  v84 = f * v84;
	}
	v27 = (float *)idVec3::ToMat3((idVec3 *)((char *)&normal + 4), &v103);
	f = v27[3] * v7->mVelocity.y + v7->mVelocity.x * *v27 + v27[6] * v7->mVelocity.z;
	v85 = v27[1] * v7->mVelocity.x + v27[4] * v7->mVelocity.y + v27[7] * v7->mVelocity.z;
	v28 = v27[2] * v7->mVelocity.x + v27[5] * v7->mVelocity.y;
	v29 = v27[8] * v7->mVelocity.z;
	atten = normal.y;
	min = normal.z;
	halfOpeningAngle = v84;
	v7->mVelocity.z = v28 + v29;
	*v13 = f;
	v7->mVelocity.y = v85;
	((void (__thiscall *)(rvParticle *, _DWORD, _DWORD, _DWORD))v7->vfptr->TransformLength)(
	  v7,
	  LODWORD(atten),
	  LODWORD(min),
	  LODWORD(halfOpeningAngle));
  }
  if ( (*(_DWORD *)v10 >> 13) & 1 )
  {
	v30 = *v13;
	halfOpeningAngle = v26;
	*v13 = v30 * -1.0;
	v7->mVelocity.y = v7->mVelocity.y * -1.0;
	v7->mVelocity.z = -1.0 * v7->mVelocity.z;
	((void (__thiscall *)(rvParticle *, _DWORD))v7->vfptr->ScaleLength)(v7, -1.0);
  }
  f = v7->mVelocity.y * v7->mVelocity.y + *v13 * *v13 + v7->mVelocity.z * v7->mVelocity.z;
  if ( f != 0.0 )
  {
	normal.y = *v13;
	normal.z = v7->mVelocity.y;
	v84 = v7->mVelocity.z;
	f = v84 * v84 + normal.z * normal.z + normal.y * normal.y;
	f = sqrt(f);
	if ( f >= 0.00000011920929 )
	{
	  f = 1.0 / f;
	  normal.y = f * normal.y;
	  normal.z = f * normal.z;
	  v84 = f * v84;
	}
  }
  v31 = (float *)idVec3::ToMat3((idVec3 *)((char *)&normal + 4), &v103);
  v32 = v94;
  f = v31[3] * v94[1] + *v94 * *v31 + v31[6] * v94[2];
  v85 = v31[4] * v94[1] + v31[1] * *v94 + v31[7] * v94[2];
  v94[2] = v31[5] * v94[1] + v31[2] * *v94 + v31[8] * v94[2];
  *v32 = f;
  v32[1] = v85;
  f = v32[1] * v32[1] + *v32 * *v32 + v32[2] * v32[2];
  v33 = 0.0;
  if ( 0.0 != f )
  {
	normal.y = *v32;
	normal.z = v32[1];
	v84 = v32[2];
	f = v84 * v84 + normal.z * normal.z + normal.y * normal.y;
	f = sqrt(f);
	if ( f >= 0.00000011920929 )
	{
	  f = 1.0 / f;
	  normal.y = f * normal.y;
	  normal.z = f * normal.z;
	  v84 = f * v84;
	}
	v33 = 0.0;
  }
  if ( v7->mFlags & 2 )
  {
	v7->mInitAxis.mat[0].x = 1.0;
	v7->mInitAxis.mat[1].y = 1.0;
	v7->mInitAxis.mat[2].z = 1.0;
	v7->mInitAxis.mat[0].y = v33;
	v7->mInitAxis.mat[0].z = v33;
	v7->mInitAxis.mat[1].x = v33;
	v7->mInitAxis.mat[1].z = v33;
	v7->mInitAxis.mat[2].x = v33;
	v7->mInitAxis.mat[2].y = v33;
	v7->mInitEffectPos = vec3_origin;
  }
  else
  {
	qmemcpy(&v7->mInitAxis, &effect->mCurrentAxis, sizeof(v7->mInitAxis));
	halfOpeningAngle = *(float *)&effect;
	v7->mInitEffectPos.x = effect->mCurrentOrigin.x;
	v7->mInitEffectPos.y = effect->mCurrentOrigin.y;
	v7->mInitEffectPos.z = effect->mCurrentOrigin.z;
	normal.x = v7->mInitAxis.mat[2].z;
	windStrength = v7->mInitAxis.mat[1].z;
	v96 = v7->mInitAxis.mat[0].z;
	v89 = (sdRenderProgram *)LODWORD(v7->mInitAxis.mat[2].y);
	v97 = v7->mInitAxis.mat[1].y;
	f = v7->mInitAxis.mat[0].y;
	t1 = v7->mInitAxis.mat[2].x;
	v95 = v7->mInitAxis.mat[1].x;
	v85 = v7->mInitAxis.mat[0].x;
	v34 = rvBSE::GetInterpolatedOffset(effect, &result, birthTime);
	v15 = effect;
	v86 = f * v34->y + v85 * v34->x + v96 * v34->z;
	v87 = v97 * v34->y + v95 * v34->x + windStrength * v34->z;
	v88 = *(float *)&v89 * v34->y + t1 * v34->x + normal.x * v34->z;
	v7->mInitPos.x = v7->mInitPos.x - v86;
	v7->mInitPos.y = v7->mInitPos.y - v87;
	v7->mInitPos.z = v7->mInitPos.z - v88;
  }
  if ( (*(_DWORD *)v10 >> 19) & 1 )
  {
	v7->mInitPos.x = initOffset->x + v7->mInitPos.x;
	v7->mInitPos.y = initOffset->y + v7->mInitPos.y;
	v7->mInitPos.z = initOffset->z + v7->mInitPos.z;
  }
  v35 = *(unsigned __int8 **)(v10 + 132);
  v36 = *v35;
  halfOpeningAngle = 0.0;
  min = 0.0;
  atten = *(float *)&v35;
  ((void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[v36])((VBRState *)&v7->mTintEnv);
  ((void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[**(unsigned __int8 **)(v10 + 136)])((VBRState *)&v7->mFadeEnv);
  v37 = v7->vfptr;
  f = *(float *)(v10 + 140);
  v38 = (VBRState *)v37->GetInitSize(v7);
  v39 = (void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[**(unsigned __int8 **)(v10 + 140)];
  halfOpeningAngle = 0.0;
  min = 0.0;
  atten = f;
  v39(v38);
  v40 = v7->vfptr;
  f = *(float *)(v10 + 144);
  v41 = (VBRState *)v40->GetInitRotation(v7);
  v42 = (void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[**(unsigned __int8 **)(v10 + 144)];
  halfOpeningAngle = 0.0;
  min = 0.0;
  atten = f;
  v42(v41);
  v43 = (void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[**(unsigned __int8 **)(v10 + 148)];
  LODWORD(t1) = (char *)v7 + 168;
  v43((VBRState *)&v7->mAngleEnv);
  v44 = (void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[**(unsigned __int8 **)(v10 + 152)];
  v89 = (sdRenderProgram *)&v7->mOffsetEnv;
  v44((VBRState *)&v7->mOffsetEnv);
  ((void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[**(unsigned __int8 **)(v10 + 192)])((VBRState *)&v7->mTintEnv.mEnd);
  v45 = *(unsigned __int8 **)(v10 + 196);
  v46 = *v45;
  halfOpeningAngle = 0.0;
  min = 0.0;
  atten = *(float *)&v45;
  ((void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[v46])((VBRState *)&v7->mFadeEnv.mEnd);
  v47 = v7->vfptr->GetDestSize;
  f = *(float *)(v10 + 200);
  v48 = (VBRState *)v47(v7);
  v49 = (void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[**(unsigned __int8 **)(v10 + 200)];
  halfOpeningAngle = 0.0;
  min = 0.0;
  atten = f;
  v49(v48);
  v50 = v7->vfptr;
  f = *(float *)(v10 + 204);
  v51 = (VBRState *)v50->GetDestRotation(v7);
  v52 = (void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[**(unsigned __int8 **)(v10 + 204)];
  halfOpeningAngle = 0.0;
  min = 0.0;
  atten = f;
  v52(v51);
  ((void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[**(unsigned __int8 **)(v10 + 208)])((VBRState *)&v7->mAngleEnv.mEnd);
  ((void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[**(unsigned __int8 **)(v10 + 212)])((VBRState *)&v7->mOffsetEnv.mEnd);
  if ( **(_BYTE **)(v10 + 152) != 3 || **(_BYTE **)(v10 + 212) != 3 )
	v7->mFlags |= 4u;
  rvParticleParms::HandleRelativeParms(
	*(rvParticleParms **)(v10 + 192),
	&v7->mTintEnv.mEnd.x,
	&v7->mTintEnv.mStart.x,
	3);
  rvParticleParms::HandleRelativeParms(*(rvParticleParms **)(v10 + 196), &v7->mFadeEnv.mEnd, &v7->mFadeEnv.mStart, 1);
  v53 = v7->vfptr;
  LODWORD(halfOpeningAngle) = *(unsigned __int8 *)(v10 + 101);
  v54 = COERCE_FLOAT((int)v53->GetInitSize(v7));
  v55 = v7->vfptr;
  min = v54;
  v56 = v55->GetDestSize(v7);
  rvParticleParms::HandleRelativeParms(
	*(rvParticleParms **)(v10 + 200),
	v56,
	(float *)LODWORD(min),
	SLODWORD(halfOpeningAngle));
  v57 = v7->vfptr->GetInitRotation;
  LODWORD(halfOpeningAngle) = *(unsigned __int8 *)(v10 + 102);
  v58 = COERCE_FLOAT((int)v57(v7));
  v59 = v7->vfptr;
  min = v58;
  v60 = (float *)((int (__thiscall *)(rvParticle *, float))v59->GetDestRotation)(v7, COERCE_FLOAT(LODWORD(v58)));
  rvParticleParms::HandleRelativeParms(*(rvParticleParms **)(v10 + 204), v60, (float *)LODWORD(halfOpeningAngle), v81);
  rvParticleParms::HandleRelativeParms(
	*(rvParticleParms **)(v10 + 208),
	&v7->mAngleEnv.mEnd.x,
	(float *)LODWORD(v91),
	3);
  rvParticleParms::HandleRelativeParms(
	*(rvParticleParms **)(v10 + 212),
	&v7->mOffsetEnv.mEnd.x,
	(float *)LODWORD(t1),
	3);
  ((void (__thiscall *)(rvParticle *, _DWORD))v7->vfptr->ScaleRotation)(v7, LODWORD(idMath::TWO_PI));
  rvParticle::ScaleAngle(v7, idMath::TWO_PI);
  v7->vfptr->HandleOrientation(v7, (rvAngles *)((char *)&angles + 4));
  v61 = *(_DWORD *)(v10 + 104);
  halfOpeningAngle = *(float *)(v61 + 44);
  min = *(float *)(v61 + 40);
  v7->mTrailTime = rvRandom::flrand(min, halfOpeningAngle);
  v62 = rvParticleTemplate::GetTrailCount((rvParticleTemplate *)v10);
  v63 = v7->vfptr->SetModel;
  v7->mTrailCount = v62;
  v7->mTrailRepeat = *(unsigned __int8 *)(v10 + 100);
  v63(v7, *(idRenderModel **)(v10 + 16));
  v7->vfptr->SetupElectricity(v7, (rvParticleTemplate *)v10);
  normal.x = rvBSE::GetAttenuation(v15, (rvSegmentTemplate *)LODWORD(v91));
  rvParticle::Attenuate(normal.x, *(rvParticleParms **)(v10 + 136), &v7->mFadeEnv);
  ((void (__thiscall *)(rvParticle *, _DWORD, _DWORD))v7->vfptr->AttenuateSize)(
	v7,
	LODWORD(normal.x),
	*(_DWORD *)(v10 + 140));
  if ( !((*(_DWORD *)(LODWORD(v91) + 36) >> 14) & 1) || v7->mVelocity.x <= 0.0 )
  {
	f = *(float *)(v10 + 64);
	v85 = *(float *)(v10 + 60);
	v65 = rvRandom::flrand(v85, f);
	goto LABEL_61;
  }
  v86 = v15->mCurrentEndOrigin.x - v15->mOriginalOrigin.x;
  v87 = v15->mCurrentEndOrigin.y - v15->mOriginalOrigin.y;
  v88 = v15->mCurrentEndOrigin.z - v15->mOriginalOrigin.z;
  f = v86 * v86 + v87 * v87 + v88 * v88;
  f = sqrt(f);
  v91 = f;
  if ( *v94 > 0.0 )
  {
	f = v7->mVelocity.x;
	f = f * f;
	f = f - (*v94 + *v94) * v91;
	f = sqrt(f);
	v64 = f / *v94;
	f = v64 - v7->mVelocity.x;
	v91 = -v7->mVelocity.x - v64;
	if ( f >= 0.0 )
	{
	  v65 = v91;
	  if ( v91 >= 0.0 )
	  {
		if ( f < v65 )
		  v65 = f;
	  }
	  else
	  {
		v65 = f;
	  }
	}
	else
	{
	  v65 = v91;
	  if ( v91 < 0.0 )
		v65 = rvParticleTemplate::GetDuration((rvParticleTemplate *)v10);
	}
	goto LABEL_61;
  }
  normal.x = v91 / v7->mVelocity.x;
  if ( normal.x < 0.0 )
  {
	v65 = rvParticleTemplate::GetDuration((rvParticleTemplate *)v10);
LABEL_61:
	normal.x = v65;
  }
  halfOpeningAngle = 0.0;
  v7->mStartTime = birthTime;
  min = 0.0;
  v7->mMotionStartTime = birthTime;
  v7->mLastTrailTime = birthTime;
  v7->mEndTime = birthTime + normal.x;
  v66 = **(unsigned __int8 **)(v10 + 128);
  atten = *(float *)(v10 + 128);
  ((void (__cdecl *)(VBRState *))rvParticleParms::spawnFunctions[v66])((VBRState *)&v102);
  v68 = 0.0;
  if ( *(float *)&v102 != 0.0 )
	v68 = 1.0 / (*(float *)&v102 + normal.x);
  v7->mFriction = v68;
  halfOpeningAngle = v67;
  rvDecalParticle::InitSizeEnv((sdRenderProgram *)&v7->mTintEnv, *(idJointMat **)(v10 + 164), SLODWORD(normal.x));
  rvDecalParticle::InitSizeEnv((sdRenderProgram *)&v7->mFadeEnv, *(idJointMat **)(v10 + 168), SLODWORD(normal.x));
  v69 = v7->vfptr->InitSizeEnv;
  halfOpeningAngle = v70;
  ((void (__thiscall *)(rvParticle *, _DWORD, _DWORD))v69)(v7, *(_DWORD *)(v10 + 172), LODWORD(normal.x));
  v71 = v7->vfptr->InitRotationEnv;
  halfOpeningAngle = v72;
  ((void (__thiscall *)(rvParticle *, _DWORD, _DWORD))v71)(v7, *(_DWORD *)(v10 + 176), LODWORD(normal.x));
  rvDecalParticle::InitSizeEnv((sdRenderProgram *)LODWORD(t1), *(idJointMat **)(v10 + 180), SLODWORD(normal.x));
  halfOpeningAngle = v73;
  rvDecalParticle::InitSizeEnv(v89, *(idJointMat **)(v10 + 184), SLODWORD(normal.x));
  f = *(float *)(v10 + 56);
  v85 = *(float *)(v10 + 52);
  f = rvRandom::flrand(v85, f);
  v86 = f * v15->mGravity.x;
  v87 = v15->mGravity.y * f;
  v88 = f * v15->mGravity.z;
  v74 = v87;
  v75 = v86;
  v76 = v94;
  v86 = v15->mCurrentAxisTransposed.mat[2].x * v88
	  + v15->mCurrentAxisTransposed.mat[1].x * v87
	  + v86 * v15->mCurrentAxisTransposed.mat[0].x;
  v87 = v15->mCurrentAxisTransposed.mat[1].y * v87
	  + v15->mCurrentAxisTransposed.mat[0].y * v75
	  + v15->mCurrentAxisTransposed.mat[2].y * v88;
  v88 = v88 * v15->mCurrentAxisTransposed.mat[2].z
	  + v74 * v15->mCurrentAxisTransposed.mat[1].z
	  + v75 * v15->mCurrentAxisTransposed.mat[0].z;
  *v94 = *v94 + v86;
  v76[1] = v87 + v76[1];
  v76[2] = v76[2] + v88;
}

	
	*/
}
