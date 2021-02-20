// BSP_Manager.cpp
//

#pragma hdrstop
#include "precompiled.h"

#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"

idCVar bse_speeds("bse_speeds", "0", CVAR_INTEGER, "print bse frame statistics");
idCVar bse_enabled("bse_enabled", "1", CVAR_BOOL, "set to false to disable all effects");
idCVar bse_render("bse_render", "1", CVAR_BOOL, "disable effect rendering");
idCVar bse_debug("bse_debug", "0", CVAR_INTEGER, "display debug info about effect");
idCVar bse_showBounds("bse_showbounds", "0", CVAR_BOOL, "display debug bounding boxes effect");
idCVar bse_physics("bse_physics", "1", CVAR_BOOL, "disable effect physics");
idCVar bse_debris("bse_debris", "1", CVAR_BOOL, "disable effect debris");
idCVar bse_singleEffect("bse_singleEffect", "", 0, "set to the name of the effect that is only played");
idCVar bse_rateLimit("bse_rateLimit", "1", CVAR_FLOAT, "rate limit for spawned effects");
idCVar bse_rateCost("bse_rateCost", "1", CVAR_FLOAT, "rate cost multiplier for spawned effects");

rvBSEManagerLocal			bseLocal;
rvBSEManager* bse = &bseLocal;

float effectCosts[EC_MAX] = { 0, 2, 0.1 }; // dd 0.0, 2 dup(0.1)

idBlockAlloc<rvBSE, 256>	rvBSEManagerLocal::effects;
idVec3						rvBSEManagerLocal::mCubeNormals[6];
idMat3						rvBSEManagerLocal::mModelToBSE;
idList<idTraceModel*>		rvBSEManagerLocal::mTraceModels;
const char*					rvBSEManagerLocal::mSegmentNames[SEG_COUNT];
int							rvBSEManagerLocal::mPerfCounters[NUM_PERF_COUNTERS];
float						rvBSEManagerLocal::mEffectRates[EC_MAX];

/*
================
BSE_Pause_f
================
*/
CONSOLE_COMMAND_SHIP(bsePause, "Use to pause all effects at the current time", NULL)
{
	if (bseLocal.pauseTime == 0.0f) {
		bseLocal.pauseTime = -1.0f;
		common->Printf("BSE Paused...\n");
	}
	else {
		bseLocal.pauseTime = 0.0f;
		common->Printf("BSE Activated...\n");
	}
}

/*
================
BSE_Log_f
================
*/
CONSOLE_COMMAND_SHIP(bseLog, "Dumps the number of times an effect has been played since game start", NULL) {
	// TODO
}

/*
====================
vBSEManagerLocal::EffectDuration
====================
*/
float rvBSEManagerLocal::EffectDuration(const rvRenderEffectLocal* def)
{
	double result; // st7

	if (!def || def->index < 0 || !def->effect)
		return 0.0;
	def->effect->GetDuration();
	return result;
}

/*
====================
rvBSEManagerLocal::CanPlayRateLimited
====================
*/
bool rvBSEManagerLocal::CanPlayRateLimited(effectCategory_t category)
{
	effectCategory_t v1; // ecx
	float v2; // esi
	bool result; // al
	float cost; // [esp+14h] [ebp+4h]

	v1 = category;
	if (category == EC_IGNORE || bse_rateLimit.GetFloat() <= 0.1000000014901161)
		return 1;
	v2 = (4 * category + 18828608); // jmarshall: 18828608 looks wierd
	cost = effectCosts[category] * bse_rateCost.GetFloat();
	if (bse_rateLimit.GetFloat() * 0.5 < rvBSEManagerLocal::mEffectRates[v1]
		&& cost + v2 > rvRandom::flrand(0.0, bse_rateLimit.GetFloat()))
	{
		return 0;
	}
	result = 1;
	v2 = cost + v2;
	return result;
}

/*
====================
rvBSEManagerLocal::Init
====================
*/
bool rvBSEManagerLocal::Init(void) {
	common->Printf("----------------- BSE Init ------------------\n");

	renderModelManager->FindModel("_default");

	common->Printf("--------- BSE Created Successfully ----------\n");
	return true;
}

/*
====================
rvBSEManagerLocal::Filtered
====================
*/
bool rvBSEManagerLocal::Filtered(const char* name, effectCategory_t category)
{
	const char* singleEffect = bse_singleEffect.GetString();

	if (singleEffect && singleEffect[0] != 0) {
		if (!strstr(name, singleEffect))
		{
			return true;
		}		
	}

	return CanPlayRateLimited(category);
}

/*
====================
rvBSEManagerLocal::UpdateRateTimes
====================
*/
void rvBSEManagerLocal::UpdateRateTimes()
{
	mEffectRates[0] = mEffectRates[0] - 0.1000000014901161;
	if (mEffectRates[0] < 0.0)
		mEffectRates[0] = 0.0;
	mEffectRates[1] = mEffectRates[1] - 0.1000000014901161;
	if (mEffectRates[1] < 0.0)
		mEffectRates[1] = 0.0;
	mEffectRates[2] = mEffectRates[2] - 0.1000000014901161;
	if (mEffectRates[2] < 0.0)
		mEffectRates[2] = 0.0;
}

/*
====================
rvBSEManagerLocal::StartFrame
====================
*/
void rvBSEManagerLocal::StartFrame()
{
	if (bse_speeds.GetInteger())
	{
		rvBSEManagerLocal::mPerfCounters[0] = 0;
		rvBSEManagerLocal::mPerfCounters[1] = 0;
		rvBSEManagerLocal::mPerfCounters[2] = 0;
		rvBSEManagerLocal::mPerfCounters[3] = 0;
		rvBSEManagerLocal::mPerfCounters[4] = 0;
	}
}

/*
====================
rvBSEManagerLocal::EndFrame
====================
*/
void rvBSEManagerLocal::EndFrame()
{
	if (bse_speeds.GetInteger()) {
		common->Printf("bse_active: %i particles: %i traces: %i texels: %i\n",
			rvBSEManagerLocal::mPerfCounters[0],
			rvBSEManagerLocal::mPerfCounters[2],
			rvBSEManagerLocal::mPerfCounters[1],
			(double)rvBSEManagerLocal::mPerfCounters[3] * 0.00000095367431640625);
	}
}

/*
====================
rvBSEManagerLocal::EndLevelLoad
====================
*/
void rvBSEManagerLocal::EndLevelLoad()
{
	mEffectRates[0] = 0.0f;
	mEffectRates[1] = 0.0f;
	mEffectRates[2] = 0.0f;
}

/*
====================
rvBSEManagerLocal::Shutdown
====================
*/
bool rvBSEManagerLocal::Shutdown(void) {
	common->Printf("--------------- BSE Shutdown ----------------\n");

	mTraceModels.Clear();	
	mEffectRates[0] = 0.0f;
	mEffectRates[1] = 0.0f;
	mEffectRates[2] = 0.0f;

	common->Printf("---------------------------------------------\n");
	return true;
}

/*
=======================
rvBSEManagerLocal::GetTraceModel
=======================
*/
idTraceModel* rvBSEManagerLocal::GetTraceModel(int index)
{
	idTraceModel* result; // eax

	if (index < 0 || index >= rvBSEManagerLocal::mTraceModels.Num())
		result = 0;
	else
		result = rvBSEManagerLocal::mTraceModels[index];
	return result;
}

/*
=======================
rvBSEManagerLocal::FreeTraceModel
=======================
*/
void rvBSEManagerLocal::FreeTraceModel(int index)
{
	if (index >= 0 && index < rvBSEManagerLocal::mTraceModels.Num())
	{
		delete rvBSEManagerLocal::mTraceModels[index];
		rvBSEManagerLocal::mTraceModels[index] = NULL;
	}
}

/*
=======================
rvBSEManagerLocal::PlayEffect
=======================
*/
bool rvBSEManagerLocal::PlayEffect(rvRenderEffectLocal* def, float time)
{
	const rvDeclEffect* v3; // esi

	v3 = def->parms.declEffect;
	idStr effectName = def->parms.declEffect->GetName();

	if(Filtered(effectName, EC_IGNORE))
		return 0;
	if (bse_debug.GetInteger())
	{
		common->Printf("Playing effect: %s at %g\n", effectName.c_str(), time);
	}
	++v3->mPlayCount;
	rvBSE* bse = effects.Alloc();	
	def->effect = bse;
	bse->Init(v3, &def->parms, time);
	return 1;
}

/*
=======================
rvBSEManagerLocal::RestartEffect
=======================
*/
void rvBSEManagerLocal::RestartEffect(rvRenderEffectLocal* def)
{
	int v1; // eax

	if (def && def->index >= 0 && def->effect)
	{
		if (bse_debug.GetInteger())
		{
			idStr effectName = def->parms.declEffect->GetName();
			common->Printf("Restarting effect %s\n", effectName.c_str());
		}		
		def->effect->SetStopped(false);
	}
}

/*
=======================
rvBSEManagerLocal::StopEffect
=======================
*/
bool rvBSEManagerLocal::ServiceEffect(rvRenderEffectLocal* def, float time, bool& forcePush)
{
	rvBSE* v5; // ebp
	idStr v6; // eax
	idBounds v9; // eax

	if (-1.0 == this->pauseTime)
		this->pauseTime = time;
	if (this->pauseTime > 0.0)
		time = this->pauseTime;
	v5 = def->effect;
	if (!v5)
		return 1;
	v6 = def->parms.declEffect->GetName();
	if (Filtered(v6, EC_IGNORE))
		return true;
	if (v5->Service(&def->parms, time, def->gameTime > def->serviceTime, forcePush))		
	{
		return true;
	}
	def->serviceTime = def->gameTime;
	v9 = v5->GetCurrentLocalBounds();
	def->referenceBounds[0].x = v9[0][0];
	def->referenceBounds[0].y = v9[0][1];
	def->referenceBounds[0].z = v9[0][2];
	def->referenceBounds[1].x = v9[1][0];
	def->referenceBounds[1].y = v9[1][1];
	def->referenceBounds[1].z = v9[1][2];
	if (bse_speeds.GetBool())
		++rvBSEManagerLocal::mPerfCounters[0];
	if (bse_debug.GetInteger())
		v5->EvaluateCost();
	
	return 0;
}

/*
=======================
rvBSEManagerLocal::StopEffect
=======================
*/
void rvBSEManagerLocal::StopEffect(rvRenderEffectLocal* def)
{	
	if (def && def->index >= 0 && def->effect)
	{
		if (bse_debug.GetInteger())
		{
			idStr effectName = def->parms.declEffect->GetName();
			common->Printf("Stopping effect %s\n", effectName.c_str());			
		}		
		def->effect->SetStopped(true);
	}
	else
	{
		def->newEffect = 0;
		def->expired = 1;
	}
}

/*
=======================
rvBSEManagerLocal::FreeEffect
=======================
*/
void rvBSEManagerLocal::FreeEffect(rvRenderEffectLocal* def)
{
	int v1; // eax
	rvBSE* v2; // eax

	if (def && def->index >= 0 && def->effect)
	{
		if (bse_debug.GetInteger())
		{
			idStr effectName = def->parms.declEffect->GetName();
			common->Printf("Freeing effect %s\n", effectName.c_str());
		}
		def->effect->Destroy();
		v2 = def->effect;
// jmarshall - not sure what this is
		//if (v2)
		//{
		//	v2[1].vfptr = (rvBSEVtbl*)unk_11F4D64;
		//	--unk_11F4D6C;
		//	unk_11F4D64 = v2;
		//}
// jmarshall end
		def->effect = 0;
	}
}

/*
=======================
rvBSEManagerLocal::CopySegment
=======================
*/
void rvBSEManagerLocal::CopySegment(rvSegmentTemplate* dest, rvSegmentTemplate* src)
{
	dest->Duplicate(*src);
}

/*
=======================
rvBSEManagerLocal::Stats
=======================
*/
void rvBSEManagerLocal::Stats(const idCmdArgs& args) {

}

/*
=======================
rvBSEManagerLocal::AddTraceModel
=======================
*/
int rvBSEManagerLocal::AddTraceModel(idTraceModel* model)
{
	mTraceModels.Append(model);
	return mTraceModels.Num() - 1;
}

/*
=======================
rvBSEManagerLocal::SetShakeParms
=======================
*/
void rvBSEManagerLocal::SetShakeParms(float time, float scale)
{
	// Missing game side code function StartViewEffect
}

/*
=======================
rvBSEManagerLocal::SetTunnelParms
=======================
*/
void __stdcall rvBSEManagerLocal::SetTunnelParms(float time, float scale)
{
	// Missing game side code function StartViewEffect
}


/*
=======================
rvBSEManagerLocal::SetTunnelParms
=======================
*/
void rvBSEManagerLocal::MakeEditable(rvParticleTemplate* particle)
{
	particle->MakeEditable();
}