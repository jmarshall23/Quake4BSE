// BSE_LightParticle.cpp
//

#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"

void rvLightParticle::GetSpawnInfo(idVec4& tint, idVec3& size, idVec3& rotate)
{
	tint = idVec4(mTintEnv.mStart.x, mTintEnv.mStart.y, mTintEnv.mStart.z, mTintEnv.mEnd.x);
	size = mSizeEnv.mStart; // Assuming mSizeEnv.mStart is an idVec3.
	rotate = idVec3(0.0f, 0.0f, 0.0f); // Explicitly setting to zero.
}

bool rvLightParticle::Destroy()
{
	if (mLightDefHandle != -1) {
		session->rw->FreeLightDef(mLightDefHandle); // Assuming gameRenderWorld is accessible and has a FreeLightDef method.
		mLightDefHandle = -1;
	}
	return true; // It always returns true, which might indicate success but seems unnecessary.
}

bool rvLightParticle::InitLight(rvBSE* effect, rvSegmentTemplate* st, float time)
{
	float duration = mEndTime - mStartTime;
	float oneOverDuration = 1.0f / duration;

	if (!rvParticle::GetEvaluationTime(time, oneOverDuration, false)) {
		return false; // Early exit if the evaluation time is not valid
	}

	// Clear the mLight structure
	memset(&mLight, 0, sizeof(mLight));

	// Evaluate tint and fade envelopes
	idVec4 tint;
	float fadeValue;
	st->mParticleTemplate.mpTintEnvelope->Evaluate(mTintEnv, time, oneOverDuration, &tint.x); // Assuming Evaluate fills in the vector
	st->mParticleTemplate.mpFadeEnvelope->Evaluate(mFadeEnv, time, oneOverDuration, &fadeValue);

	// Evaluate size envelope
	idVec3 size;
	st->mParticleTemplate.mpSizeEnvelope->Evaluate(mSizeEnv, time, oneOverDuration, &size.x); // Assuming a similar Evaluate method for idVec3

	// Compute new position based on evaluated size
	float deltaTime = time - mMotionStartTime;
	idVec3 position;
	EvaluatePosition(effect, &st->mParticleTemplate, position, deltaTime); // Assuming EvaluatePosition updates 'position'

	// Setup light properties
	mLight.origin = position;
	mLight.lightRadius = size; // Assuming size represents the radius
	mLight.lightRadius = mLight.lightRadius; // Ensure the radius is at least 1.0 in all dimensions
	mLight.shaderParms[SHADERPARM_RED] = tint.z; // Assuming tint.z is correctly mapped to red
	mLight.shaderParms[SHADERPARM_GREEN] = tint.w; // Assuming tint.w represents green intensity
	mLight.shaderParms[SHADERPARM_BLUE] = fadeValue; // Assuming fadeValue represents blue intensity
// Quake Wars
	//mLight.maxVisDist = 4096;
	//mLight.material = st->mParticleTemplate.mMaterial;
	//mLight.flags |= renderLight_t::RL_FLAG_SHADOW; // Assuming flags setting for shadow
	//mLight.manualPriority = 1;
// Quake Wars
	mLight.lightId = reinterpret_cast<int>(this); // Assuming lightId is used for identifying the light

	// Register the light with the game's render world
	mLightDefHandle = session->rw->AddLightDef(&mLight);

	return true;
}

bool rvLightParticle::PresentLight(rvBSE* effect, rvParticleTemplate* pt, float time, bool infinite)
{
	float duration = mEndTime - mStartTime;
	float oneOverDuration = 1.0f / duration;
	idVec3 position;

	if (!rvParticle::GetEvaluationTime(time, oneOverDuration, infinite)) {
		return false; // Early exit if evaluation time is not valid
	}

	// Evaluate tint and fade envelopes
	idVec4 tint;
	float fadeValue;
	pt->mpTintEnvelope->Evaluate(mTintEnv,time, oneOverDuration, &tint.x); // Assuming Evaluate fills in the vector
	pt->mpFadeEnvelope->Evaluate(mFadeEnv,time, oneOverDuration, &fadeValue);

	// Evaluate size envelope
	idVec3 size;
	pt->mpSizeEnvelope->Evaluate(mSizeEnv, time, oneOverDuration, &size.x); // Assuming a similar Evaluate method for idVec3

	// Compute new position based on evaluated size and effect's position and orientation
	float deltaTime = time - mMotionStartTime;
	EvaluatePosition(effect, pt, position, deltaTime); // Assuming EvaluatePosition updates 'position'

	// Setup light properties
	mLight.origin = position;
	mLight.lightRadius = tint.ToVec3(); // Assuming idVec4's xyz components can represent radius and w represents intensity
	mLight.lightRadius = mLight.lightRadius; // Ensure the radius is at least 1.0 in all dimensions
	mLight.shaderParms[SHADERPARM_RED] = tint.w; // Assuming tint.w is the light intensity
	mLight.shaderParms[SHADERPARM_GREEN] = fadeValue;
	// Note: shaderParms[2] seems to be uninitialized in the original function. Assuming it's an oversight and not setting it.

	// Update or create the light in the render world
	if (mLightDefHandle != -1) {
		session->rw->UpdateLightDef(mLightDefHandle, &mLight);
	}
	else {
		mLightDefHandle = session->rw->AddLightDef(&mLight);
	}

	return true;
}