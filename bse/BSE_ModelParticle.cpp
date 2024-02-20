// BSE_ModelParticle.cpp
//

#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"

#undef min
#undef max

bool rvModelParticle::Render(const rvBSE* effect, rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time, float override) {
	// Ensure the particle has a model and is within its lifespan
	if (!mModel || time <= mStartTime - 0.002 || time >= mEndTime) {
		return false;
	}

	// Calculate normalized time
	float normalizedTime = std::min((time - mStartTime) / (mEndTime - mStartTime), 1.0f - 0.002f / (mEndTime - mStartTime));

	// Evaluate tint, fade, size, and rotation for the current time
	idVec4 tint;
	float fadeValue;
	idVec3 size;
	idAngles rotationAngles;

	pt->mpTintEnvelope->Evaluate(mTintEnv, normalizedTime, 1.0f, &tint.x);
	pt->mpFadeEnvelope->Evaluate(mFadeEnv, normalizedTime, 1.0f, &fadeValue);
	pt->mpSizeEnvelope->Evaluate(mSizeEnv, normalizedTime, 1.0f, &size.x);
	pt->mpRotateEnvelope->Evaluate(mRotationEnv, normalizedTime, 1.0f, &rotationAngles.yaw);

	// Adjust tint based on alpha
//	tint *= alpha;

	// Compute transformation matrix
	idMat3 rotationMatrix = rotationAngles.ToMat3();
	//idMat3 scaleMatrix = idMat3::Scale(size);
	idMat3 transformMatrix = rotationMatrix;

	// Transform model vertices
	const modelSurface_t* surface = mModel->Surface(0);
	const srfTriangles_t* modelTriangles = surface->geometry;
	int vertexOffset = tri->numVerts; // Offset for new vertices in the tri structure

	// Evaluate position and apply override time if specified
	idVec3 position;
	float adjustedTime = (override > 0.0f) ? override - this->mMotionStartTime : time - this->mMotionStartTime;
	EvaluatePosition(effect, pt, position, adjustedTime);

	for (int i = 0; i < modelTriangles->numVerts; ++i) {
		idDrawVert& destVert = tri->verts[vertexOffset + i];
		const idDrawVert& srcVert = modelTriangles->verts[i];

		// Apply transformation
		destVert.xyz = transformMatrix * srcVert.xyz + position;

		// Apply tint
		destVert.color[0] = byte(tint.x * 255);
		destVert.color[1] = byte(tint.y * 255);
		destVert.color[2] = byte(tint.z * 255);
		destVert.color[3] = byte(tint.w * 255);
	}

	// Update triangles to include new vertices
	tri->numVerts += modelTriangles->numVerts;

	// Update triangle indices
	for (int i = 0; i < modelTriangles->numIndexes; ++i) {
		tri->indexes[tri->numIndexes + i] = modelTriangles->indexes[i] + vertexOffset;
	}

	tri->numIndexes += modelTriangles->numIndexes;

	return true;
}