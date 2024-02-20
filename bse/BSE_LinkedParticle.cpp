// BSE_LinkedParticle.cpp
//

#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"

void rvLinkedParticle::HandleTiling(rvParticleTemplate * pt) {
	if (mFlags & 0x100000) {
		// Apply tiling settings from the particle template
		mTextureScale = pt->mTiling;
		mTextureOffset = 0.0; // Reset the texture offset to ensure tiling starts from the beginning
	}
}

void rvLinkedParticle::FinishSpawn(rvBSE* effect, rvSegment* segment, float birthTime, float fraction, const idVec3& initOffset, const idMat3& initAxis) {
	rvSegmentTemplate* segmentTemplate = segment->GetSegmentTemplate();
	if (segmentTemplate) {
		// Complete the base spawning process
		rvParticle::FinishSpawn(effect, segment, birthTime, fraction, initOffset, initAxis);
		// Apply any template-specific adjustments, such as texture tiling
		HandleTiling(&segmentTemplate->mParticleTemplate);
	}
}

bool rvLinkedParticle::Render(const rvBSE* effect, rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override) {
	// Initial checks and setup
	if (this->mStartTime - 0.002f >= time || this->mEndTime <= time)
		return false; // Early out if the time is outside the particle's active range

	float elapsedTime = time - this->mStartTime;
	if (this->mEndTime - 0.002f <= time)
		elapsedTime = this->mEndTime - this->mStartTime - 0.002f;

	float duration = this->mEndTime - this->mStartTime;
	float oneOverDuration = 1.0f / duration;

	idVec4 tint;
	float dest;
	// Evaluate tint and fade based on elapsed time
	pt->mpTintEnvelope->Evaluate(mTintEnv, elapsedTime, oneOverDuration, &tint.x);
	pt->mpFadeEnvelope->Evaluate(mFadeEnv, elapsedTime, oneOverDuration, &dest);

	idVec2 size;
	pt->mpSizeEnvelope->Evaluate(mSizeEnv, elapsedTime, oneOverDuration, &size.x);

	// Position evaluation
	idVec3 position;
	float motionElapsedTime = time - this->mMotionStartTime;
	this->EvaluatePosition(effect, pt, position, motionElapsedTime);

	// Check for lightning effect flag and adjust direction if necessary
	idVec3 dir;
	// Check if the particle is affected by the lightning effect
	if (((unsigned int)mFlags >> 22) & 1) {
		// Calculate new direction and position based on the lightning axis
		// This applies a transformation to simulate the effect of lightning on the particle's trajectory
		float timec = effect->mLightningAxis.mat[2].x * dir.x + effect->mLightningAxis.mat[0].x * position.y + effect->mLightningAxis.mat[1].x * position.z;
		float ptb = effect->mLightningAxis.mat[1].y * position.z + effect->mLightningAxis.mat[0].y * position.y + effect->mLightningAxis.mat[2].y * dir.x;
		// Adjust the direction of the particle based on the lightning effect
		dir.x = position.y * effect->mLightningAxis.mat[0].z + position.z * effect->mLightningAxis.mat[1].z + dir.x * effect->mLightningAxis.mat[2].z;
		// Update the position to reflect the effect of the lightning
		position.y = timec;
		position.z = ptb;
	}

	// Setup for rendering
	// Assuming vertices are to be directly modified based on the position and size
	for (int i = 0; i < 2; ++i) { // Assuming 2 vertices to simplify
		idDrawVert& vert = tri->verts[tri->numVerts + i];
		vert.xyz = position + idVec3(size.x, size.y, 0.0f); // Example modification
		vert.SetColor(dest); // Pseudo code to set color based on tint and dest
	}

	// Update indices for triangle strip or fan
	// Example update, actual logic depends on how the vertices are meant to form triangles
	int baseIndex = tri->numVerts;
	tri->numVerts += 2; // Increment by 2 or more depending on actual vertex usage
	
	tri->indexes[tri->numIndexes++] = baseIndex;
	tri->indexes[tri->numIndexes++] = baseIndex + 1;
	tri->indexes[tri->numIndexes++] = baseIndex + 2;
	tri->indexes[tri->numIndexes++] = baseIndex + 2;
	tri->indexes[tri->numIndexes++] = baseIndex + 1;
	tri->indexes[tri->numIndexes++] = baseIndex + 3;

	return true; // Successful render
}
