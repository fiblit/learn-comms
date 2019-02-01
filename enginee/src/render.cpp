#include "render.h"
#include "Camera.h"
#include "light/Shader.h"
#include "light/PointLight.h"
#include "light/DirLight.h"
#include "light/SpotLight.h"
#include "model/LineMesh.h"
#include "model/CubeMesh.h"
#include "util/debug.h"
#include "io.h"
#include "ui.h"
#include "Pool.h"
#include <glad.h>
#include <stb_image.h>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

#include "demo/demo.h"
namespace render {
float cam_dist = 1.f;
unique_ptr<Shader> mtl;
vector<unique_ptr<DirLight>> dir_lights;
vector<unique_ptr<PointLight>> point_lights;
vector<unique_ptr<SpotLight>> spot_lights;
unique_ptr<Camera> cam;

std::function<glm::vec3(glm::vec3)> comm_vis;

GLuint create_tex(std::string path) {
    auto img = read_image(path);
    if (!img) {
        cerr << "gg! Failed to create texture of " << path << "\n";
        return 0;
    }

    GLenum format = GL_RGB;
    if (img->channels == 1) {
        format = GL_RED;
    } else if (img->channels == 3) {
        format = GL_RGB;
    } else if (img->channels == 4) {
        format = GL_RGBA;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format),
        img->width, img->height, 0,
        format, GL_UNSIGNED_BYTE, img->bytes);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(img->bytes);//would prefer if this was solved by RAII
    return tex;
}

static glm::vec3 uv2rgb(glm::vec3 Luv) {
    glm::vec3 rgb = {
    //Here the 0.5 in each equation is just the desired luminance Y, you can play with it
        +0.000 * Luv[0] + 1.140 * Luv[1],
        -0.395 * Luv[0] - 0.581 * Luv[1],
        +2.032 * Luv[0] + 0.000 * Luv[1]
    };
    rgb += Luv[0];

    //Clamp the results to 0 to 1
    return glm::max(glm::vec3(0.f), glm::min(glm::vec3(1.f), rgb));
}

#ifndef NO_RENDER
static glm::vec3 Lab2rgb(glm::vec3 Lab) {
    // https://www.easyrgb.com/en/math.php
    Lab[0] *= 100.f;
    Lab[1] *= 128.f;
    Lab[2] *= 128.f;
    glm::vec3 XYZ;
    XYZ[0] = (Lab[0] + 16.) / 116.;
    XYZ[1] = Lab[1] / 500. + XYZ[0];
    XYZ[2] = XYZ[0] - Lab[2] / 200.;

    auto maybe_cube = [](double a) {
        double cube = a * a * a;
        if (cube > 0.008856) {
            return cube;
        }
        return (a - 16./116.) / 7.787;
    };
    // D65 , 2 degree standard illuminant (Daylight, sRGB, Adobe-RGB)
    XYZ[0] = 0.95047 * maybe_cube(XYZ[0]);
    XYZ[1] = 1.00000 * maybe_cube(XYZ[1]);
    XYZ[2] = 1.08883 * maybe_cube(XYZ[2]);

    glm::mat3 XYZ2rgb = {
        {3.2406, -1.5372, -0.4986},
        {-0.9689, 1.8758, 0.0415},
        {0.0557, -0.2040, 1.0570},
    };
    glm::vec3 rgb = XYZ * XYZ2rgb;

    auto finish_rgb = [](double a) -> double {
        if (a > 0.0031308) {
            a = 1.055 * pow(a, 1./2.4) - 0.055;
        } else {
            a *= 12.92;
        }
        return pow(max(0., min(1., a)), 2.2);
    };
    rgb[0] = finish_rgb(rgb[0]);
    rgb[1] = finish_rgb(rgb[1]);
    rgb[2] = finish_rgb(rgb[2]);
    return rgb;
}

static glm::vec3 Lx_2rg_(glm::vec3 Lx_) {
    float x = Lx_[1];
    return x < 0 ? glm::vec3(-x, 0, 0) : glm::vec3(0, x, 0);
}

static glm::vec3 L_y2_gb(glm::vec3 L_y) {
    float y = L_y[2];
    return y < 0 ? glm::vec3(0, 0, -y) : glm::vec3(0, y, 0);
}
#endif

void init(glm::vec<2, int> dims) {
    ui::add_handler(input_key);
    ui::add_handler(input_cursor);
    ui::add_handler(input_scroll);
    comm_vis = uv2rgb;

    //build material
    mtl = unique_ptr<Shader>(new Shader());
    std::string pwd(PROJECT_SRC_DIR);
    mtl->add(GL_VERTEX_SHADER, pwd + "/res/glsl/tex.vert");
    mtl->add(GL_FRAGMENT_SHADER, pwd + "/res/glsl/lit_mtl.frag");
    mtl->build();

    POOL.for_<Mesh>([&](Mesh& m, Entity&){
        //TODO: material as component so entities can set their albedo & shaders
        if (m._type == Mesh::Type::LINE) {
            //LineMesh& l = m;
            m.set_material(mtl.get(), 0, glm::vec3(0, 100, 0));
        } else {
            m.set_material(mtl.get(), 32);
        }
        //TODO:
        //.material(Material(
        //  shader,
        //  {textures},
        //  albedo reflections,
        //  shininess,
        //  other lit properties like metallicity and transparency.)
     });

    //send (static) lights to shader(s)
    mtl->use();
    mtl->set("n_dir_lights", static_cast<GLint>(dir_lights.size()));
    for (size_t i = 0; i < dir_lights.size(); ++i) {
        dir_lights[i]->pass_to(*mtl, "dir_lights[" + to_string(i) + "].");
    }
    mtl->set("n_point_lights", static_cast<GLint>(point_lights.size()));
    for (size_t i = 0; i < point_lights.size(); ++i) {
        point_lights[i]->pass_to(*mtl, "point_lights[" + to_string(i) + "].");
    }
    mtl->set("n_spot_lights", static_cast<GLint>(spot_lights.size()));
    for (size_t i = 0; i < spot_lights.size(); ++i) {
        spot_lights[i]->pass_to(*mtl, "spot_lights[" + to_string(i) + "].");
    }

    //set up camera
    cam = make_unique<Camera>();
    cam->aspect(static_cast<float>(dims.x) / static_cast<float>(dims.y));
    cam->set_pos(glm::vec3(0, 25.f * cam_dist, 0.f * cam_dist));
    cam->set_rot(glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));

    //TODO: Uniform buffer object; see below
    cam->apply_proj(*mtl);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}


void draw() {
    glClearColor(.2f, .2f, .2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    cam->apply_proj(*mtl);
    //TODO:
    //update camera view: note to self, the most efficient way to do this across
    // many shaders (i.e. materials) is to use Uniform Buffer Objects:
    //www.geeks3d.com/20140704/gpu-buffers-introduction-to-opengl-3-1-unfiorm-buffer-objects
    //is a good tutorial for them. Also, the Khronos standard seems pretty good.
    cam->apply_view(*mtl);

    //TODO: (pre?)rendering by scene graph?
    //TODO: dynamic lighting

    POOL.for_<Mesh>([&](Mesh& m, Entity& e){
        auto t = POOL.get<Transform>(e);
        if (t) {
            mtl->set("model", t->global_mat());
        } else {
            mtl->set("model", glm::mat4(1.f));
        }

        auto c = POOL.get<CommComp>(e);
        if (c) {
            glm::vec2 ab = c->c;
            m._diffuse = comm_vis(glm::vec3(0.5, ab));
        }
        //update models _and_ do glDraw; this combination seems to cause issues.
        if (m._type == Mesh::Type::LINE) {
            LineMesh l = m;
            l.draw();
        } else {
            m.draw();
        }
    });
}

//custom handler for input
void input_key(GLFWwindow* w, double ddt) {
    float dt = static_cast<float>(ddt);
    glm::vec3 motion(0, 0, 0);
    if (ui::key_map[GLFW_KEY_W]) {
        motion += glm::vec3(0, 0, 1);
    }
    if (ui::key_map[GLFW_KEY_D]) {
        motion += glm::vec3(1, 0, 0);
    }
    if (ui::key_map[GLFW_KEY_R]) {
        motion += glm::vec3(0, 1, 0);
    }
    if (ui::key_map[GLFW_KEY_S]) {
        motion += glm::vec3(0, 0, -1);
    }
    if (ui::key_map[GLFW_KEY_A]) {
        motion += glm::vec3(-1, 0, 0);
    }
    if (ui::key_map[GLFW_KEY_F]) {
        motion += glm::vec3(0, -1, 0);
    }
    cam->move(5.f*motion * dt);

    float roll = 0;
    if (ui::key_map[GLFW_KEY_Q]) {
        roll -= 1.0f;
    }
    if (ui::key_map[GLFW_KEY_E]) {
        roll += 1.0f;
    }
    roll *= dt;
    glm::vec3 up_new = cam->up() + roll * cam->right();
    cam->set_rot(cam->look_dir(), up_new);

    #ifndef NO_RENDER
    if (ui::edge_up(GLFW_KEY_X)) {
        if (glfwGetInputMode(w, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (ui::edge_up(GLFW_KEY_0)) {
        comm_vis = uv2rgb;
    } else if (ui::edge_up(GLFW_KEY_1)) {
        comm_vis = Lab2rgb;
    } else if (ui::edge_up(GLFW_KEY_2)) {
        comm_vis = Lx_2rg_;
    } else if (ui::edge_up(GLFW_KEY_3)) {
        comm_vis = L_y2_gb;
    }
    #else
    UNUSED(w);
    #endif

}

void input_cursor(GLFWwindow*, double) {//ddt) {
    //float dt = static_cast<float>(ddt);
    //glm::vec2 offset = ui::d_cursor_pos * 0.001f;
    //glm::vec3 look_new = cam->look_dir() +
    //    + offset.x * cam->right() - offset.y * cam->up();
    //cam->set_rot(look_new, cam->up());
}

void input_scroll(GLFWwindow*, double ddt) {
    float dt = static_cast<float>(ddt);
    cam->zoom(cam->zoom()*(1+ui::d_scroll * dt));
}

void framebuffer_resize(GLFWwindow* w, int width, int height) {
    int old_width;
    int old_height;
    glfwGetWindowSize(w, &old_width, &old_height);
    clog << "gg. Window resize (" << old_width << "," << old_height << ") -> "
         << "(" << width << "," << height << ")\n";
    glViewport(0, 0, width, height);

    cam->aspect(static_cast<float>(width)/static_cast<float>(height));
}
}//render::
