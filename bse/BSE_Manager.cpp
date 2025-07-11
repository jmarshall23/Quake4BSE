/*
===========================================================================

QUAKE 4 BSE CODE RECREATION EFFORT - (c) 2025 by Justin Marshall(IceColdDuke).

QUAKE 4 BSE CODE RECREATION EFFORT is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QUAKE 4 BSE CODE RECREATION EFFORT is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QUAKE 4 BSE CODE RECREATION EFFORT.  If not, see <http://www.gnu.org/licenses/>.

In addition, the QUAKE 4 BSE CODE RECREATION EFFORT is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "bse.h"

rvBSEManagerLocal bseLocal;
rvBSEManager* bse = &bseLocal;

idMat3  rvBSEManagerLocal::mModelToBSE;

// ──────────────────────────────────────────────────────────────────────────────
//  Implementation
// ──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::Init()
{
    common->Printf("----------------- BSE Init ------------------\n");

    // warm-up default resources so play-time is hitch-free
    declManager->FindEffect("_default", true);
    declManager->FindMaterial("_default", true);
    declManager->FindMaterial("gfx/effects/particles_shapes/motionblur", true);
    declManager->FindType(DECL_TABLE, "halfsintable", true, false);
    renderModelManager->FindModel("_default");

    //g_decals = cvarSystem->Find("g_decals");

    // register console commands
    //cmdSystem->AddCommand("bseStats", &rvBSEManagerLocal::Cmd_Stats,
    //    nullptr, "Dump effect statistics; pass 'all' to force parse");
    //cmdSystem->AddCommand("bseLog", &rvBSEManagerLocal::Cmd_Log,
    //    nullptr, "Dump effect play counts since game start");

    common->Printf("--------- BSE Created Successfully ----------\n");
    return true;
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::Shutdown()
{
    common->Printf("--------------- BSE Shutdown ----------------\n");

    for (int i = 0; i < traceModels.Num(); i++)
    {
        delete traceModels[i];
    }
    traceModels.Clear();    

    common->Printf("---------------------------------------------\n");
    return true;
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::EndLevelLoad()
{
   // effectCredits.fill(0.0f);
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::StartFrame()
{
  //  if (DebugHudActive())
   //     perfCounters_.fill(0);
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::EndFrame()
{
    if (!DebugHudActive()) return;

    //game->DebugSetInt("fx_num_active", perfCounters_[0]);
    //game->DebugSetInt("fx_num_traces", perfCounters_[1]);
    //game->DebugSetInt("fx_num_particles", perfCounters_[2]);
    //game->DebugSetFloat("fx_num_texels",
    //    static_cast<float>(perfCounters_[3]) / (1 << 20)); // 2^20 == 1 048 576
    //game->DebugSetInt("fx_num_segments", perfCounters_[4]);
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::UpdateRateTimes()
{
//    for (float& credit : effectCredits_)
//        credit = std::max(0.0f, credit - kDecayPerFrame);
}
//──────────────────────────────────────────────────────────────────────────────
float rvBSEManagerLocal::EffectDuration(const rvRenderEffectLocal* def)
{
    return (def && def->index >= 0 && def->effect)
        ? def->effect->mDuration
        : 0.0f;
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::CheckDefForSound(const renderEffect_t* def)
{
    rvDeclEffect* decl = (rvDeclEffect*)def->declEffect;    
    return (decl->mFlags & 1u) != 0;
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::SetDoubleVisionParms(float t, float s)
{
    game->StartViewEffect(0, t, s);
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::SetShakeParms(float t, float s)
{
    game->StartViewEffect(1, t, s);
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::SetTunnelParms(float t, float s)
{
    game->StartViewEffect(2, t, s);
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::Filtered(const char* name, effectCategory_t cat)
{
    //const char* filter = bse_singleEffect->GetString();        // CVAR helper
    //const bool  filterEmpty = (filter[0] == '\0');
    //
    //const bool namePasses = filterEmpty || (strstr(name, filter) != nullptr);
    //const bool ratePasses = CanPlayRateLimited(cat);
    //
    //return !(namePasses && ratePasses);
    return false;
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::CanPlayRateLimited(effectCategory_t c)
{
    //if (c == EC_IGNORE || bse_rateLimit->GetFloat() <= 0.1f)
    //    return true;
    //
    //const float cost = effectCosts[c] * bse_rateCost->GetFloat();
    //const float limit = bse_rateLimit->GetFloat();
    //float& bucket = effectCredits_[c];
    //
    //// simple leaky-bucket: if >50 % full and the random test fails, refuse
    //if (bucket > 0.5f * limit &&
    //    cost + bucket > rvRandom::flrand(0.0f, limit))
    //    return false;
    //
    //bucket += cost;
    return true;
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::StopEffect(rvRenderEffectLocal* def)
{
    if (!def || def->index < 0 || !def->effect) return;

//    if (bse_debug->GetBool())
//        common->Printf("BSE: Stop: %s\n",
//            def->parms.declEffect->base->GetName());

    def->effect->mFlags |= F_STOP_REQUESTED;
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::FreeEffect(rvRenderEffectLocal* def)
{
    if (!def || def->index < 0 || !def->effect) return;

//    if (bse_debug->GetBool())
//        common->Printf("BSE: Free: %s\n",
//            def->parms.declEffect->base->GetName());

    def->effect->Destroy();                     // returns memory to pool
    effects_.Free(def->effect);
    def->effect = nullptr;
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::ServiceEffect(rvRenderEffectLocal* def, float now)
{
    if (!def || !def->effect) return true;            // nothing to do → finished

// jmarshall
//    if (Filtered(def->parms.declEffect->base->GetName(),
//        def->parms.category))
//        return true;                                   // filtered out – treat as finished

    if (def->effect->Service(&def->parms, now))
        return true;                                   // still alive

    // effect finished – copy its final bounds for spatial culling
    def->referenceBounds = def->effect->mCurrentLocalBounds;

//    if (DebugHudActive())
//        ++perfCounters_[0];
//
//    if (common->IsMultiplayer() || bse_debug->GetBool())
//        rvBSE::EvaluateCost(def->effect, -1);

    return false;                                      // tell caller to destroy def
}
//──────────────────────────────────────────────────────────────────────────────
bool rvBSEManagerLocal::PlayEffect(rvRenderEffectLocal* def, float now)
{
    rvDeclEffect* decl = (rvDeclEffect *)def->parms.declEffect;

//    if (Filtered(decl->base->GetName(), def->parms.category))
//        return false;

//    if (bse_debug->GetBool())
//        common->Printf("BSE: Play %d: %s at %g\n",
//            ++count, decl->base->GetName(), now);

    ++decl->mPlayCount;

    def->effect = effects_.Alloc();
    def->effect->Init(decl, &def->parms, now);
    return true;
}
//──────────────────────────────────────────────────────────────────────────────
int rvBSEManagerLocal::AddTraceModel(idTraceModel* m)
{
    traceModels.Append(m);
    return traceModels.Num();
}
idTraceModel* rvBSEManagerLocal::GetTraceModel(int idx)
{
    return traceModels[idx];
}
void rvBSEManagerLocal::FreeTraceModel(int idx)
{
    if (idx < 0 || idx >= traceModels.Num()) 
        return;
    delete traceModels[idx];
    traceModels[idx] = nullptr;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Console command glue
// ──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::Cmd_Stats(const idCmdArgs& args)
{
    /*  The original Hex-Rays dump walks the whole declManager list, counts segments,
        reports average particles, etc.  That code was extremely ugly and
        completely tied to idTech internals.

        Re-implementing it *verbatim* adds no instructional value, so here we only
        keep the behaviour (same console text) while writing it clearly.         */

   // const bool forceParseAll = (args.Argc() > 1 && !idStr::Icmp(args.Argv(1), "all"));
   //
   // const int numDecls = declManager->GetNumDecls(DECL_EFFECT);
   //
   // common->Printf("... processing %d registered effects\n", numDecls);
   //
   // int loaded = 0;
   // int neverReferenced = 0;
   // int segments = 0;
   // int segmentsWithParticles = 0;
   // int particlesTotal = 0;
   //
   // for (int i = 0; i < numDecls; ++i)
   // {
   //     auto* effect = declManager->EffectByIndex(i, forceParseAll);
   //     if (!effect || effect->base->GetState() != DS_PARSED)
   //         continue;
   //
   //     ++loaded;
   //     const int segCount = effect->mSegmentTemplates.Num();
   //     segments += segCount;
   //
   //     for (int s = 0; s < segCount; ++s)
   //     {
   //         auto* seg = effect->GetSegmentTemplate(s);
   //         const bool hasParticles = seg->mFlags & ST_HAS_PARTICLES;
   //         if (hasParticles)
   //         {
   //             ++segmentsWithParticles;
   //             particlesTotal += static_cast<int>(seg->mCount.y);
   //         }
   //     }
   // }
   //
   // common->Printf("%d segments in %d loaded effects (%d never referenced)\n",
   //     segments, loaded, neverReferenced);
   //
   // if (loaded > 0)
   // {
   //     common->Printf("%.2f segments per effect\n",
   //         static_cast<float>(segments) / loaded);
   // }
   // if (segments > 0)
   // {
   //     common->Printf("%.2f of segments have particles\n",
   //         static_cast<float>(segmentsWithParticles) / segments);
   //     common->Printf("%.2f particles per segment with particles\n",
   //         static_cast<float>(particlesTotal) /
   //         std::max(1, segmentsWithParticles));
   // }
}
//──────────────────────────────────────────────────────────────────────────────
void rvBSEManagerLocal::Cmd_Log(const idCmdArgs& /*args*/)
{
    const int numDecls = declManager->GetNumDecls(DECL_EFFECT);

    common->Printf("Processing %d effect decls...\n", numDecls);

    int playedOrLooped = 0;

    for (int i = 0; i < numDecls; ++i)
    {
        auto* e = declManager->EffectByIndex(i, false);
        if (!e) continue;

        const int plays = e->mPlayCount;
        const int loops = e->mLoopCount;

        if (plays || loops)
        {
            common->Printf("%d plays (%d loops): '%s'\n",
                plays, loops, e->base->GetName());
            ++playedOrLooped;
        }
    }

    common->Printf("%d effects played or looped out of %d\n",
        playedOrLooped, numDecls);
}
//──────────────────────────────────────────────────────────────────────────────