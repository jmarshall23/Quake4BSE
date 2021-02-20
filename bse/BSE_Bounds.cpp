// BSE_Bounds.cpp
//

#pragma hdrstop
#include "precompiled.h"

#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"


float rvParticleTemplate::GetMaxParmValue(rvParticleParms& spawn, rvParticleParms& death, rvEnvParms& envelope)
{
	double result; // st7
	idBounds sbounds; // [esp+4h] [ebp-34h] BYREF
	idBounds dbounds; // [esp+1Ch] [ebp-1Ch] BYREF
	float v7; // [esp+34h] [ebp-4h]

	spawn.GetMinsMaxs(sbounds[0], sbounds[1]);
	if (envelope.GetMinMax(sbounds[0][0], spawn.mRange)) // jmarshall not sure
	{
		sbounds[0].y = sbounds[0].y * sbounds[0].x;
		sbounds[0].z = sbounds[0].z * sbounds[0].x;
		sbounds[1].x = sbounds[0].x * sbounds[1].x;
		sbounds[1].y = sbounds[1].y * spawn.mRange;
		sbounds[1].z = sbounds[1].z * spawn.mRange;
		dbounds[0].x = *(float*)&spawn * dbounds[0].x;
		death.GetMinsMaxs(dbounds[0], dbounds[1]);
		envelope.mEnvOffset = dbounds[0]; // (rvEnvParms*)&dbounds[0].y;
		death.mRange = sbounds[0].y;
		dbounds[0].y = dbounds[0].y * sbounds[0].x;
		dbounds[0].z = dbounds[0].z * sbounds[0].x;
		dbounds[1].x = sbounds[0].x * dbounds[1].x;
		dbounds[1].y = dbounds[1].y * spawn.mRange;
		dbounds[1].z = dbounds[1].z * spawn.mRange;
		v7 = spawn.mRange; // *(float*)&spawn* v7;
		sbounds[0].y = fminf(sbounds[0].y, dbounds[0].y);
		sbounds[0].z = fminf(sbounds[0].z, dbounds[0].z);
		sbounds[1].x = fminf(sbounds[1].x, dbounds[1].x);
		sbounds[1].y = fmaxf(sbounds[1].y, dbounds[1].y);
		sbounds[1].z = fmaxf(sbounds[1].z, dbounds[1].z);
		dbounds[0].x = fmaxf(dbounds[0].x, v7);
	}
	spawn.mRange = sbounds[1].x * sbounds[1].x + sbounds[0].y * sbounds[0].y + sbounds[0].z * sbounds[0].z;
	spawn.mRange = sqrt(spawn.mRange);
	sbounds[0].x = spawn.mRange;
	spawn.mRange = dbounds[0].x * dbounds[0].x + sbounds[1].y * sbounds[1].y + sbounds[1].z * sbounds[1].z;
	spawn.mRange = sqrt(spawn.mRange);
	result = spawn.mRange;
	if (sbounds[0].x >= spawn.mRange)
		result = sbounds[0].x;
	return result;
}


void rvParticleTemplate::EvaluateSimplePosition(idVec3& pos, float time, float lifeTime, idVec3& initPos, idVec3& velocity, idVec3& acceleration, idVec3& friction)
{
	float timea; // [esp+8h] [ebp+8h]
	float timeb; // [esp+8h] [ebp+8h]
	float velocitya; // [esp+14h] [ebp+14h]
	float velocityb; // [esp+14h] [ebp+14h]

	pos.x = velocity.x * time + initPos.x;
	pos.y = velocity.y * time + initPos.y;
	pos.z = velocity.z * time + initPos.z;
	timea = time * (0.5 * time);
	pos.x = acceleration.x * timea + pos.x;
	pos.y = acceleration.y * timea + pos.y;
	pos.z = acceleration.z * timea + pos.z;
	velocitya = (lifeTime - timea) / lifeTime;
	velocityb = exp(velocitya);
	timeb = (velocityb - 1.0) * (0.3333333432674408 * timea) * timea;
	pos.x = friction.x * timeb + pos.x;
	pos.y = friction.y * timeb + pos.y;
	pos.z = timeb * friction.z + pos.z;
}

float rvParticleTemplate::GetFurthestDistance()
{
/*
	float v2; // ebp correct
			  //     
	idBounds* v3; // esi
	float* v4; // edi
	float v5; // xmm1_4
	float v6; // xmm3_4
	float v7; // xmm5_4
	float* v8; // esi
	float* v9; // edi
	float v10; // xmm1_4
	float v11; // xmm3_4
	float v12; // xmm5_4
	float* v13; // esi
	float* v14; // edi
	float v15; // xmm1_4
	float v16; // xmm3_4
	float v17; // xmm5_4
	float* v18; // esi
	float* v19; // edi
	float v20; // xmm1_4
	float v21; // xmm3_4
	float v22; // xmm5_4
	float* v23; // esi
	float* v24; // edi
	float v25; // xmm1_4
	float v26; // xmm3_4
	float v27; // xmm5_4
	float* v28; // esi
	float* v29; // edi
	float v30; // xmm1_4
	float v31; // xmm3_4
	float v32; // xmm5_4
	float* v33; // esi
	float* v34; // edi
	float v35; // xmm1_4
	float v36; // xmm3_4
	float v37; // xmm5_4
	float* v38; // esi
	float* v39; // edi
	float v40; // xmm1_4
	float v41; // xmm3_4
	float v42; // xmm5_4
	float* v43; // esi
	float* v44; // edi
	float v45; // xmm1_4
	float v46; // xmm3_4
	float v47; // xmm5_4
	float* v48; // esi
	float* v49; // edi
	float v50; // xmm1_4
	float v51; // xmm3_4
	float v52; // xmm5_4
	float* v53; // esi
	float* v54; // edi
	float v55; // xmm1_4
	float v56; // xmm3_4
	float v57; // xmm5_4
	float* v58; // esi
	float* v59; // edi
	float v60; // xmm1_4
	float v61; // xmm3_4
	float v62; // xmm5_4
	float* v63; // esi
	float* v64; // edi
	float v65; // xmm1_4
	float v66; // xmm3_4
	float v67; // xmm5_4
	float* v68; // esi
	float* v69; // edi
	float v70; // xmm1_4
	float v71; // xmm3_4
	float v72; // xmm5_4
	float* v73; // esi
	float* v74; // edi
	float v75; // xmm1_4
	float v76; // xmm3_4
	float v77; // xmm5_4
	float v78; // esi
	float v79; // edi
	float v80; // xmm1_4
	float v81; // xmm3_4
	float v82; // xmm5_4
	double v83; // st7
	idVec3 pos; // [esp+24h] [ebp-124h] BYREF
	float max; // [esp+34h] [ebp-114h]
	idBounds result; // [esp+38h] [ebp-110h] BYREF
	idBounds accel; // [esp+50h] [ebp-F8h] BYREF
	idBounds friction; // [esp+68h] [ebp-E0h] BYREF
	idBounds spawn; // [esp+80h] [ebp-C8h] BYREF
	idBounds velocity; // [esp+98h] [ebp-B0h] BYREF
	idBounds offset; // [esp+B0h] [ebp-98h] BYREF
	float v92; // [esp+CCh] [ebp-7Ch]
	float* v93; // [esp+D0h] [ebp-78h]
	float* v94; // [esp+D4h] [ebp-74h]
	float* v95; // [esp+D8h] [ebp-70h]
	float* v96; // [esp+DCh] [ebp-6Ch]
	float* v97; // [esp+E0h] [ebp-68h]
	float* v98; // [esp+E4h] [ebp-64h]
	float* v99; // [esp+E8h] [ebp-60h]
	float* v100; // [esp+ECh] [ebp-5Ch]
	float* v101; // [esp+F0h] [ebp-58h]
	float* v102; // [esp+F4h] [ebp-54h]
	float* v103; // [esp+F8h] [ebp-50h]
	float* v104; // [esp+FCh] [ebp-4Ch]
	float* v105; // [esp+100h] [ebp-48h]
	idBounds* v106; // [esp+104h] [ebp-44h]
	float* v107; // [esp+108h] [ebp-40h]
	float* v108; // [esp+10Ch] [ebp-3Ch]
	float* v109; // [esp+110h] [ebp-38h]
	float* v110; // [esp+114h] [ebp-34h]
	float* v111; // [esp+118h] [ebp-30h]
	float* v112; // [esp+11Ch] [ebp-2Ch]
	float* v113; // [esp+120h] [ebp-28h]
	float* v114; // [esp+124h] [ebp-24h]
	float* v115; // [esp+128h] [ebp-20h]
	float* v116; // [esp+12Ch] [ebp-1Ch]
	float* v117; // [esp+130h] [ebp-18h]
	float* v118; // [esp+134h] [ebp-14h]
	float* v119; // [esp+138h] [ebp-10h]
	float* v120; // [esp+13Ch] [ebp-Ch]
	float* v121; // [esp+140h] [ebp-8h]
	float* v122; // [esp+144h] [ebp-4h]

	mpSpawnPosition->GetMinsMaxs((idVec3*)&spawn.b[0].y, (idVec3*)&spawn.b[1].y);
	GetMinsMaxs(this->mpSpawnVelocity, (idVec3*)&velocity.b[0].y, (idVec3*)&velocity.b[1].y);
	GetMinsMaxs(this->mpSpawnAcceleration, (idVec3*)&accel.b[0].y, (idVec3*)&accel.b[1].y);
	GetMinsMaxs(this->mpSpawnFriction, (idVec3*)&friction.b[0].y, (idVec3*)&friction.b[1].y);
	GetMinsMaxs(this->mpSpawnOffset, (idVec3*)&offset.b[0].y, (idVec3*)&offset.b[1].y);
	accel.b[0].x = 0.0;
	result.b[1].z = 0.0;
	v103 = &pos.y;
	result.b[1].y = 0.0;
	result.b[1].x = 0.0;
	result.b[0].z = 0.0;
	v106 = (idBounds*)&result.b[0].y;
	result.b[0].y = 0.0;
	v108 = &pos.y;
	v110 = &result.b[0].y;
	v112 = &pos.y;
	v114 = &result.b[0].y;
	v116 = &pos.y;
	v118 = &result.b[0].y;
	v120 = &pos.y;
	v122 = &result.b[0].y;
	v95 = &pos.y;
	v109 = &result.b[0].y;
	v97 = &pos.y;
	v119 = &result.b[0].y;
	v99 = &pos.y;
	v2 = 0.0;
	v111 = &result.b[0].y;
	max = 0.0;
	v101 = &pos.y;
	v117 = &result.b[0].y;
	v93 = &pos.y;
	v113 = &result.b[0].y;
	v105 = &pos.y;
	v121 = &result.b[0].y;
	v107 = &pos.y;
	v115 = &result.b[0].y;
	v94 = &pos.y;
	v96 = &result.b[0].y;
	v98 = &pos.y;
	v100 = &result.b[0].y;
	v102 = &pos.y;
	v104 = &result.b[0].y;
	v92 = COERCE_FLOAT((idVec3*)&pos.y);
	LODWORD(result.b[0].x) = &result.b[0].y;
	do
	{
		max = (double)SLODWORD(max) * this->mDuration.y * 0.125;
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[0].y,
			(idVec3*)&velocity.b[0].y,
			(idVec3*)&accel.b[0].y,
			(idVec3*)&friction.b[0].y);
		v3 = v106;
		v4 = v103;
		v5 = *v103;
		v106->b[0].x = fminf(v106->b[0].x, *v103);
		v6 = v4[1];
		v3->b[0].y = fminf(v3->b[0].y, v6);
		v7 = v4[2];
		v3->b[0].z = fminf(v3->b[0].z, v7);
		v3->b[1].x = fmaxf(v5, v3->b[1].x);
		v3->b[1].y = fmaxf(v6, v3->b[1].y);
		v3->b[1].z = fmaxf(v7, v3->b[1].z);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[0].y,
			(idVec3*)&velocity.b[0].y,
			(idVec3*)&accel.b[0].y,
			(idVec3*)&friction.b[1].y);
		v8 = v110;
		v9 = v108;
		v10 = *v108;
		*v110 = fminf(*v110, *v108);
		v11 = v9[1];
		v8[1] = fminf(v8[1], v11);
		v12 = v9[2];
		v8[2] = fminf(v8[2], v12);
		v8[3] = fmaxf(v10, v8[3]);
		v8[4] = fmaxf(v11, v8[4]);
		v8[5] = fmaxf(v12, v8[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[0].y,
			(idVec3*)&velocity.b[0].y,
			(idVec3*)&accel.b[1].y,
			(idVec3*)&friction.b[0].y);
		v13 = v114;
		v14 = v112;
		v15 = *v112;
		*v114 = fminf(*v114, *v112);
		v16 = v14[1];
		v13[1] = fminf(v13[1], v16);
		v17 = v14[2];
		v13[2] = fminf(v13[2], v17);
		v13[3] = fmaxf(v15, v13[3]);
		v13[4] = fmaxf(v16, v13[4]);
		v13[5] = fmaxf(v17, v13[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[0].y,
			(idVec3*)&velocity.b[0].y,
			(idVec3*)&accel.b[1].y,
			(idVec3*)&friction.b[1].y);
		v18 = v118;
		v19 = v116;
		v20 = *v116;
		*v118 = fminf(*v118, *v116);
		v21 = v19[1];
		v18[1] = fminf(v18[1], v21);
		v22 = v19[2];
		v18[2] = fminf(v18[2], v22);
		v18[3] = fmaxf(v20, v18[3]);
		v18[4] = fmaxf(v21, v18[4]);
		v18[5] = fmaxf(v22, v18[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[0].y,
			(idVec3*)&velocity.b[1].y,
			(idVec3*)&accel.b[0].y,
			(idVec3*)&friction.b[0].y);
		v23 = v122;
		v24 = v120;
		v25 = *v120;
		*v122 = fminf(*v122, *v120);
		v26 = v24[1];
		v23[1] = fminf(v23[1], v26);
		v27 = v24[2];
		v23[2] = fminf(v23[2], v27);
		v23[3] = fmaxf(v25, v23[3]);
		v23[4] = fmaxf(v26, v23[4]);
		v23[5] = fmaxf(v27, v23[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[0].y,
			(idVec3*)&velocity.b[1].y,
			(idVec3*)&accel.b[0].y,
			(idVec3*)&friction.b[1].y);
		v28 = v109;
		v29 = v95;
		v30 = *v95;
		*v109 = fminf(*v109, *v95);
		v31 = v29[1];
		v28[1] = fminf(v28[1], v31);
		v32 = v29[2];
		v28[2] = fminf(v28[2], v32);
		v28[3] = fmaxf(v30, v28[3]);
		v28[4] = fmaxf(v31, v28[4]);
		v28[5] = fmaxf(v32, v28[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[0].y,
			(idVec3*)&velocity.b[1].y,
			(idVec3*)&accel.b[1].y,
			(idVec3*)&friction.b[0].y);
		v33 = v119;
		v34 = v97;
		v35 = *v97;
		*v119 = fminf(*v119, *v97);
		v36 = v34[1];
		v33[1] = fminf(v33[1], v36);
		v37 = v34[2];
		v33[2] = fminf(v33[2], v37);
		v33[3] = fmaxf(v35, v33[3]);
		v33[4] = fmaxf(v36, v33[4]);
		v33[5] = fmaxf(v37, v33[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[0].y,
			(idVec3*)&velocity.b[1].y,
			(idVec3*)&accel.b[1].y,
			(idVec3*)&friction.b[1].y);
		v38 = v111;
		v39 = v99;
		v40 = *v99;
		*v111 = fminf(*v111, *v99);
		v41 = v39[1];
		v38[1] = fminf(v38[1], v41);
		v42 = v39[2];
		v38[2] = fminf(v38[2], v42);
		v38[3] = fmaxf(v40, v38[3]);
		v38[4] = fmaxf(v41, v38[4]);
		v38[5] = fmaxf(v42, v38[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[1].y,
			(idVec3*)&velocity.b[0].y,
			(idVec3*)&accel.b[0].y,
			(idVec3*)&friction.b[0].y);
		v43 = v117;
		v44 = v101;
		v45 = *v101;
		*v117 = fminf(*v117, *v101);
		v46 = v44[1];
		v43[1] = fminf(v43[1], v46);
		v47 = v44[2];
		v43[2] = fminf(v43[2], v47);
		v43[3] = fmaxf(v45, v43[3]);
		v43[4] = fmaxf(v46, v43[4]);
		v43[5] = fmaxf(v47, v43[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[1].y,
			(idVec3*)&velocity.b[0].y,
			(idVec3*)&accel.b[0].y,
			(idVec3*)&friction.b[1].y);
		v48 = v113;
		v49 = v93;
		v50 = *v93;
		*v113 = fminf(*v113, *v93);
		v51 = v49[1];
		v48[1] = fminf(v48[1], v51);
		v52 = v49[2];
		v48[2] = fminf(v48[2], v52);
		v48[3] = fmaxf(v50, v48[3]);
		v48[4] = fmaxf(v51, v48[4]);
		v48[5] = fmaxf(v52, v48[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[1].y,
			(idVec3*)&velocity.b[0].y,
			(idVec3*)&accel.b[1].y,
			(idVec3*)&friction.b[0].y);
		v53 = v121;
		v54 = v105;
		v55 = *v105;
		*v121 = fminf(*v121, *v105);
		v56 = v54[1];
		v53[1] = fminf(v53[1], v56);
		v57 = v54[2];
		v53[2] = fminf(v53[2], v57);
		v53[3] = fmaxf(v55, v53[3]);
		v53[4] = fmaxf(v56, v53[4]);
		v53[5] = fmaxf(v57, v53[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[1].y,
			(idVec3*)&velocity.b[0].y,
			(idVec3*)&accel.b[1].y,
			(idVec3*)&friction.b[1].y);
		v58 = v115;
		v59 = v107;
		v60 = *v107;
		*v115 = fminf(*v115, *v107);
		v61 = v59[1];
		v58[1] = fminf(v58[1], v61);
		v62 = v59[2];
		v58[2] = fminf(v58[2], v62);
		v58[3] = fmaxf(v60, v58[3]);
		v58[4] = fmaxf(v61, v58[4]);
		v58[5] = fmaxf(v62, v58[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[1].y,
			(idVec3*)&velocity.b[1].y,
			(idVec3*)&accel.b[0].y,
			(idVec3*)&friction.b[0].y);
		v63 = v96;
		v64 = v94;
		v65 = *v94;
		*v96 = fminf(*v96, *v94);
		v66 = v64[1];
		v63[1] = fminf(v63[1], v66);
		v67 = v64[2];
		v63[2] = fminf(v63[2], v67);
		v63[3] = fmaxf(v65, v63[3]);
		v63[4] = fmaxf(v66, v63[4]);
		v63[5] = fmaxf(v67, v63[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[1].y,
			(idVec3*)&velocity.b[1].y,
			(idVec3*)&accel.b[0].y,
			(idVec3*)&friction.b[1].y);
		v68 = v100;
		v69 = v98;
		v70 = *v98;
		*v100 = fminf(*v100, *v98);
		v71 = v69[1];
		v68[1] = fminf(v68[1], v71);
		v72 = v69[2];
		v68[2] = fminf(v68[2], v72);
		v68[3] = fmaxf(v70, v68[3]);
		v68[4] = fmaxf(v71, v68[4]);
		v68[5] = fmaxf(v72, v68[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[1].y,
			(idVec3*)&velocity.b[1].y,
			(idVec3*)&accel.b[1].y,
			(idVec3*)&friction.b[0].y);
		v73 = v104;
		v74 = v102;
		v75 = *v102;
		*v104 = fminf(*v104, *v102);
		v76 = v74[1];
		v73[1] = fminf(v73[1], v76);
		v77 = v74[2];
		v73[2] = fminf(v73[2], v77);
		v73[3] = fmaxf(v75, v73[3]);
		v73[4] = fmaxf(v76, v73[4]);
		v73[5] = fmaxf(v77, v73[5]);
		rvParticleTemplate::EvaluateSimplePosition(
			this,
			(idVec3*)&pos.y,
			max,
			this->mDuration.y,
			(idVec3*)&spawn.b[1].y,
			(idVec3*)&velocity.b[1].y,
			(idVec3*)&accel.b[1].y,
			(idVec3*)&friction.b[1].y);
		v78 = result.b[0].x;
		v79 = v92;
		v80 = *(float*)LODWORD(v92);
		*(float*)LODWORD(result.b[0].x) = fminf(*(float*)LODWORD(result.b[0].x), *(float*)LODWORD(v92));
		v81 = *(float*)(LODWORD(v79) + 4);
		*(float*)(LODWORD(v78) + 4) = fminf(*(float*)(LODWORD(v78) + 4), v81);
		v82 = *(float*)(LODWORD(v79) + 8);
		*(float*)(LODWORD(v78) + 8) = fminf(*(float*)(LODWORD(v78) + 8), v82);
		*(float*)(LODWORD(v78) + 12) = fmaxf(v80, *(float*)(LODWORD(v78) + 12));
		*(float*)(LODWORD(v78) + 16) = fmaxf(v81, *(float*)(LODWORD(v78) + 16));
		*(float*)(LODWORD(v78) + 20) = fmaxf(v82, *(float*)(LODWORD(v78) + 20));
		++LODWORD(v2);
		max = v2;
	} while (SLODWORD(v2) < 8);
	result.b[0].x = result.b[0].z * result.b[0].z + result.b[0].y * result.b[0].y + result.b[1].x * result.b[1].x;
	result.b[0].x = sqrt(result.b[0].x);
	v92 = result.b[0].x * 0.5;
	result.b[0].x = result.b[1].z * result.b[1].z + result.b[1].y * result.b[1].y + accel.b[0].x * accel.b[0].x;
	result.b[0].x = sqrt(result.b[0].x);
	result.b[0].x = result.b[0].x * 0.5;
	v83 = result.b[0].x;
	if (v92 >= (double)result.b[0].x)
		v83 = v92;
	return v83;
*/
	return 0.0f;
}

float rvSegmentTemplate::CalculateBounds()
{
	rvParticleTemplate* v1; // esi
	int v2; // eax
	double v3; // st7
	float maxDist; // ST10_4
	float v5; // ST18_4
	double result; // st7
	float maxSize; // [esp+8h] [ebp-8h]
	float v8; // [esp+Ch] [ebp-4h]

	//switch (this->mSegType)
	//{
	//case 2:
	//case 3:
	//case 7:
	//	v1 = &this->mParticleTemplate;
	//	v8 = rvParticleTemplate::GetMaxParmValue(
	//		this->mParticleTemplate.mpSpawnSize,
	//		this->mParticleTemplate.mpDeathSize,
	//		this->mParticleTemplate.mpSizeEnvelope);
	//	maxSize = rvParticleTemplate::GetFurthestDistance(v1);
	//	v2 = v1->mType;
	//	if (v2 == 2 || v2 == 7)
	//		v3 = rvParticleTemplate::GetMaxParmValue(v1->mpSpawnLength, v1->mpDeathLength, v1->mpLengthEnvelope);
	//	else
	//		v3 = 0.0;
	//	maxDist = v3;
	//	v5 = rvParticleTemplate::GetMaxParmValue(v1->mpSpawnOffset, v1->mpDeathOffset, v1->mpOffsetEnvelope)
	//		+ maxSize
	//		+ v8
	//		+ maxDist;
	//	result = v5;
	//	break;
	//case 6:
	//	result = rvParticleTemplate::GetMaxParmValue(
	//		this->mParticleTemplate.mpSpawnSize,
	//		this->mParticleTemplate.mpDeathSize,
	//		this->mParticleTemplate.mpSizeEnvelope);
	//	break;
	//default:
		result = 8.0;
		//break;
	//}
	return result;
}

float rvDeclEffect::CalculateBounds()
{
	int v2; // edi
	int v3; // ebx
	float segSize; // [esp+10h] [ebp-8h]
	float v7; // [esp+14h] [ebp-4h]

	segSize = 0.0;
	v2 = 0;
	if (this->mSegmentTemplates.Num() > 0)
	{
		v3 = 0;
		do
		{
			v7 = mSegmentTemplates[v3].CalculateBounds();// rvSegmentTemplate::CalculateBounds(&v1->mSegmentTemplates.list[v3]);
			if (segSize < (double)v7)
				segSize = v7;
			++v2;
			++v3;
		} while (v2 < mSegmentTemplates.Num());
	}
	return (float)ceil(segSize);
}