#include "LMP.h"
#include "BVH.h"
#include "Pool.h"
#include "ai.h"
#include <util/Seeder.h>
//#include "debug.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#undef GLM_ENABLE_EXPERIMENTAL
#include <limits>

// TODO: properly polymorph for all BoundingVolumes
float LMP::ttc(BoundVolume& i, glm::vec2 iv, BoundVolume& j, glm::vec2 jv) {
    // I wish there was a way I didn't have to check the types..
    float ttc = -1.f;
    if (i._vt == BoundVolume::volume_type::CIRC) {
        Circ& c = static_cast<Circ&>(i);
        if (j._vt == BoundVolume::volume_type::CIRC) {
            ttc = ttc_(c, iv, static_cast<Circ&>(j), jv);
        } else if (j._vt == BoundVolume::volume_type::RECT) {
            ttc = ttc_(c, iv, static_cast<Rect&>(j), jv);
        }
    } else if (i._vt == BoundVolume::volume_type::RECT) {
        Rect& r = static_cast<Rect&>(i);
        if (j._vt == BoundVolume::volume_type::RECT) {
            ttc = LMP::ttc_(r, iv, static_cast<Rect&>(j), jv);
        } else if (j._vt == BoundVolume::volume_type::CIRC) {
            ttc = LMP::ttc_(static_cast<Circ&>(j), jv, r, iv);
        }
    }
    if (ttc < 0 || std::isnan(ttc))
        return std::numeric_limits<float>::max();
    else
        return ttc;
}

float LMP::ttc_(Circ& i, glm::vec2 iv, Circ& j, glm::vec2 jv) {
    /*
    float r = i._r + j._r;
    glm::vec2 w = j._o - i._o;
    float w2 = glm::dot(w, w);
    float c = w2 - r * r;
    if (c < 0) {// agents are colliding
        //return 0; //original

        //as per Stephen Guy's suggestion; halve the radii when colliding
        //as per Caleb Biasco's suggestion; use the smallest radii, not both
        float smaller = (i._r > j._r ? j._r : i._r);
        r -= smaller;
        c = w2 - r * r;
    }
    */
    Circ cspace_circ = Circ(i._o, i._r + j._r);
    return cspace_circ.intersect(j._o, jv - iv);
}

// finds the ttc via a component analysis of the velocity vectors
float LMP::ttc_(Rect& i, glm::vec2 iv, Rect& j, glm::vec2 jv) {
    Rect r = Rect(i._o, i._w + j._w, i._h + j._h);
    glm::vec2 dv = jv - iv;
    return r.intersect(j._o, dv);
}

float LMP::ttc_(Circ& i, glm::vec2 iv, Rect& j, glm::vec2 jv) {
    Rect h(j._o, 2 * i._r + j._w, j._h);
    Rect v(j._o, j._w, 2 * i._r + j._h);
    Circ tr(glm::vec2(j._o.x + j._w / 2, j._o.y + j._h / 2), i._r);
    Circ tl(glm::vec2(j._o.x - j._w / 2, j._o.y + j._h / 2), i._r);
    Circ br(glm::vec2(j._o.x + j._w / 2, j._o.y - j._h / 2), i._r);
    Circ bl(glm::vec2(j._o.x - j._w / 2, j._o.y - j._h / 2), i._r);

    std::array<BoundVolume*, 6> ms{&h, &v, &tr, &tl, &br, &bl};

    float t_min = std::numeric_limits<float>::max();
    glm::vec2 dv = iv - jv;
    for (BoundVolume* bv : ms) {
        float t = bv->intersect(i._o, dv);
        if (t < t_min)
            t_min = t;
    }
    return t_min;
}

glm::vec2 LMP::lookahead(Agent& a, BoundVolume& bv) {
    size_t target = static_cast<size_t>(a.num_done);
    if (target < a.plan->size()) {
        bool at_target = glm::length2(bv._o - (*a.plan)[target]) < ai::at_dist;
        if (at_target)
            ++target;
        size_t next = target + 1;

        // local_goal should be the next PRM node
        if (target < a.plan->size())
            a.local_goal = (*a.plan)[target];

        // do the actual looking ahead for further nodes
        while (true) {
            bool incomplete = next < a.plan->size();
            if (!incomplete)
                break;
            bool next_visible = a.cspace->line_of_sight(bv._o, (*a.plan)[next]);
            if (!next_visible)
                break;
            ++target;
            a.local_goal = (*a.plan)[target];
            next = target + 1;
        }
        a.num_done = static_cast<int>(target);
    } else {
        a.local_goal = (*a.plan)[a.plan->size() - 1];
    }
    return a.local_goal;
}

const double TTC_THRESHOLD = 5.0;
glm::vec2 LMP::ttc_forces_(double ttc, glm::vec2 dir) {
    float len = glm::length2(dir);
    if (len > 0)
        dir /= sqrt(len);

    double t_h = TTC_THRESHOLD; // seconds
    double mag = 0;
    if (ttc >= 0 && ttc <= t_h)
        mag = (t_h - ttc) / (ttc + 0.001);
    mag = mag > 20 ? 20 : mag;
    return glm::vec2(mag * dir.x, mag * dir.y);
}
glm::vec2 LMP::ttc_forces(
    Dynamics& da,
    BoundVolume& bva,
    BoundVolume& bvb,
    float ttc
) {
    glm::vec2 V_dt(da.vel.x * ttc, da.vel.z * ttc);
    glm::vec2 dir = (bva._o + V_dt - bvb._o);
    return ttc_forces_(ttc, dir);
}
glm::vec2 LMP::ttc_forces(
    Dynamics& da,
    BoundVolume& bva,
    Dynamics& db,
    BoundVolume& bvb,
    float ttc
) {
    glm::vec2 aV_dt(da.vel.x * ttc, da.vel.z * ttc);
    glm::vec2 bV_dt(db.vel.x * ttc, db.vel.z * ttc);
    // glm::vec2 perturb(0.000001, 0.000001);
    glm::vec2 dir = (bva._o + aV_dt - (bvb._o + bV_dt)); // +perturb);
    // if (bva._vt == BoundVolume::volume_type::CIRC) {
    //    Circ future(bva._o + aV_dt, static_cast<Circ&>(bva)._r);
    //    dir = closest_circ_point(bvb._o + bV_dt, &future);
    //} else {
    //    Rect future(bva._o + aV_dt, static_cast<Rect&>(bva)._w,
    //    static_cast<Rect&>(bva)._h); dir = closest_aabb_point(bvb._o + bV_dt,
    //    &future);
    //}
    return ttc_forces_(ttc, dir);
}

/*
//distance to leader, weak close, strong far
glm::vec2 follow_force(Agent* lead, Agent* a) {
    GLfloat ff0_r = 2.0f;//radius of following force of 0
    GLfloat ff1_r = 10.0f;//radius of following force of 1 towards leader

    glm::vec2 ff;
    //todo dalton: del o ref
    glm::vec2 toLeader = lead->bv->o - a->bv->o;
    float dist2_to_leader = glm::dot(toLeader, toLeader);
    if (dist2_to_leader < ff0_r * ff0_r)
        ff = glm::vec2(0);
    else
        ff = toLeader * (dist2_to_leader - ff0_r) / (ff1_r - ff0_r);

    return ff;
}

glm::vec2 boid_force(Agent* a, BVH* dynamic_bvh, std::vector<Agent*> dynamics) {
    const float boid_speed = 1.2f;

    glm::vec2 avg_vel(0, 0);
    glm::vec2 avg_pos(0, 0);
    glm::vec2 avg_dir(0, 0);
    //limit to search for forces for boidlings
    float cohes_r_look = 1.0f;
    float align_r_look = 1.0f;
    float separ_r_look = .5f;
    glm::vec2 align_force;
    glm::vec2 cohesion_force;
    glm::vec2 follow_force;
    glm::vec2 spread_force;

    std::vector<Agent*> NNdynamic =
        dynamic_bvh->query(new Circ(a->bv->o, 1.1f));
    for (size_t i = 0; i < NNdynamic.size(); i++) {
        Agent* boid = NNdynamic[i];
        if (!boid->ai->has_boid_f() || boid == a) {
            continue;
        }

        // gather metrics for later forces
        glm::vec2 dist = boid->bv->o - a->bv->o;
        float fi = static_cast<float>(i);
        if (glm::dot(dist, dist) < align_r_look * align_r_look) {
            avg_vel = (avg_vel * fi + glm::vec2(boid->dyn->vel.x,
boid->dyn->vel.z)) / (fi + 1.f);
        }
        if (glm::dot(dist, dist) < cohes_r_look * cohes_r_look) {
            avg_pos = (avg_pos * fi + glm::vec2(boid->bv->o)) / (fi + 1.f);
        }

        // Seperation force //
        //force from inverted direction of nearest neighbours
        glm::vec2 toBoid = boid->bv->o - a->bv->o;
        float dist2_to_boid = glm::dot(toBoid, toBoid);

        if (dist2_to_boid < separ_r_look * separ_r_look) {
            spread_force += -toBoid / (10 * sqrt(dist2_to_boid));
        }
    }

    // alignnment force //
    //average velocity; pull towards that
    float norm = glm::length(avg_vel);
    if (norm != 0) {
        avg_vel /= norm;
    }
    align_force = (avg_vel - glm::vec2(a->dyn->vel.x, a->dyn->vel.z)) *
boid_speed;

    // cohesion force //
    //average cohesion; pull towards that
    cohesion_force = avg_pos - a->bv->o;

    glm::vec2 boid_force = align_force + cohesion_force + spread_force;
    if (glm::dot(boid_force, boid_force) > 20 * 20) {
        boid_force /= glm::length(boid_force);
        boid_force *= 20.f;
    }
    return boid_force;
}
*/

glm::vec2 LMP::calc_sum_force(
    Entity* e,
    BVH*,
    BVH*, // dynamic_bvh,
    std::vector<Entity*> statics,
    std::vector<Entity*> // dynamics) {
) {
    const float speed = 1.0f; // x m/s
    glm::vec2 goal_vel;
    glm::vec2 goal_F(0);

    // if there is a plan, follow it
    auto& a = *POOL.get<Agent>(*e);
    auto& bv = **POOL.get<BoundVolume*>(*e);
    auto& d = *POOL.get<Dynamics>(*e);
    if (a.has_plan()) {
        a.local_goal = LMP::lookahead(a, bv);
        Seeder s;
        std::uniform_real_distribution<float> perturb(-.01f, .01f);
        glm::vec2 fuzzy_goal = a.local_goal + glm::vec2(perturb(s.gen()));
        glm::vec2 diff = fuzzy_goal - bv._o;
        // prevents overshooting
        goal_vel = speed * (diff / glm::max(speed, glm::length(diff)));
    } else {
        a.local_goal = bv._o;
        goal_vel = glm::vec2(0);
    }

    const float k = 2.0f;
    goal_F = k * (goal_vel - glm::vec2(d.vel.x, d.vel.z));

    // float real_speed = glm::length(d.vel);

    /* ttc - approximate */
    glm::vec2 ttc_F(0);
    // keeping this small is very important for framerate; at some point I'll
    // have to replace this circle with an extrusion or cone. ... or a shifted
    // circle! this works pretty damn well! 1000 Agents at 30FPS!! :D
    glm::vec2 vel2d(d.vel.x, d.vel.z);
    //float sz = 2.f * static_cast<float>(TTC_THRESHOLD);
    //Rect q(bv._o, sz, sz);

    // auto NNdynamic = dynamic_bvh->query(&q);
    // for (auto& nearby_query : NNdynamic) {
    POOL.for_<Agent>([&](Agent& b, Entity& nearby) {
        // Entity* nearby = nearby_query.first;
        // Agent* b = POOL.get<Agent>(*nearby);
        BoundVolume& bbv = **POOL.get<BoundVolume*>(nearby);
        Dynamics& bd = *POOL.get<Dynamics>(nearby);
        if (&a == &b) {
            // if the agents are the same, move on.
            return; // continue
        }
        double ttc = LMP::ttc(
            bv, glm::vec2(d.vel.x, d.vel.z), bbv, glm::vec2(bd.vel.x, bd.vel.z)
        );
        if (ttc > TTC_THRESHOLD) { // seconds
            return; // continue
        }
        ttc_F += LMP::ttc_forces(d, bv, bd, bbv, static_cast<float>(ttc));
    });

    //auto NNstatic = static_bvh->query(&q);
    for (Entity* nearby : statics) {
        //Entity* nearby = nearby_query.first;
        BoundVolume& bbv = **POOL.get<BoundVolume*>(*nearby);
        if (glm::length2(bbv._o - glm::vec2(d.pos.x, d.pos.z)) > TTC_THRESHOLD) {
            continue;
        }
        double ttc =
            LMP::ttc(bv, glm::vec2(d.vel.x, d.vel.z), bbv, glm::vec2(0));
        if (ttc > TTC_THRESHOLD) { // seconds
            continue;
        }
        ttc_F += ttc_forces(d, bv, bbv, static_cast<float>(ttc));
    }

    /*
    glm::vec2 boid_F(0);
    if (a->ai->has_boid_f())
        boid_F += boid_force(a, dynamic_bvh, dynamics);

    glm::vec2 follow_F(0);
    if (a->ai->method == ai_comp::Planner::PACK) {
        for (Object * leader : leaders) {
            follow_F += follow_force(leader, a);
        }
    }
    */

    return goal_F + ttc_F;
}
