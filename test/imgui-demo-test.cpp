#include <iostream>
#include <vector>
#include <fstream>

#include "sre/Texture.hpp"
#include "sre/Renderer.hpp"
#include "sre/Inspector.hpp"
#include "sre/Material.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "sre/SDLRenderer.hpp"

#include "imgui_demo.cpp" 

using namespace sre;

class GUIExample {
public:
    GUIExample() {
        r.init()
                .withSdlInitFlags(SDL_INIT_EVERYTHING)
                .withSdlWindowFlags(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        // connect render callback
        r.frameRender = [&]() {
            frameRender();
        };
        // start render loop
        r.startEventLoop();
    }


    void frameRender() {
        RenderPass rp = RenderPass::create()
                .withCamera(camera)
                .build();
        static bool show = true;
        ImGui::ShowDemoWindow(&show);
    }
private:
    SDLRenderer r;
    Camera camera;
};

int main() {
    std::make_unique<GUIExample>();
    return 0;
}
