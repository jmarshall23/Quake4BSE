// BSE_Segment.cpp
//




#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"

#include "../renderer/tr_local.h"

#undef min
#undef max

rvSegment::~rvSegment() {

}

void rvSegment::Handle(rvBSE* effect, float time) {
	rvSegmentTemplate* segmentTemplate = (rvSegmentTemplate*)(mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle));
	if (segmentTemplate && mSegStartTime <= time) {
		switch (segmentTemplate->mSegType) {
		case SEG_EMITTER:
		case SEG_SPAWNER:
			if (effect->GetEndOriginChanged())
				RefreshParticles(effect, segmentTemplate);
			break;
		case SEG_SOUND:
			effect->UpdateSoundEmitter(segmentTemplate, this);
			break;
		case SEG_LIGHT:
			if (segmentTemplate->mFlags & 1)
				HandleLight(effect, segmentTemplate, time);
			break;
		default:
			return;
		}
	}
}

void rvSegment::ValidateSpawnRates() {
	float minSecondsPerParticle = 0.002f;
	float maxSecondsPerParticle = 300.0f;
	float secondsPerParticleY = std::max(minSecondsPerParticle, std::min(maxSecondsPerParticle, mSecondsPerParticle.y));
	mSecondsPerParticle.y = secondsPerParticleY;
	float secondsPerParticleX = mSecondsPerParticle.x;
	if (secondsPerParticleX < secondsPerParticleY)
		mSecondsPerParticle.x = secondsPerParticleY;
	else
		mSecondsPerParticle.x = std::min(secondsPerParticleX, secondsPerParticleY);
}

void rvSegment::GetSecondsPerParticle(rvBSE* effect, rvSegmentTemplate* st, rvParticleTemplate* pt) {
	double volume = pt->GetSpawnVolume(effect);
	double minVolume = 0.002;
	double maxVolume = 2048.0;
	double boundedVolume = std::max(minVolume, std::min(maxVolume, volume));
	if (volume == 0.0)
		mCount = st->mCount;
	else {
		mCount.x = st->mDensity.x * boundedVolume;
		mCount.y = boundedVolume * st->mDensity.y;
	}
	int segmentType = st->mSegType;
	if (segmentType == SEG_EMITTER || segmentType == SEG_SPAWNER) {
		if (mCount.x != 0.0)
			mSecondsPerParticle.x = 1.0 / mCount.x;
		if (mCount.y != 0.0)
			mSecondsPerParticle.y = 1.0 / mCount.y;
		ValidateSpawnRates();
	}
}

void rvSegment::InitTime(rvBSE* effect, rvSegmentTemplate* st, float time) {
	mFlags &= ~1;
	mSegStartTime = rvRandom::flrand(st->mLocalStartTime.x, st->mLocalStartTime.y) + time;
	float duration = rvRandom::flrand(st->mLocalDuration.x, st->mLocalDuration.y);
	mSegEndTime = mSegStartTime + duration;
	if (!(st->mFlags & 0x10) || (!effect->GetLooping() && !st->GetSoundLooping())) {
		float segDuration = mSegEndTime - time;
		effect->SetDuration(segDuration);
	}
}

float rvSegment::AttenuateDuration(rvBSE* effect, rvSegmentTemplate* st) {
	float attenuation = effect->GetAttenuation(st);
	if ((st->mFlags & 0x80) != 0)
		attenuation = 1.0f - attenuation;
	if (attenuation < 0.002f)
		return 1.0f;
	float attenuatedDuration = mSegEndTime - mSegStartTime;
	float result = attenuatedDuration / attenuation;
	if (result <= 0.00000011920929f)
		result = 0.00000023841858f;
	return result;
}

float rvSegment::AttenuateInterval(rvBSE* effect, rvSegmentTemplate* st) {
	float minInterval = (mSecondsPerParticle.y - mSecondsPerParticle.x) * 1.0f + mSecondsPerParticle.x;
	if (mSecondsPerParticle.y <= minInterval) {
		if (mSecondsPerParticle.x < minInterval)
			minInterval = mSecondsPerParticle.x;
	}
	else {
		minInterval = mSecondsPerParticle.y;
	}
	if ((st->mFlags & 0x40) == 0)
		return minInterval;
	float attenuation = effect->GetAttenuation(st);
	if ((st->mFlags & 0x80) != 0)
		attenuation = 1.0f - attenuation;
	if (attenuation < 0.002f)
		return 1.0f;
	float attenuatedInterval = minInterval / attenuation;
	float result = attenuatedInterval;
	if (attenuatedInterval <= 0.00000011920929f)
		result = 0.00000023841858f;
	return result;
}

float rvSegment::AttenuateCount(rvBSE* effect, rvSegmentTemplate* st, float min, float max) {
	float countRange = max - min;
	float randomCount = rvRandom::flrand(min, min + countRange);
	if (min <= randomCount) {
		if (max < randomCount)
			randomCount = max;
	}
	else {
		randomCount = min;
	}
	if ((st->mFlags & 0x40) != 0) {
		float attenuation = effect->GetAttenuation(st);
		if ((st->mFlags & 0x80) != 0)
			attenuation = 1.0f - attenuation;
		randomCount *= attenuation;
	}
	return randomCount;
}

void rvSegment::RefreshParticles(rvBSE* effect, rvSegmentTemplate* st) {
	if (st->mParticleTemplate.UsesEndOrigin()) {
		rvParticle* particle = mUsedHead;
		while (particle) {
			rvParticle* nextParticle = particle->GetNext();
			particle->Refresh(effect, st, &st->mParticleTemplate);
			particle = nextParticle;
		}
	}
}

void rvParticle::DoRenderBurnTrail(rvBSE* effect, rvParticleTemplate* pt, const idMat3& view, srfTriangles_t* tri, float time) {
	int trailCount = mTrailCount;
	if (trailCount) {
		if (mTrailTime != 0.0) {
			int delta = 1;
			float deltaTime = mTrailTime / trailCount;
			for (int i = 1; i <= trailCount; ++i) {
				float trailTime = time - delta * deltaTime;
				if (mStartTime <= trailTime && mEndTime > trailTime) {
					float trailRatio = static_cast<float>(mTrailCount - i) / static_cast<float>(mTrailCount);
					float interpolatedTime = trailTime;
					Render(effect, pt, view, tri, interpolatedTime, trailRatio);
				}
				++delta;
			}
		}
	}
}

void rvSegment::RenderMotion(rvBSE* effect, const renderEffect_s* owner, idRenderModel* model, rvParticleTemplate* pt, float time) {
	const modelSurface_t* surface = model->Surface(mSurfaceIndex + 1);
	srfTriangles_t* triangles = surface->geometry;
	rvParticle* particle = mUsedHead;
	while (particle) {
		particle->RenderMotion(effect, pt, triangles, owner, time, pt->GetTrailScale());
		particle = particle->GetNext();
	}
	R_BoundTriSurf(triangles);
}

rvParticle* rvSegment::SpawnParticle(rvBSE* effect, rvSegmentTemplate* st, float birthTime, const idVec3& initOffset, const idMat3& initAxis) {
	rvParticle* particle = nullptr;
	if ((mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle)->mFlags & 0x100) != 0)
		particle = mParticles;
	else {
		if (mFreeHead) {
			rvParticle* nextParticle = mUsedHead;
			mFreeHead->SetNext(nextParticle);
			mUsedHead = mFreeHead;
			mFreeHead = mFreeHead->GetNext();
			particle = mUsedHead;
		}
	}
	if (particle)
		particle->FinishSpawn(effect, this, birthTime, birthTime - effect->GetStartTime(), initOffset, initAxis);
	return particle;
}

void rvSegment::SpawnParticles(rvBSE* effect, rvSegmentTemplate* st, float birthTime, int count) {
	int totalCount = count;
	while (totalCount > 0) {
		if ((mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle)->mFlags & 0x100) == 0) {
			if (mFreeHead) {
				rvParticle* nextParticle = mUsedHead;
				mFreeHead->SetNext(nextParticle);
				mUsedHead = mFreeHead;
				mFreeHead = mFreeHead->GetNext();
				float countRatio = static_cast<float>(mLoopParticleCount - totalCount) / static_cast<float>(mLoopParticleCount - 1);
				mUsedHead->FinishSpawn(effect, this, birthTime, countRatio, vec3_origin, mat3_identity);
			}
		}
		else {
			rvParticle* nextParticle = mParticles;
			float countRatio = static_cast<float>(mLoopParticleCount - totalCount) / static_cast<float>(mLoopParticleCount - 1);
			mParticles->FinishSpawn(effect, this, birthTime, countRatio, vec3_origin, mat3_identity);
		}
		--totalCount;
	}
}

void rvSegment::PlayEffect(rvBSE* effect, rvSegmentTemplate* st, float time) {
	//if (mEffectDecl->IsSpawnTimeValid(time)) {
		int count = static_cast<int>(AttenuateCount(effect, st, st->mCount.x, st->mCount.y));
		if (count > 0) {
			SpawnParticles(effect, st, time, count);
		}
	//}
}

bool rvSegment::UpdateParticles(rvBSE* effect, float time) {
	bool result = false;
	if (mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle)->mFlags & 0x100) {
		rvParticle* particle = mParticles;
		while (particle) {
			rvParticle* nextParticle = particle->GetNext();
			result |= particle->Update((rvParticleTemplate *)effect, time);
			particle = nextParticle;
		}
	}
	else {
		rvParticle* particle = mUsedHead;
		while (particle) {
			rvParticle* nextParticle = particle->GetNext();
			if (particle->Update((rvParticleTemplate*)effect, time)) {
				particle->SetNext(mFreeHead);
				mFreeHead = particle;
			}
			else {
				result = true;
			}
			particle = nextParticle;
		}
	}
	return result;
}

bool rvSegment::Active() {
	rvSegmentTemplate* segmentTemplate = (rvSegmentTemplate*)mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
	if (segmentTemplate && (segmentTemplate->mFlags & 4) && mActiveCount)
		return segmentTemplate->mFlags & 1;
	return false;
}

void rvSegment::AllocateSurface(rvBSE* effect, idRenderModel* model) {
	rvSegmentTemplate* segmentTemplate = (rvSegmentTemplate*)mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
	if (segmentTemplate && (segmentTemplate->mFlags & 4)) {
		rvParticleTemplate* particleTemplate = &segmentTemplate->mParticleTemplate;
		int particleCount = effect->GetLooping() ? mLoopParticleCount : mParticleCount;
		srfTriangles_t* triangles = model->AllocSurfaceTriangles(particleCount * particleTemplate->GetVertexCount(), particleCount * particleTemplate->GetIndexCount());

		modelSurface_t surf;
		surf.id = 0;
		surf.geometry = triangles;
		surf.shader = particleTemplate->GetMaterial();
		model->AddSurface(surf);

		mSurfaceIndex = model->NumSurfaces() - 1;

		if (particleTemplate->GetTrailType() == 2 && (particleTemplate->GetMaxTrailCount() || particleTemplate->GetMaxTrailTime() >= 0.0020000001)) {
			int trailCount = particleTemplate->GetMaxTrailCount();
			srfTriangles_t* trailTriangles = model->AllocSurfaceTriangles(2 * particleCount * trailCount + 2, 12 * particleCount * trailCount);

			modelSurface_t trailSurf;
			trailSurf.id = 0;
			trailSurf.geometry = trailTriangles;
			trailSurf.shader = particleTemplate->GetTrailMaterial();
			model->AddSurface(trailSurf);

			mFlags |= 4u;
		}
	}
}

void rvSegment::ClearSurface(rvBSE* effect, idRenderModel* model) {
	rvSegmentTemplate* segmentTemplate = (rvSegmentTemplate*)mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
	if (segmentTemplate && (segmentTemplate->mFlags & 4) != 0) {
		const modelSurface_t* surface = model->Surface(mSurfaceIndex);
		if (segmentTemplate->mParticleTemplate.GetType() == 7) {
			model->FreeSurfaceTriangles(surface->geometry);
			int count = effect->GetLooping() ? mLoopParticleCount : mParticleCount;
			int totalCount = count * mActiveCount;
			int vertexCount = totalCount * segmentTemplate->mParticleTemplate.GetVertexCount();
			int indexCount = totalCount * segmentTemplate->mParticleTemplate.GetIndexCount();
			if (vertexCount > 10000) vertexCount = 10000;
			if (indexCount > 30000) indexCount = 30000;
			srfTriangles_t* triangles = model->AllocSurfaceTriangles(vertexCount, indexCount);
			// triangles->texCoordScale = 100.0;
			if ((mFlags & 4) != 0) {
				modelSurface_t* trailSurface = (modelSurface_t*)model->Surface(mSurfaceIndex + 1);
				model->FreeSurfaceTriangles(trailSurface->geometry);
				int trailVertexCount = 2 * totalCount + 2;
				int trailIndexCount = 12 * totalCount;
				if (trailVertexCount > 10000) trailVertexCount = 10000;
				if (trailIndexCount > 30000) trailIndexCount = 30000;
				trailSurface->geometry = model->AllocSurfaceTriangles(trailVertexCount, trailIndexCount);
				// trailSurface->geometry->texCoordScale = 100.0;
			}
		}
		else {
			srfTriangles_t* geometry = surface->geometry;
			geometry->numIndexes = 0;
			geometry->numVerts = 0;
			if ((mFlags & 4) != 0) {
				srfTriangles_t* trailGeometry = model->Surface(mSurfaceIndex + 1)->geometry;
				trailGeometry->numIndexes = 0;
				trailGeometry->numVerts = 0;
			}
		}
	}
}

void rvSegment::RenderTrail(rvBSE* effect, const renderEffect_s* owner, idRenderModel* model, float time) {
	rvSegmentTemplate* segmentTemplate = (rvSegmentTemplate*)mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
	if (segmentTemplate) {
		rvParticleTemplate* particleTemplate = &segmentTemplate->mParticleTemplate;
		float maxTrailTime = particleTemplate->GetMaxTrailTime();
		if (maxTrailTime >= 0.0020000001 && particleTemplate->GetTrailType() == 2)
			rvSegment::RenderMotion(effect, owner, model, particleTemplate, time);
	}
}

void rvSegment::Init(rvBSE* effect, const rvDeclEffect* effectDecl, int segmentTemplateHandle, float time) {
	mSegmentTemplateHandle = segmentTemplateHandle;
	mFlags = 0;
	mEffectDecl = effectDecl;
	mParticleType = effectDecl->GetSegmentTemplate(segmentTemplateHandle)->mParticleTemplate.GetType();
	mSurfaceIndex = -1;
	rvSegmentTemplate* segmentTemplate = (rvSegmentTemplate*)effectDecl->GetSegmentTemplate(segmentTemplateHandle);
	if (segmentTemplate) {
		mActiveCount = 0;
		mLastTime = time;
		// Additional initialization code if needed
	}
}

void rvSegment::AddToParticleCount(rvBSE* effect, int count, int loopCount, float duration) {
	rvSegmentTemplate* segmentTemplate = (rvSegmentTemplate*)mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
	if (segmentTemplate) {
		if (duration < (double)segmentTemplate->mParticleTemplate.GetMaxDuration())
			duration = segmentTemplate->mParticleTemplate.GetMaxDuration();
		float durationa = duration + 0.01600000075995922;
		float durationb = durationa / this->mSecondsPerParticle.y;
		int durationc = (int)ceil(durationb);
		int totalCount = (durationc + 1) * (loopCount + count);
		if (effect->GetLooping()) {
			if (totalCount > 2048) {
				common->Warning("More than MAX_PARTICLES required for segment %s\n", segmentTemplate->GetSegmentName().c_str());
				totalCount = 2048;
			}
			this->mLoopParticleCount = totalCount;
		}
		else {
			if (totalCount > 2048) {
				common->Warning("More than MAX_PARTICLES required for segment %s\n", segmentTemplate->GetSegmentName().c_str());
				totalCount = 2048;
			}
			this->mParticleCount = totalCount;
		}
	}
}

void rvSegment::CalcTrailCounts(rvBSE* effect, rvSegmentTemplate* st, rvParticleTemplate* pt, float duration) {
	if (st->mTrailSegmentIndex >= 0) {
		effect->GetTrailSegment(st->mTrailSegmentIndex)->AddToParticleCount(effect, mParticleCount, mLoopParticleCount, duration);
	}
}

void rvSegment::CalcCounts(rvBSE* effect, float time) {
	rvSegmentTemplate* segmentTemplate = (rvSegmentTemplate*)mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
	if (segmentTemplate) {
		float effectMinDuration = segmentTemplate->GetParticleTemplate()->GetMaxDuration() + 0.01600000075995922;
		float pt = mEffectDecl->GetMinDuration();
		float duration = segmentTemplate->mLocalDuration.y;
		int segType = segmentTemplate->mSegType;
		int count = 0;
		int loopCount = 0;
		switch (segType) {
		case SEG_EMITTER:
			if (segmentTemplate->mParticleTemplate.GetType() == 10) {
				count = loopCount = 1;
			}
			else {
				float durationPlus = duration < (double)effectMinDuration ? segmentTemplate->GetParticleTemplate()->GetMaxDuration() + 0.01600000075995922 : duration + 0.01600000075995922;
				float durationDiv = durationPlus / this->mSecondsPerParticle.y;
				int durationCeil = (int)ceilf(durationDiv);
				count = loopCount = durationCeil + 1;
				if (effectMinDuration > (double)pt) {
					loopCount = (int)ceilf(effectMinDuration * (durationCeil + 1) / pt) + 1;
				}
			}
			break;
		case SEG_SPAWNER:
			if (segmentTemplate->mParticleTemplate.GetType() == 10) {
				count = loopCount = 1;
			}
			else {
				count = loopCount = (int)ceilf(this->mCount.y);
				if (pt != 0.0 && !(segmentTemplate->mFlags & 0x20) && effectMinDuration > pt) {
					loopCount *= ((int)ceilf(effectMinDuration / pt) + 1);
					loopCount++;
				}
			}
			break;
		default:
			break;
		}
		if (effect->GetLooping() && loopCount > 2048) {
			common->Warning("More than MAX_PARTICLES required for segment %s\n", segmentTemplate->GetSegmentName().c_str());
			loopCount = 2048;
		}
		if (!effect->GetLooping() && count > 2048) {
			common->Warning("More than MAX_PARTICLES required for segment %s\n", segmentTemplate->GetSegmentName().c_str());
			count = 2048;
		}
		mParticleCount = count;
		mLoopParticleCount = loopCount;
		if ((segmentTemplate->mFlags & 4) && (count == 0 || loopCount == 0)) {
			common->Warning("Segment with no particles for effect %s\n", mEffectDecl->base->GetName());
			int segType = segmentTemplate->mSegType;
			if (segType == SEG_EMITTER || segType == SEG_SPAWNER) {
				CalcTrailCounts(effect, segmentTemplate, &segmentTemplate->mParticleTemplate, duration);
			}
		}
		if (!effect->GetLooping()) {
			float effecta = mSegEndTime - time + segmentTemplate->mParticleTemplate.GetMaxDuration();
			effect->SetDuration(effecta);
		}
	}
}

void rvSegment::ResetTime(rvBSE* effect, float time) {
	rvSegmentTemplate* segmentTemplate = (rvSegmentTemplate*)mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
	if (segmentTemplate && !(segmentTemplate->mFlags & 0x20)) {
		InitTime(effect, segmentTemplate, time);
	}
}

rvParticle* rvSegment::InitParticleArray(rvBSE* effect) {
	int particleCount = effect->GetLooping() ? mLoopParticleCount : mParticleCount;
	int type = mEffectDecl->GetSegmentTemplate(mSegmentTemplateHandle)->mParticleTemplate.GetType();
	rvParticle* particle = nullptr;
	rvParticle* prevParticle = nullptr;
	for (int i = 0; i < particleCount - 1; i++) {
		switch (type) {
		case PTYPE_LINE:        particle = new rvLineParticle(); break;
		case PTYPE_ORIENTED:    particle = new rvOrientedParticle(); break;
		case PTYPE_DECAL:       particle = new rvDecalParticle(); break;
		case PTYPE_MODEL:       particle = new rvModelParticle(); break;
		case PTYPE_LIGHT:       particle = new rvLightParticle(); break;
		case PTYPE_ELECTRICITY: particle = new rvElectricityParticle(); break;
		case PTYPE_LINKED:      particle = new rvLinkedParticle(); break;
		//case PTYPE_ORIENTEDLINKED: particle = new sdOrientedLinkedParticle(); break;
		case PTYPE_DEBRIS:      particle = new rvDebrisParticle(); break;
		default:                particle = new rvSpriteParticle(); break;
		}
		if (i > 0) {
			prevParticle->SetNext(particle);
		}
		if (i == 0) {
			mFreeHead = particle;
		}
		prevParticle = particle;
	}
	if (particleCount > 0) {
		prevParticle->SetNext(nullptr);
	}
	mUsedHead = nullptr;
	return mFreeHead;
}

void rvSegment::Sort(const idVec3& eyePos) {
	// This used smooth sort to sort the particles from the eye position.
	// Needs eval.
}

void rvSegment::InitParticles(rvBSE* effect)
{
	if (mEffectDecl->GetSegmentTemplate(this->mSegmentTemplateHandle))
	{
		mParticles = (rvParticle*)InitParticleArray(effect);
		mActiveCount = 0;
	}
}

void rvSegment::Render(rvBSE* effect, const renderEffect_s* owner, idRenderModel* model, float time) {
	const rvDeclEffect* effectDecl = mEffectDecl;
	rvSegmentTemplate* segmentTemplate = nullptr;
	rvParticleTemplate* particleTemplate = nullptr;

	if (effectDecl) {
		segmentTemplate = (rvSegmentTemplate * )effectDecl->GetSegmentTemplate(mSegmentTemplateHandle);
		if (segmentTemplate) {
			particleTemplate = (rvParticleTemplate*) & segmentTemplate->mParticleTemplate;

			const modelSurface_t* surface = model->Surface(mSurfaceIndex);
			srfTriangles_t* triangles = surface->geometry;
			int startVerts = triangles->numVerts;
			int startIndexes = triangles->numIndexes;

			idMat3 view;
			float modelMatrix[16];

			R_AxisToModelMatrix(owner->axis, owner->origin, modelMatrix);
			R_GlobalVectorToLocal(modelMatrix, effect->GetViewAxis()[1], view[1]);
			R_GlobalVectorToLocal(modelMatrix, effect->GetViewAxis()[2], view[2]);

			float offsetX = effect->GetViewOrg().x - owner->origin.x;
			float offsetY = effect->GetViewOrg().y - owner->origin.y;
			float offsetZ = effect->GetViewOrg().z - owner->origin.z;

			float axis20 = owner->axis[2].z;
			float axis10 = owner->axis[1].z;
			float axis21 = owner->axis[2].y;
			float axis11 = owner->axis[1].y;
			float axis02 = owner->axis[2].x;
			float axis01 = owner->axis[0].y;

			float viewX = owner->axis[0].x * offsetX + offsetY * axis01 + offsetZ * owner->axis[0].z;
			float viewY = offsetY * axis11 + owner->axis[1].x * offsetX + offsetZ * axis10;
			float viewZ = offsetZ * axis20 + offsetX * axis02 + offsetY * axis21;

			view[0].x = viewX;
			view[0].y = viewY;
			view[0].z = viewZ;

			int numAllocedVerts = std::min(9500, triangles->numVerts);
			int numAllocedIndices = std::min(29500, triangles->numIndexes);

			rvParticle* currentParticle = mUsedHead;
			int numRenderedParticles = 0;

			while (currentParticle) {
				if ((segmentTemplate->mFlags & 0x20) != 0) {
					currentParticle->mEndTime = time + 1.0;
				}

				if (triangles->numVerts + particleTemplate->GetVertexCount() > numAllocedVerts ||
					triangles->numIndexes + particleTemplate->GetIndexCount() > numAllocedIndices) {
					break;
				}

				if (!segmentTemplate->GetInverseDrawOrder()) {
					numRenderedParticles++;
					float currentRenderTime = time;
					if (currentParticle->Render(effect, particleTemplate, view, triangles, currentRenderTime, 1.0f) &&
						particleTemplate->GetTrailType() == 1) {
						currentParticle->RenderBurnTrail(effect, particleTemplate, view, triangles, time);
					}
				}

				currentParticle = currentParticle->mNext;
			}

			if (triangles->numVerts > mActiveCount * particleTemplate->GetVertexCount()) {
				common->Printf("rvSegment::Render - tri->numVerts > pt->GetVertexCount() * mActiveCount ( [%d %d] [%d %d] [%d %d] [%d %d %d] )",
					startVerts,
					startIndexes,
					triangles->numVerts,
					triangles->numIndexes,
					numRenderedParticles,
					mActiveCount,
					particleTemplate->GetIndexCount(),
					particleTemplate->GetVertexCount(),
					segmentTemplate->GetInverseDrawOrder());
			}

			if (triangles->numIndexes > mActiveCount * particleTemplate->GetIndexCount()) {
				common->Printf("rvSegment::Render - tri->numIndexes > pt->GetIndexCount() * mActiveCount ( [%d %d] [%d %d] [%d %d] [%d %d %d] )",
					startVerts,
					startIndexes,
					triangles->numVerts,
					triangles->numIndexes,
					numRenderedParticles,
					mActiveCount,
					particleTemplate->GetIndexCount(),
					particleTemplate->GetVertexCount(),
					segmentTemplate->GetInverseDrawOrder());
			}

			R_BoundTriSurf(triangles);
		}
	}
}

bool rvSegment::HandleLight(rvBSE* effect, rvSegmentTemplate* st, float time) {
	// Check if there are any used particles in the segment
	if (!this->mUsedHead) {
		return false; // Early exit if no particles are being used
	}

	// Present the light effect for the used particle
	this->mUsedHead->PresentLight(effect, &st->mParticleTemplate, time, (st->mFlags >> 5) & ~0x1);

	// Check if the light handling flag is set in the segment template's flags
	if ((st->mFlags >> 5) & 1) {
		return false; // Skip further processing if the flag is set
	}

	// Calculate adjusted time for the light effect
	float adjustedTime = this->mUsedHead->mStartTime - 0.002f;
	if (adjustedTime > time) {
		return false; // Skip if the adjusted start time is greater than the current time
	}

	// Move the used particle to the free list
	this->mFreeHead = this->mUsedHead;
	this->mUsedHead = nullptr; // Clear the used list

	return true; // Indicate that light handling was successful
}