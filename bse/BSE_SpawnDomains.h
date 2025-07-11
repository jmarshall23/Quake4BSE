#pragma once
// ──────────────────────────────────────────────────────────────────────────────
//  Doom-3 / BSE   –   Particle-system helpers
// ──────────────────────────────────────────────────────────────────────────────
//
//  • rvParticleParms – parameter block used by the BSE (“Basic Set of Effects”)
//    runtime for spawning particles.
//
//  • A family of free “Spawn*” helpers that know how to fill out a position/
//    scalar/normal vector based on the parameter block.
//
//  All code is self-contained except for three base headers that already exist
//  in the id Tech 4 code-base:
//
//      #include "idMath.h"     //  idMat3, idVec3, idMath::TWO_PI …
//      #include "Random.h"     //  rvRandom::flrand / irand
//      #include "RenderWorld.h"//  srfTriangles_s, R_DeriveFacePlanes(…)
//
//  If you are porting outside id Tech 4, stub these 3 headers appropriately.
//
// ──────────────────────────────────────────────────────────────────────────────
#include <cstddef>     //  size_t
#include <cstdint>     //  uint32_t
#include <cmath>       //  fabs, std::sqrt

// Forward declarations to avoid pulling heavy headers into every unit
struct idVec3;
struct idMat3;

void R_DeriveFacePlanes(struct srfTriangles_s* tris);

//───────────────────────────────────────────────────────────────────────────────
//  Free helper functions – declared here, defined in the .cpp
//───────────────────────────────────────────────────────────────────────────────
void SpawnGetNormal(idVec3* normal, const idVec3& pos,
    const idVec3* centre = nullptr);

void SpawnNone1(float* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);
void SpawnNone2(float* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);
void SpawnNone3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);

void SpawnOne1(float* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);
void SpawnOne2(float* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);
void SpawnOne3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);

void SpawnPoint1(float* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);
void SpawnPoint2(float* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);
void SpawnPoint3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);

void SpawnLinear1(float* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);
void SpawnLinear2(float* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);
void SpawnLinear3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);

void SpawnBox1(float* out,
    const rvParticleParms& p);
void SpawnBox2(float* out,
    const rvParticleParms& p);
void SpawnBox3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);

void SpawnSurfaceBox1(float* out,
    const rvParticleParms& p);
void SpawnSurfaceBox2(float* out,
    const rvParticleParms& p);
void SpawnSurfaceBox3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);

void SpawnSphere2(float* out,
    const rvParticleParms& p);
void SpawnSphere3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);

void SpawnSurfaceSphere2(float* out,
    const rvParticleParms& p);
void SpawnSurfaceSphere3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);

void SpawnCylinder3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);
void SpawnSurfaceCylinder3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);

void SpawnSpiral2(float* out,
    const rvParticleParms& p);
void SpawnSpiral3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);

void SpawnModel3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = nullptr,
    const idVec3* c = nullptr);


void SpawnStub(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnNone1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnNone2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnNone3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnOne1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnOne2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnOne3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnPoint1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnPoint2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnPoint3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnLinear1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnLinear2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnLinear3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnBox1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnBox2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnBox3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceBox1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceBox2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceBox3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSphere2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSphere3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceSphere2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceSphere3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnCylinder3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceCylinder3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSpiral2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSpiral3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnModel3(float*, const rvParticleParms&, idVec3*, const idVec3*);

