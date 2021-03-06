#include "ai.h"
#include "Pool.h"
#include "model/CubeMesh.h"
#include "model/LineMesh.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

namespace ai {
Cspace2d* std_cspace;
PRM* std_prm;
BVH* static_bvh;
BVH* dynamic_bvh;
const float at_dist = 1.f;

static void rebuild_sbvh(std::vector<Entity*> statics) {
    static_bvh = new BVH(statics);
}
static void rebuild_dbvh(std::vector<Entity*> dynamics) {
    dynamic_bvh = new BVH(dynamics);
}

// todo: figure out how to handle varying size Cspace
void init() {
    static const float root2 = static_cast<float>(sqrt(2));
    std::vector<Entity*> statics;
    std::vector<Entity*> dynamics;
    POOL.for_<BoundVolume*>([&](BoundVolume*&, const Entity& e) {
        if (POOL.get<Agent>(e) && POOL.get<Dynamics>(e)) {
            dynamics.push_back(&const_cast<Entity&>(e));
        } else {
            statics.push_back(&const_cast<Entity&>(e));
        }
    });

    rebuild_sbvh(statics);
    rebuild_dbvh(dynamics);

    if (dynamics.size() > static_cast<size_t>(0)) {
        // cspace
        std::vector<BoundVolume*> obs_bv;
        for (Entity* s : statics) {
            obs_bv.push_back(*POOL.get<BoundVolume*>(*s));
        }
        std::unique_ptr<Cspace2d> cs(
            new Cspace2d(obs_bv, &**POOL.get<BoundVolume*>(*dynamics[0]))
        );
        std_cspace = cs.get();

        // prm
        glm::vec2 center_2d(0, 0); // org
        glm::vec2 dim(25 / 2, 25 / 2); // w,h
        dim *= 1; // cellsize
        std_prm = new PRM(
            std::move(cs),
            5.f * root2,
            0.f,
            glm::vec2(2.5f, 2.5f),
            0,
            center_2d - dim,
            center_2d + dim,
            1.f
        ); // 1.f

#ifdef PRM_DEBUG
#    ifndef NO_RENDER
        auto& rm = std_prm->_roadmap;
        std::vector<Vertex> endpoints(rm->vertex_num());
        std::vector<GLuint> lines(2 * rm->edge_num());

        rm->for_vertex([&](NodeId u) {
            /*
            Entity& v = POOL.spawn_entity();
            uint16_t tid = POOL.create<Transform>(Transform(nullptr));
            std::vector<Texture> tex = {};
            uint16_t mid = POOL.create<Mesh>(CubeMesh(tex));*/
            glm::vec2 v_pos = *rm->data(u);
            /*auto& t = *POOL.get<Transform>(tid);
            glm::mat4 scale(.3f);
            scale[3][3] = 1.f;
            t.set_mat(scale);
            t.set_pos(glm::vec3(v_pos.x, 0, v_pos.y));
            POOL.attach<Transform>(v, tid);
            POOL.attach<Mesh>(v, mid);
            */

            glm::vec3 pos(v_pos.x, 0, v_pos.y);
            Vertex end;
            end.pos = pos;
            end.norm = glm::vec3(0);
            end.tex = glm::vec2(0);
            endpoints[u] = end;
        });

        rm->for_edge([&](NodeId u, NodeId v) {
            lines.push_back(u);
            lines.push_back(v);
        });
        {
            Entity& debug_map = POOL.spawn_entity();
            uint16_t mid = POOL.create<Mesh>(LineMesh(endpoints, lines));
            POOL.attach<Mesh>(debug_map, mid);
        }
#    endif
#endif

        // planners
        POOL.for_<Agent>([&](Agent& a, Entity&) {
            a.num_done = 0;
            a.cspace = std_cspace;
            a.prm = std_prm;
            a.local_goal = glm::vec2(0, 0);
            a.plan = nullptr;

            GMP::plan_one(a);
            a.goal_dist = glm::length(a.final_goal - a.start);

            // min_time shouldn't include the last unit that isn't timed. The
            // min time isn't as large as it seems. Any other negative overheads
            // would be due to shortcutting the straight paths of A*
            a.min_time = -at_dist;
            glm::vec2 last = a.start;
            for (const auto& p : *a.plan) {
                a.min_time += glm::length(p - last);
                last = p;
            }
        });
    }
}

// move to AI/planner --- this is a force-based LMP
void update_agents() {
    std::vector<Entity*> statics;
    std::vector<Entity*> dynamics;
    POOL.for_<BoundVolume*>([&](BoundVolume*&, const Entity& e) {
        if (POOL.get<Agent>(e) && POOL.get<Dynamics>(e)) {
            dynamics.push_back(&const_cast<Entity&>(e));
        } else {
            statics.push_back(&const_cast<Entity&>(e));
        }
    });

    // probably very slow
    //delete static_bvh;
    //rebuild_sbvh(statics);
    // TODO: fix PRM

    delete dynamic_bvh;
    rebuild_dbvh(dynamics);

    int replanned = 0;
    const int limit = 2;
    POOL.for_<Agent>([&](Agent& a, Entity& e) {
        a.goal_dist = glm::length(a.final_goal - a.start);
        BoundVolume* bv = *POOL.get<BoundVolume*>(e);
        size_t next = static_cast<size_t>(a.num_done);
        bool visible_next = false;
        if (a.has_plan() && next < a.plan->size()) {
            visible_next = a.cspace->line_of_sight(bv->_o, (*a.plan)[next]);
        }
        a.lost_frames = visible_next ? 0 : a.lost_frames + 1;
        bool need_replan = !a.has_plan() || (a.done() && a.goal_dist > 1.f)
            || (!a.done() && !visible_next && a.lost_frames >= 360);
        if (replanned < limit && need_replan) {
            GMP::plan_one(a);
            ++replanned;
            a.lost_frames = 0;
        }

        glm::vec2 f2d =
            LMP::calc_sum_force(&e, static_bvh, dynamic_bvh, statics, dynamics);
        POOL.get<Dynamics>(e)->force += glm::vec3(f2d.x, 0, f2d.y);
    });
}

void terminate() {
    POOL.for_<Agent>([](Agent& a, Entity&) {
        if (a.plan) {
            delete a.plan;
            a.plan = nullptr;
        }
    });
    delete static_bvh;
    delete dynamic_bvh;
    delete std_prm;
}
} // namespace ai
