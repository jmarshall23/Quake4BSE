// BSE_ParseParticle2.cpp
//



#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"

rvTrailInfo rvParticleTemplate::sTrailInfo;
rvElectricityInfo rvParticleTemplate::sElectricityInfo;
rvEnvParms rvParticleTemplate::sDefaultEnvelope;
rvEnvParms rvParticleTemplate::sEmptyEnvelope;
rvParticleParms rvParticleTemplate::sSPF_ONE_1;
rvParticleParms rvParticleTemplate::sSPF_ONE_2;
rvParticleParms rvParticleTemplate::sSPF_ONE_3;
rvParticleParms rvParticleTemplate::sSPF_NONE_0;
rvParticleParms rvParticleTemplate::sSPF_NONE_1;
rvParticleParms rvParticleTemplate::sSPF_NONE_3;
bool rvParticleTemplate::sInited = false;

float rvSegmentTemplate::mSegmentBaseCosts[SEG_COUNT];

void rvParticleTemplate::AllocTrail()
{
	if (mTrailInfo != NULL) {
		mTrailInfo = new rvTrailInfo();
	}	
}

bool rvParticleTemplate::ParseBlendParms(rvDeclEffect* effect, idParser* src) {
	idToken token;
	if (!src->ReadToken(&token)) {
		return false;
	}

	if (idStr::Icmp(token, "add") != 0) {
		src->Error("Invalid blend type");
		return false;
	}

	mFlags |= 0x8000u;
	return true;
}

bool rvParticleTemplate::ParseImpact(rvDeclEffect* effect, idParser* src) {
	if (!src->ExpectTokenString("{")) {
		return false;
	}

	mFlags |= 0x200u;
	idToken token;
	while (src->ReadToken(&token) && idStr::Cmp(token, "}")) {
		if (idStr::Icmp(token, "effect") == 0) {
			if (mNumImpactEffects >= 4) {
				src->Error("too many impact effects");
				return false;
			}
			mImpactEffects[mNumImpactEffects++] = declManager->FindEffect(token);
		}
		else if (idStr::Icmp(token, "remove") == 0) {
			mFlags |= src->ParseInt() ? 0x400u : 0xFFFFFBFF;
		}
		else if (idStr::Icmp(token, "bounce") == 0) {
			mBounce = src->ParseFloat();
		}
		else if (idStr::Icmp(token, "physicsDistance") == 0) {
			mPhysicsDistance = src->ParseFloat();
		}
		else {
			src->Error("Invalid impact parameter");
			return false;
		}
	}

	return true;
}
bool rvParticleTemplate::ParseTimeout(rvDeclEffect* effect, idParser* src) {
	if (!src->ExpectTokenString("{")) {
		return false;
	}

	idToken token;
	while (src->ReadToken(&token) && idStr::Cmp(token, "}")) {
		if (idStr::Icmp(token, "effect") == 0) {
			if (mNumTimeoutEffects >= 4) {
				src->Error("Too many timeout effects");
				return false;
			}
			mTimeoutEffects[mNumTimeoutEffects++] = declManager->FindEffect(token);
		}
		else {
			src->Error("Invalid timeout parameter");
			return false;
		}
	}

	return true;
}

rvEnvParms* rvParticleTemplate::ParseMotionParms(idParser* src, int count, rvEnvParms* def) {
	if (!src->ExpectTokenString("{")) {
		return def;
	}

	rvEnvParms* newParms = new rvEnvParms();
	newParms->Init();

	idToken token;
	while (src->ReadToken(&token) && idStr::Cmp(token, "}")) {
		if (idStr::Icmp(token, "envelope") == 0) {
			src->ReadToken(&token);
			newParms->mTable = declManager->FindTable(token); // Find and assign the envelope table
		}
		else if (idStr::Icmp(token, "rate") == 0) {
			src->Parse1DMatrix(count, newParms->mRate.ToFloatPtr(), true);
			newParms->mIsCount = false; // Assuming 'rate' implies not count
		}
		else if (idStr::Icmp(token, "count") == 0) {
			src->Parse1DMatrix(count, newParms->mRate.ToFloatPtr(), true);
			newParms->mIsCount = true;
		}
		else if (idStr::Icmp(token, "offset") == 0) {
			src->Parse1DMatrix(count, newParms->mEnvOffset.ToFloatPtr(), true);
		}
		else {
			src->Error("Invalid motion parameter");
			delete newParms; // Ensure to clean up allocated memory
			return def;
		}
	}

	if (newParms->Compare(*def)) {
		delete newParms; // If new parameters match the default, no need to keep them
		return def;
	}

	return newParms; // Return the new, customized parameters
}


bool rvParticleTemplate::ParseMotionDomains(rvDeclEffect* effect, idParser* src) {
	if (!src->ExpectTokenString("{")) {
		return false;
	}

	idToken token;
	while (src->ReadToken(&token)) {
		if (token == "}") {
			return true; // Successfully parsed all motion domains.
		}
		else if (token.Icmp("tint") == 0) {
			mpTintEnvelope = ParseMotionParms(src, 3, &sDefaultEnvelope);
		}
		else if (token.Icmp("fade") == 0) {
			mpFadeEnvelope = ParseMotionParms(src, 1, &sDefaultEnvelope);
		}
		else if (token.Icmp("size") == 0) {
			mpSizeEnvelope = ParseMotionParms(src, mNumSizeParms, &sDefaultEnvelope);
		}
		else if (token.Icmp("rotate") == 0) {
			mpRotateEnvelope = ParseMotionParms(src, mNumRotateParms, &sDefaultEnvelope);
		}
		else if (token.Icmp("angle") == 0) {
			mpAngleEnvelope = ParseMotionParms(src, 3, &sDefaultEnvelope);
		}
		else if (token.Icmp("offset") == 0) {
			mpOffsetEnvelope = ParseMotionParms(src, 3, &sDefaultEnvelope);
		}
		else if (token.Icmp("length") == 0) {
			mpLengthEnvelope = ParseMotionParms(src, 3, &sDefaultEnvelope);
		}
		else {
			src->Error("Invalid motion domain '%s'", token.c_str());
			src->SkipBracedSection(1); // Skip the erroneous section to continue parsing.
			return false; // Optionally return false here if you want to stop parsing at the first error.
		}
	}

	// If the loop exits without finding a closing "}", it's an error in the file structure.
	src->Error("Missing closing '}' for motion domains");
	return false;
}

void rvParticleTemplate::FixupParms(rvParticleParms* parms)
{
	if (!parms) return;

	// Simplify comparisons by reducing complex conditional checks.
	auto& maxs = parms->mMaxs;
	auto& mins = parms->mMins;
	auto type = parms->mSpawnType & 3; // Mask to get base type.
	auto modifier = parms->mSpawnType & ~3; // Mask to get modifier.

	// Early exit for certain types that don't need fixing.
	if (modifier == 0 || modifier == 4 || parms->mSpawnType == 43 || parms->mSpawnType == 47) return;

	// If mins and maxs are equal across the specified dimensions, adjust the spawn type accordingly.
	bool equalAcrossDimensions = true;
	for (int i = 0; i < type; ++i) {
		if (mins[i] != maxs[i]) {
			equalAcrossDimensions = false;
			break;
		}
	}

	if (equalAcrossDimensions) {
		if (mins[0] == 0.0f) {
			parms->mSpawnType = type;
		}
		else if (mins[0] == 1.0f) {
			parms->mSpawnType = type + 4;
		}
		else {
			parms->mSpawnType = type + 8;
		}
	}
	else if (modifier == 8) {
		parms->mSpawnType = type + 8;
	}

	// Zero out unused dimensions based on the type.
	switch (type) {
	case 1: // 1D
		mins[1] = mins[2] = maxs[1] = maxs[2] = 0.0f;
		break;
	case 2: // 2D
		mins[2] = maxs[2] = 0.0f;
		break;
	}

	// If spawn type is within a certain range, align mins and maxs.
	if (parms->mSpawnType <= 0xBu) {
		maxs = mins;
	}

	// Adjust spawn type based on flags.
	if (parms->mFlags & 2) {
		if (parms->mSpawnType <= 0xCu) parms->mSpawnType = type + 12;
	}
}

bool rvParticleTemplate::CheckCommonParms(idParser* src, rvParticleParms& parms)
{
	idToken token;
	while (src->ReadToken(&token)) {
		if (token == "}") {
			return true; // Successfully parsed common parameters.
		}
		else if (token.Icmp("surface") == 0) {
			parms.mFlags |= 1u;
		}
		else if (token.Icmp("useEndOrigin") == 0) {
			parms.mFlags |= 2u;
		}
		else if (token.Icmp("cone") == 0) {
			parms.mFlags |= 4u;
		}
		else if (token.Icmp("relative") == 0) {
			parms.mFlags |= 8u;
		}
		else if (token.Icmp("linearSpacing") == 0) {
			parms.mFlags |= 0x10u;
		}
		else if (token.Icmp("attenuate") == 0) {
			parms.mFlags |= 0x20u;
		}
		else if (token.Icmp("inverseAttenuate") == 0) {
			parms.mFlags |= 0x40u;
		}
		else {
			src->Error("Unknown parameter '%s'", token.c_str());
			return false; // Consider returning false on unknown parameters to halt parsing.
		}
	}

	src->Error("Missing closing '}' for common parameters");
	return false; // If loop exits, closing "}" wasn't found, indicating an error.
}

rvParticleParms* rvParticleTemplate::ParseSpawnParms(rvDeclEffect* effect, idParser* src, int count, rvParticleParms* def) {
	idToken token;

	if (!src->ExpectTokenString("{")) {
		return def;
	}

	if (!src->ReadToken(&token) || token == "}") {
		return def;
	}

	rvParticleParms* parms = new rvParticleParms(); // Allocate new parameters object.

	// Map geometric types to spawn types and parse relevant data.
	std::map<std::string, int> spawnTypeOffsets = {
		{"box", 16}, {"sphere", 24}, {"cylinder", 32}, {"model", 44},
		{"spiral", 40}, {"line", 12}, {"point", 8}
	};

	auto it = spawnTypeOffsets.find(token.c_str());
	if (it != spawnTypeOffsets.end()) {
		parms->mSpawnType = count + it->second;

		if (token == "model") {
			src->ReadToken(&token);
			idRenderModel* model = renderModelManager->FindModel(token);
			if (!model) {
				src->Error("Failed to load model %s\n", token.c_str());
				delete parms; // Clean up to prevent memory leak.
				return def;
			}
			parms->mModelInfo = new sdModelInfo(); // Allocate new model info.
			parms->mModelInfo->model = model;
		}
		else if (token != "point") {
			// For geometric types other than 'point', parse mins and maxs.
			src->Parse1DMatrix(count, parms->mMins.ToFloatPtr(), true);
			src->ExpectTokenString(",");
			src->Parse1DMatrix(count, parms->mMaxs.ToFloatPtr(), true);

			if (token == "spiral") {
				parms->mRange = src->ParseFloat(); // Specific to 'spiral'.
			}
		}
		else {
			// For 'point', only parse mins as it represents a single point.
			src->Parse1DMatrix(count, parms->mMins.ToFloatPtr(), true);
		}

		// Check and apply common parameters.
		if (!CheckCommonParms(src, *parms)) {
			src->Error("Invalid %s parameter!", token.c_str());
			delete parms; // Clean up to prevent memory leak.
			return def;
		}

		// Apply fixes based on the parsed parameters.
		FixupParms(parms);

		return parms;
	}
	else {
		src->Error("Unknown spawn type: %s", token.c_str());
		delete parms; // Clean up to prevent memory leak.
		return def;
	}
}

bool rvParticleTemplate::ParseSpawnDomains(rvDeclEffect* effect, idParser* src) {
	idToken token;

	src->ExpectTokenString("{");
	while (true) {
		src->ReadToken(&token);

		if (token == "}")
			break;

		if (token == "windStrength") {
			mpSpawnWindStrength = ParseSpawnParms(effect, src, 1, &rvParticleTemplate::sSPF_NONE_1);
		}
		else if (token == "length") {
			mpSpawnLength = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "offset") {
			mpSpawnOffset = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "angle") {
			mpSpawnAngle = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "rotate") {
			mpSpawnRotate = ParseSpawnParms(effect, src, mNumRotateParms, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "size") {
			mpSpawnSize = ParseSpawnParms(effect, src, mNumSizeParms, &rvParticleTemplate::sSPF_ONE_3);
		}
		else if (token == "fade") {
			mpSpawnFade = ParseSpawnParms(effect, src, 1, &rvParticleTemplate::sSPF_ONE_1);
		}
		else if (token == "tint") {
			mpSpawnTint = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_ONE_3);
		}
		else if (token == "friction") {
			mpSpawnFriction = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "acceleration") {
			mpSpawnAcceleration = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "velocity") {
			mpSpawnVelocity = ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else if (token == "direction") {
			mpSpawnDirection = rvParticleTemplate::ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
			mFlags |= 0x4000u;
		}
		else if (token == "position") {
			mpSpawnPosition = rvParticleTemplate::ParseSpawnParms(effect, src, 3, &rvParticleTemplate::sSPF_NONE_3);
		}
		else {
			src->Error("Invalid spawn type %s\n", token.c_str());
		}
	}

	return true;
}

bool rvParticleTemplate::ParseDeathDomains(rvDeclEffect* effect, idParser* src) {
	idToken token;

	src->ExpectTokenString("{");
	while (src->ReadToken(&token) && token != "}") {
		int numParms = 0;
		rvParticleParms** targetParm = nullptr;
		rvEnvParms** targetEnvelope = nullptr;

		if (token == "length") {
			numParms = 3; 
			targetParm = &mpDeathLength; 
			targetEnvelope = &mpLengthEnvelope;
		}
		else if (token == "offset") {
			numParms = 3; 
			targetParm = &mpDeathOffset; 
			targetEnvelope = &mpOffsetEnvelope;
		}
		else if (token == "angle") {
			numParms = 3; 
			targetParm = &mpDeathAngle; 
			targetEnvelope = &mpAngleEnvelope;
		}
		else if (token == "rotate") {
			numParms = mNumRotateParms; 
			targetParm = &mpDeathRotate; 
			targetEnvelope = &mpRotateEnvelope;
		}
		else if (token == "size") {
			numParms = mNumSizeParms; 
			targetParm = &mpDeathSize; 
			targetEnvelope = &mpSizeEnvelope;
		}
		else if (token == "fade") {
			numParms = 1; 
			targetParm = &mpDeathFade; 
			targetEnvelope = &mpFadeEnvelope;
		}
		else if (token == "tint") {
			numParms = 3; 
			targetParm = &mpDeathTint;
			targetEnvelope = &mpTintEnvelope;
		}
		else {
			src->Error("Invalid end type %s\n", token.c_str());
			continue;
		}

		*targetParm = ParseSpawnParms(effect, src, numParms, &rvParticleTemplate::sSPF_NONE_3);
		if (*targetEnvelope == &rvParticleTemplate::sEmptyEnvelope) {
			*targetEnvelope = &rvParticleTemplate::sDefaultEnvelope;
		}
	}

	return true;
}

bool rvParticleTemplate::Parse(rvDeclEffect* effect, idParser* src) {
	idToken token;

	src->ExpectTokenString("{");	
	while (true) {
		src->ReadToken(&token);

		if (token == "}")
			break;

		if (token == "windDeviationAngle") {
			mWindDeviationAngle = src->ParseFloat();
		}
		else if (token == "timeout") {
			ParseTimeout(effect, src);
		}
		else if (token == "impact") {
			ParseImpact(effect, src);
		}
		else if (token == "model") {
			src->ReadToken(&token);
			mModel = renderModelManager->FindModel(token);

			if (mModel == NULL) {
				mModel = renderModelManager->FindModel("_default");

				src->Warning("No surfaces defined in model %s", token.c_str());
			}
		}
		else if (token == "numFrames") {
			mNumFrames = src->ParseInt();
		}
		else if (token == "fadeIn") {
			// TODO  v3->mFlags |= (unsigned int)&vwin8192[2696]; <-- garbage.
		}
		else if (token == "useLightningAxis") {
			mFlags |= 0x400000u;
		}
		else if (token == "specular") {
			mFlags |= 0x40000u;
		}
		else if (token == "shadows") {
			mFlags |= 0x20000u;
		}
		else if (token == "blend") {
			ParseBlendParms(effect, src);
		}
		else if (token == "entityDef") {
			src->ReadToken(&token);
			mEntityDefName = token;
		}
		else if (token == "material") {
			src->ReadToken(&token);
			mMaterial = declManager->FindMaterial(token);
		}
		else if (token == "trailScale") {
			AllocTrail();
			mTrailInfo->mTrailScale = src->ParseFloat();
		}
		else if (token == "trailCount") {
			AllocTrail();
			mTrailInfo->mTrailCount.x = src->ParseFloat();
			src->ExpectTokenString(",");
			mTrailInfo->mTrailCount.y = src->ParseFloat();
		}
		else if (token == "trailRepeat") {
			AllocTrail();
			mTrailRepeat = src->ParseInt();
		}
		else if (token == "trailTime") {
			AllocTrail();
			mTrailInfo->mTrailTime.x = src->ParseFloat();
			src->ExpectTokenString(",");
			mTrailInfo->mTrailTime.y = src->ParseFloat();
		}
		else if (token == "trailMaterial") {
			AllocTrail();
			src->ReadToken(&token);
			mTrailInfo->mTrailMaterial = declManager->FindMaterial(token);
		}
		else if (token == "trailType") {
			src->ReadToken(&token);

			if (token == "burn") {
				mTrailInfo->mTrailType = 1;
			}
			else if (token == "motion") {
				mTrailInfo->mTrailType = 2;
			}
			else {
				mTrailInfo->mTrailType = 3;
				mTrailInfo->mTrailTypeName = token;
			}
		}
		else if (token == "gravity") {
			mGravity.x = src->ParseFloat();
			src->ExpectTokenString(",");
			mGravity.y = src->ParseFloat();
		}
		else if (token == "duration") {
			float srcb = src->ParseFloat();
			float v8 = 0.0020000001;
			if (srcb >= 0.0020000001)
			{
				v8 = srcb;
				if (srcb > 300.0)
					v8 = 300.0;
			}
			float srcg = v8;
			mDuration.x = srcg;
			src->ExpectTokenString(",");

			float srcc = src->ParseFloat();
			float v9 = 0.0020000001;
			if (srcc < 0.0020000001 || (v9 = srcc, srcc <= 300.0))
			{
				float srch = v9;
				mDuration.y = srch;
			}
			else
			{
				mDuration.y = 300.0;
			}
		}
		else if (token == "parentvelocity") {
			mFlags |= 0x2000000u;
		}
		else if (token == "tiling") {
			mFlags |= 0x100000u;
			float srca = src->ParseFloat();
			float v7 = 0.0020000001;
			if (srca < 0.0020000001 || (v7 = srca, srca <= 1024.0))
			{
				float srcf = v7;
				mTiling = srcf;
			}
			else
			{
				mTiling = 1024.0;
			}
		}
		else if (token == "persist") {
			mFlags |= 0x200000u;
		}
		else if (token == "generatedLine") {
			mFlags |= 0x10000u;
		}
		else if (token == "flipNormal") {
			mFlags |= 0x2000u;
		}
		else if (token == "lineHit") {
			mFlags |= 0x4000000u;
		}
		else if (token == "generatedOriginNormal") {
			mFlags |= 0x1000u;
		}
		else if (token == "generatedNormal") {
			mFlags |= 0x800u;
		}
		else if (token == "motion") {
			ParseMotionDomains(effect, src);
		}
		else if (token == "end") {
			ParseDeathDomains(effect, src);
		}
		else if (token == "start") {
			ParseSpawnDomains(effect, src);
		}
		else {
			src->Error("Invalid particle keyword %s\n", token.c_str());
		}
	}

	Finish();

	return true;
}

void rvParticleTemplate::Duplicate(rvParticleTemplate const& copy) {

}
void rvParticleTemplate::Finish() {
	double v2; // st7
	rvParticleTemplate* v3; // esi
	rvTrailInfo* v4; // eax
	float* v5; // eax
	const modelSurface_t* v6; // eax
	const modelSurface_t* v7; // ebp
	idTraceModel* v8; // eax
	idTraceModel* v9; // edi
	idBounds* v10; // ebp
	rvTrailInfo* v11; // ecx
	rvElectricityInfo* v12; // eax
	float v13; // ST10_4
	float v14; // ST10_4
	rvTrailInfo* v15; // ecx
	float v16; // ST10_4
	rvTrailInfo* v17; // ecx
	double v18; // st6
	float* v19; // ecx
	float v20; // ST10_4
	float* v21; // eax
	float v22; // ST14_4
	float v23; // ST18_4
	float v24; // ST1C_4
	float v25; // ST20_4
	float v26; // ST24_4
	float v27; // ST28_4
	signed int retaddr; // [esp+2Ch] [ebp+0h]

	v2 = 0.0;
	v3 = this;
	v3->mFlags |= 0x100u;
	v4 = this->mTrailInfo;
	if ((!v4->mTrailType || v4->mTrailType == 3) && !v4->mStatic)
	{
		v4->mTrailTime.y = 0.0;
		v4->mTrailTime.x = 0.0;
		v5 = &this->mTrailInfo->mTrailCount.x;
		v5[1] = 0.0;
		*v5 = 0.0;
	}
	switch (this->mType)
	{
	case 1:
	case 2:
		v11 = this->mTrailInfo;
		v3->mVertexCount = 4;
		v3->mIndexCount = 6;
		if (0.0 != v11->mTrailCount.y && v11->mTrailType == 1)
		{
			v3->mVertexCount *= (unsigned __int16)v3->GetMaxTrailCount();
			v2 = 0.0;
			v3->mIndexCount *= (unsigned __int16)v3->GetMaxTrailCount();
		}
		break;
	case 4:
	case 6:
	case 8:
	case 9:
		this->mVertexCount = 4;
		this->mIndexCount = 6;
		break;
	case 5:
		v6 = this->mModel->Surface(0);
		v7 = v6;
		if (v6)
		{
			v3->mVertexCount = v6->geometry->numVerts;// *(_WORD*)(*(_DWORD*)(v6 + 8) + 48);
			v3->mIndexCount = v6->geometry->numIndexes; // *(_WORD*)(*(_DWORD*)(v6 + 8) + 56);
		}
		v3->mMaterial = *(idMaterial**)(v6 + 4);
		v3->PurgeTraceModel();
		v8 = (idTraceModel*)operator new(0xB4Cu);
		v9 = v8;
		retaddr = 0;
		if (v8)
		{
			v10 = *(idBounds**)(v7 + 8);
			v8->InitBox();
			v9->SetupBox(*v10);
		}
		retaddr = -1;
		v2 = 0.0;
		v3->mTraceModelIndex = bse->AddTraceModel(v8);
		break;
	case 7:
		v12 = this->mElecInfo;
		this->mVertexCount = 20 * (LOWORD(v12->mNumForks) + 1);
		this->mIndexCount = 60 * (LOWORD(v12->mNumForks) + 1);
		break;
	case 0xA:
		this->mVertexCount = 0;
		this->mIndexCount = 0;
		break;
	default:
		break;
	}
	if (v3->mDuration.y <= (double)v3->mDuration.x)
	{
		v13 = v3->mDuration.x;
		v3->mDuration.x = v3->mDuration.y;
		v3->mDuration.y = v13;
	}
	if (v3->mGravity.y <= (double)v3->mGravity.x)
	{
		v14 = v3->mGravity.x;
		v3->mGravity.x = v3->mGravity.y;
		v3->mGravity.y = v14;
	}
	v15 = v3->mTrailInfo;
	if (!v15->mStatic)
	{
		if (v15->mTrailTime.y <= (double)v15->mTrailTime.x)
		{
			v16 = v15->mTrailTime.x;
			v15->mTrailTime.x = v15->mTrailTime.y;
			v15->mTrailTime.y = v16;
		}
		v17 = v3->mTrailInfo;
		v18 = v17->mTrailCount.x;
		v19 = &v17->mTrailCount.x;
		if (v19[1] <= v18)
		{
			v20 = *v19;
			*v19 = v19[1];
			v19[1] = v20;
		}
	}
	v3->mCentre.z = v2;
	v3->mCentre.y = v2;
	v3->mCentre.x = v2;
	if (!(((unsigned int)v3->mFlags >> 12) & 1))
	{
		v21 = (float*)&v3->mpSpawnPosition->mSpawnType;
		switch (*(unsigned __int8*)v21)
		{
		case 0xBu:
			v3->mCentre.x = v21[3];
			v3->mCentre.y = v21[4];
			v3->mCentre.z = v21[5];
			break;
		case 0xFu:
		case 0x13u:
		case 0x17u:
		case 0x1Bu:
		case 0x1Fu:
		case 0x23u:
		case 0x27u:
		case 0x2Bu:
		case 0x2Fu:
			v22 = v21[6] + v21[3];
			v23 = v21[7] + v21[4];
			v24 = v21[8] + v21[5];
			v25 = v22 * 0.5;
			v26 = v23 * 0.5;
			v27 = 0.5 * v24;
			v3->mCentre.x = v25;
			v3->mCentre.y = v26;
			v3->mCentre.z = v27;
			break;
		default:
			return;
		}
	}
}

void rvParticleTemplate::InitStatic()
{
	if (!rvParticleTemplate::sInited) {
		rvParticleTemplate::sInited = true;
		rvParticleTemplate::sTrailInfo.mTrailType = 0;

		// Initialize sElectricityInfo
		rvParticleTemplate::sElectricityInfo.mNumForks = 0;
		rvParticleTemplate::sElectricityInfo.mStatic = true;
		rvParticleTemplate::sElectricityInfo.mForkSizeMins = idVec3(-20.0, -20.0, -20.0);
		rvParticleTemplate::sElectricityInfo.mForkSizeMaxs = idVec3(20.0, 20.0, 20.0);
		rvParticleTemplate::sElectricityInfo.mJitterSize = idVec3(3.0, 7.0, 7.0);
		rvParticleTemplate::sElectricityInfo.mJitterRate = 0.0;
		rvParticleTemplate::sElectricityInfo.mJitterTable = declManager->FindTable("halfsintable", false);

		rvParticleTemplate::sDefaultEnvelope.Init();
		rvParticleTemplate::sDefaultEnvelope.SetDefaultType();
		rvParticleTemplate::sDefaultEnvelope.mStatic = true;

		rvParticleTemplate::sEmptyEnvelope.Init();
		rvParticleTemplate::sEmptyEnvelope.mStatic = true;

		// Initialize SPF (Spawn Parameter Flags) structures with default values
		rvParticleParms spfOne1, spfOne2, spfOne3, spfNone0, spfNone1, spfNone3;

		spfOne1.mRange = 0.0;
		spfOne1.mMins = idVec3(0.0, 0.0, 0.0);
		spfOne1.mMaxs = idVec3(0.0, 0.0, 0.0);
		spfOne1.mSpawnType = 5;
		spfOne1.mFlags = 0;
		spfOne1.mModelInfo = 0;
		spfOne1.mStatic = true;
		rvParticleTemplate::sSPF_ONE_1 = spfOne1;

		spfOne2.mSpawnType = 6;
		spfOne2.mFlags = 0;
		spfOne2.mRange = 0.0;
		spfOne2.mModelInfo = 0;
		spfOne2.mMins = idVec3(0.0, 0.0, 0.0);
		spfOne2.mMaxs = idVec3(0.0, 0.0, 0.0);
		spfOne2.mStatic = true;
		rvParticleTemplate::sSPF_ONE_2 = spfOne2;

		spfOne3.mSpawnType = 7;
		spfOne3.mFlags = 0;
		spfOne3.mRange = 0.0;
		spfOne3.mModelInfo = 0;
		spfOne3.mMins = idVec3(0.0, 0.0, 0.0);
		spfOne3.mMaxs = idVec3(0.0, 0.0, 0.0);
		spfOne3.mStatic = true;
		rvParticleTemplate::sSPF_ONE_3 = spfOne3;

		spfNone0.mSpawnType = 0;
		spfNone0.mFlags = 0;
		spfNone0.mModelInfo = 0;
		spfNone0.mRange = 0.0;
		spfNone0.mMins = idVec3(0.0, 0.0, 0.0);
		spfNone0.mMaxs = idVec3(0.0, 0.0, 0.0);
		spfNone0.mStatic = true;
		rvParticleTemplate::sSPF_NONE_0 = spfNone0;

		spfNone1.mSpawnType = 1;
		spfNone1.mFlags = 0;
		spfNone1.mModelInfo = 0;
		spfNone1.mRange = 0.0;
		spfNone1.mMins = idVec3(0.0, 0.0, 0.0);
		spfNone1.mMaxs = idVec3(0.0, 0.0, 0.0);
		spfNone1.mStatic = true;
		rvParticleTemplate::sSPF_NONE_1 = spfNone1;

		spfNone3.mSpawnType = 3;
		spfNone3.mFlags = 0;
		spfNone3.mModelInfo = 0;
		spfNone3.mRange = 0.0;
		spfNone3.mMins = idVec3(0.0, 0.0, 0.0);
		spfNone3.mMaxs = idVec3(0.0, 0.0, 0.0);
		spfNone3.mStatic = true;
		rvParticleTemplate::sSPF_NONE_3 = spfNone3;
	}
}
void rvParticleTemplate::Init() {
	idVec3 vec3_zero;
	vec3_zero.Zero();

	// Initialize static configurations if not already done.
	InitStatic();

	// Reset all member variables to default states.
	mFlags = 0x4000000u | 0x8000000u; // Assuming these flags are default states.
	mType = 0;
	SetMaterial((idMaterial *)declManager->FindMaterial("_default"));
	mModel = renderModelManager->FindModel("_default");

	// Initialize simple numerical members with default values.
	mTraceModelIndex = -1;
	mGravity = idVec2(0, 0);
	mTiling = 8.0f;
	mPhysicsDistance = 0.0f;
	mBounce = 0.0f;
	mDuration = idVec2(0.0020000001f, 0.0020000001f);
	mCentre = vec3_zero;

	// Pointers to parameter defaults.
	mpSpawnPosition = mpSpawnDirection = mpSpawnVelocity = mpSpawnAcceleration = mpSpawnFriction =
		mpSpawnRotate = mpSpawnAngle = mpSpawnOffset = mpSpawnLength = mpDeathTint = mpDeathRotate =
		mpDeathAngle = mpDeathOffset = mpDeathLength = &rvParticleTemplate::sSPF_NONE_3;

	// Other initializations.
	mNumSizeParms = 2;
	mNumRotateParms = 1;
	mVertexCount = 4;
	mIndexCount = 6;
	mTrailInfo = &rvParticleTemplate::sTrailInfo;
	mElecInfo = &rvParticleTemplate::sElectricityInfo;

	// Default parameter pointers.
	mpSpawnTint = mpSpawnFade = mpSpawnSize = &rvParticleTemplate::sSPF_ONE_3;
	mpSpawnWindStrength = &rvParticleTemplate::sSPF_NONE_1;
	mpTintEnvelope = mpFadeEnvelope = mpSizeEnvelope = mpRotateEnvelope = mpAngleEnvelope = mpOffsetEnvelope = mpLengthEnvelope = &rvParticleTemplate::sEmptyEnvelope;

	mpDeathFade = &rvParticleTemplate::sSPF_NONE_1;
	mpDeathSize = &rvParticleTemplate::sSPF_ONE_3;

	mTrailRepeat = 1;
	mNumFrames = mNumImpactEffects = mNumTimeoutEffects = 0;
}

void rvParticleTemplate::SetParameterCounts() {
	static const std::map<int, std::pair<int, int>> typeMappings = {
		{1, {2, 1}},
		{2, {1, 0}},
		{7, {1, 0}},
		{8, {1, 0}},
		{9, {1, 0}},
		{3, {2, 3}},
		{4, {3, 1}},
		{5, {3, 3}},
		{6, {3, 0}},
		{0xA, {0, 3}}
	};

	auto it = typeMappings.find(mType);
	if (it != typeMappings.end()) {
		mNumSizeParms = it->second.first;
		mNumRotateParms = it->second.second;
	}

	// Set default parameter pointers based on size and rotate params.
	mpSpawnSize = (mNumSizeParms > 1) ? &rvParticleTemplate::sSPF_ONE_3 : &rvParticleTemplate::sSPF_ONE_1;
	mpSpawnRotate = (mNumRotateParms > 0) ? &rvParticleTemplate::sSPF_NONE_3 : &rvParticleTemplate::sSPF_NONE_0;
	mpDeathSize = mpSpawnSize;
	mpDeathRotate = mpSpawnRotate;
}

void rvParticleTemplate::PurgeTraceModel(void) {
	if (mTraceModelIndex != -1) {
		// bse->FreeTraceModel(mTraceModelIndex); // Assuming this is a function call to release the model.
		mTraceModelIndex = -1;
	}
}

void rvParticleTemplate::Purge() {
	// TODO
}

float rvParticleTemplate::CostTrail(float cost) const
{
	const rvTrailInfo* info = mTrailInfo;
	switch (info->mTrailType) {
		case 1: return info->mTrailCount.y * (cost * 2);
		case 2: return info->mTrailCount.y * (cost * 1.5f) + 20.0f;
		default: 
			return cost;
	}
}

bool rvParticleTemplate::UsesEndOrigin(void) {
	return (mpSpawnPosition->mFlags & 2) || (mpSpawnLength->mFlags & 2) || ((mFlags >> 22) & 1);
}
