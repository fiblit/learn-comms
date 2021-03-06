#pragma once
#include "Agent.h"
#include "BVH.h"
#include "BoundVolume.h"
#include "Dynamics.h"
#include "GMP.h"
#include "LMP.h"
#include "PRM.h"

namespace ai {
extern Cspace2d* std_cspace;
extern PRM* std_prm;
extern BVH* static_bvh;
extern BVH* dynamic_bvh;
extern const float at_dist;
void init();
void update_agents();
void terminate();
} // namespace ai
