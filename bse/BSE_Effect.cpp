// BSE_Effect.cpp
//




#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"

#include "../renderer/Model_local.h"
//#include "../renderer/RenderCommon.h"

#undef min
#undef max

void rvBSE::Init(const rvDeclEffect* declEffect, renderEffect_s* parms, float time) {
	mStartTime = time;
	mDeclEffect = declEffect;
	mLastTime = time;
	mFlags = 0;
	mDuration = 0.0;
	mAttenuation = 1.0;
	mCost = 0.0;
	mFlags = parms->loop;
	mCurrentLocalBounds[1] = idVec3(0.0f);
	mCurrentLocalBounds[0] = idVec3(0.0f);

	float size = declEffect->mSize;
	mCurrentLocalBounds[0] -= idVec3(size);
	mCurrentLocalBounds[1] += idVec3(size);

	mGrownRenderBounds[0] = idVec3(1.0e30f);
	mGrownRenderBounds[1] = idVec3(-1.0e30f);
	mForcePush = 0;
	mOriginalOrigin = parms->origin;
	mOriginalEndOrigin = idVec3(0.0f);
	memcpy(&mOriginalAxis, &parms->axis, sizeof(mOriginalAxis));

	if (parms->hasEndOrigin) {
		mFlags |= 2u;
		mOriginalEndOrigin = parms->endOrigin;
		mCurrentEndOrigin = parms->endOrigin;
	}

	mCurrentTime = time;
	mCurrentOrigin = mOriginalOrigin;
	mCurrentVelocity = idVec3(0.0f);
	UpdateFromOwner(parms, time, 1);
	mReferenceSound = 0;
	UpdateSegments(time);
	mOriginDistanceToCamera = 0.0;
	mShortestDistanceToCamera = 0.0;
}

float rvBSE::GetAttenuation(rvSegmentTemplate* st) const {
	if (st->mAttenuation.x <= 0.0 && st->mAttenuation.y <= 0.0) {
		return mAttenuation;
	}

	float start = st->mAttenuation.x + 1.0f;
	if (start > mShortestDistanceToCamera) {
		return mAttenuation;
	}

	float end = st->mAttenuation.y - 1.0f;
	if (end >= mShortestDistanceToCamera) {
		float fraction = (mShortestDistanceToCamera - st->mAttenuation.x) / (st->mAttenuation.y - st->mAttenuation.x);
		return (1.0f - fraction) * mAttenuation;
	}

	return 0.0f;
}

void rvBSE::UpdateSoundEmitter(rvSegmentTemplate* st, rvSegment* seg) {
	if (!(mFlags & 8) && st->GetSoundLooping() && (seg->mFlags & 2)) {
		mReferenceSound->StopSound(seg->mSegmentTemplateHandle + 1);
	}
	else {
		soundShaderParms_t parms = {};
		parms.shakes = seg->mSoundVolume;
		*(float*)&parms.soundClass = seg->mFreqShift;
		mReferenceSound->UpdateEmitter(mCurrentOrigin, 0, &parms);
	}
}

const idVec3 rvBSE::GetInterpolatedOffset(float time) const {
	idVec3 result(0.0f);
	float deltaTime = mCurrentTime - mLastTime;
	if (deltaTime >= 0.0020000001) {
		float fraction = 1.0f - (time - mLastTime) / deltaTime;
		result = (mCurrentOrigin - mLastOrigin) * fraction;
	}
	return result;
}

void rvBSE::SetDuration(float time) {
	mDuration = (time < 0.0) ? mDeclEffect->mMinDuration : std::min(time, mDuration);
}

const char* rvBSE::GetDeclName() {
	return mDeclEffect->base->GetName();
}

void rvBSE::UpdateAttenuation() {
	if (mDeclEffect->mFlags & 4) {
		idVec3 origin, axis;
		game->GetPlayerView(origin, axis.ToMat3());
		float distance = (mCurrentOrigin - origin).LengthFast();
		mOriginDistanceToCamera = std::min(std::max(1.0f, distance), 131072.0f);
		mShortestDistanceToCamera = mCurrentLocalBounds.ShortestDistance(origin - mCurrentOrigin);
		mShortestDistanceToCamera = std::min(std::max(1.0f, mShortestDistanceToCamera), 131072.0f);
	}
}

void rvBSE::LoopInstant(float time) {
	if (mDuration == 0.0) {
		mStartTime += mDeclEffect->mMaxDuration + 0.5;
		for (int i = 0; i < mSegments.Num(); ++i) {
			mSegments[i].ResetTime(this, mStartTime);
		}
		if (bse_debug.GetInteger() == 2) {
			common->Printf("BSE: Looping duration %g\n", mDeclEffect->mMaxDuration + 0.5);
		}
		mDeclEffect->mLoopCount++;
	}
}

void rvBSE::LoopLooping(float time) {
	if (mDuration != 0.0) {
		mStartTime += mDuration;
		mDuration = 0.0;
		for (int i = 0; i < mSegments.Num(); ++i) {
			mSegments[i].ResetTime(this, mStartTime);
		}
		if (bse_debug.GetInteger() == 2) {
			common->Printf("BSE: Looping duration: %g", mDuration);
		}
		mDeclEffect->mLoopCount++;
	}
}

bool rvBSE::Service(renderEffect_t* parms, float time, bool spawn, bool& forcePush) {
	UpdateFromOwner(parms, time, 0);
	UpdateAttenuation();
	if (spawn) {
		for (int i = 0; i < mSegments.Num(); ++i) {
			mSegments[i].Check(this, time, (mSegments.Num() - i) * -10.0);
		}
	}
	if (!(mFlags & 8) && parms->loop && mStartTime + mDuration < time) {
		LoopLooping(time);
	}
	bool active = false;
	for (int i = 0; i < mSegments.Num(); ++i) {
		if (mSegments[i].UpdateParticles(this, time)) {
			active = true;
		}
	}
	mFlags &= ~8;
	forcePush = mForcePush;
	mForcePush = 0;
	if (!(mFlags & 8)) {
		if (parms->loop && mStartTime + mDuration < time) {
			LoopInstant(time);
			return false;
		}
		return mStartTime + mDuration < time;
	}
	return !active;
}

float rvBSE::EvaluateCost(int segment) {
	if (segment < 0) {
		mCost = 0.0;
		for (int i = 0; i < mSegments.Num(); ++i) {
			mCost += mDeclEffect->EvaluateCost(mSegments[i].mActiveCount, segment);
		}
	}
	else {
		mCost = mDeclEffect->EvaluateCost(mSegments[segment].mActiveCount, segment);
	}
	return mCost;
}

void rvBSE::InitModel(idRenderModel* model) {
	for (int i = 0; i < mSegments.Num(); ++i) {
		mSegments[i].AllocateSurface(this, model);
	}
}

void rvBSE::UpdateSegments(float time) {
	int numSegments = mDeclEffect->mSegmentTemplates.Num();
	mSegments.SetNum(numSegments);
	for (int i = 0; i < numSegments; ++i) {
		mSegments[i].Init(this, mDeclEffect, i, time);
	}
	for (int i = 0; i < mSegments.Num(); ++i) {
		mSegments[i].CalcCounts(this, time);
	}
	for (int i = 0; i < mSegments.Num(); ++i) {
		mSegments[i].InitParticles(this);
	}
}

void rvBSE::Destroy() {
	mSegments.Clear();
	if (mReferenceSound) {
		mReferenceSound->Free(false);
	}
}

void rvBSE::DisplayDebugInfo(const struct renderEffect_s* parms, const struct viewDef_s* view, idBounds& bounds) {
	// TODO
}

void __thiscall rvBSE::UpdateFromOwner(renderEffect_s* parms, float time, bool init)
{
	 // Update timing and position information
    mLastTime = mCurrentTime;
    mLastOrigin = mCurrentOrigin;
    mCurrentTime = time;
    mCurrentOrigin = parms->origin;

    // Update orientation and wind vector
    mCurrentAxis = parms->axis;
    mCurrentAxisTransposed = mCurrentAxis.Transpose(); // Assuming Transpose method is available
   // mCurrentWindVector = parms->axis * parms->windVector; // Assuming operator* is implemented for this // Quake Wars

    // Update velocity if there has been a significant change in time
    float deltaTime = mCurrentTime - mLastTime;
    if (deltaTime > 0.002f) {
        mCurrentVelocity = (mCurrentOrigin - mLastOrigin) / deltaTime;
    }

    // Update gravity direction and magnitude
    mGravity = parms->gravity;
    float gravityLength = mGravity.Length();
    if (gravityLength >= 1e-6) {
        mGravityDir = mGravity / gravityLength;
    }
// todo
    // Update bounding box based on whether the entity uses render bounds or is static
    //if (parms->useRenderBounds || parms->isStatic) {
    //    // Assuming a method to update or calculate world bounds based on the current status
    //    UpdateWorldBounds(parms);
    //} else {
    //    ResetWorldBounds();
    //}
	//
    //// Debug visualization, assuming it's encapsulated in a method
    //if (bse_debug.GetInt() > 2) {
    //    DebugVisualize(parms);
    //}
	//
    //// Update end origin and related calculations if necessary
    //if (ShouldUpdateEndOrigin(parms, init)) {
    //    UpdateEndOrigin(parms);
    //}

	// Update the local bounds based on current and end origins
	mCurrentLocalBounds.Clear();

	// Adjust local bounds to include the effect's current origin
	mCurrentLocalBounds.AddPoint(mCurrentOrigin - idVec3(mDeclEffect->mSize));
	mCurrentLocalBounds.AddPoint(mCurrentOrigin + idVec3(mDeclEffect->mSize));

	// If the effect has an end origin, adjust the local bounds to include it as well
	if (parms->hasEndOrigin) {
		mCurrentLocalBounds.AddPoint(mCurrentEndOrigin - idVec3(mDeclEffect->mSize));
		mCurrentLocalBounds.AddPoint(mCurrentEndOrigin + idVec3(mDeclEffect->mSize));
	}

	// Convert local bounds to world bounds by applying the current axis transformations
	// This process ensures that the bounds are correctly oriented in the world space
	idBounds tempBounds = mCurrentLocalBounds;
	mCurrentWorldBounds.Clear();
	for (int i = 0; i < 8; ++i) {
		idVec3 corner = tempBounds.GetCorner(i);
		idVec3 worldCorner = mCurrentOrigin + corner * mCurrentAxis;
		mCurrentWorldBounds.AddPoint(worldCorner);
	}

    // Update additional properties like tint, brightness, material color, etc.
    mTint = idVec4(parms->shaderParms[0], parms->shaderParms[1], parms->shaderParms[2], parms->shaderParms[3]);
    mBrightness = parms->shaderParms[6];
//  mSuppressLightsInViewID = parms->suppressLightsInViewID; // Quake Wars
    mAttenuation = parms->attenuation;
	mMaterialColor = idVec3(1, 1, 1); // parms->materialColor; // Quake Wars

	// Process end origin if it has changed or if it's the initial update
	if (parms->hasEndOrigin && (init || mCurrentEndOrigin != parms->endOrigin)) {
		mCurrentEndOrigin = parms->endOrigin;

		// Adjust the world bounds to include the end origin, expanding as needed
		mCurrentWorldBounds.AddPoint(mCurrentEndOrigin);

		// If there's a significant change in position indicating movement or initial setup,
		// adjust internal representations accordingly
		//if (init || mLastOrigin != mCurrentOrigin || mLastEndOrigin != mCurrentEndOrigin) {
		//	mLastEndOrigin = mCurrentEndOrigin; // Update last end origin for next comparison
		//}
	}
}