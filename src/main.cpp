#include "game.hpp"
#include "input.hpp"
#include <cstdio>
#include <cstdint>
#include <array>


int main(int argc, char** args) {

    if (!gfx::init()) return 1;
    if (!game::init()) return 1;


    SDL_Event e;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&e)) {

            switch (e.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
                if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) running = false;
                break;

            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    gfx::resize({ e.window.data1, e.window.data2 });
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                    if (e.button.button != SDL_BUTTON_LEFT) break;
                    glm::ivec2 p = { e.button.x, e.button.y };
                    if (e.button.state == SDL_PRESSED) input::finger_down(999, p);
                    else                               input::finger_up(999, p);
                    break;
                }
            case SDL_MOUSEMOTION: {
                    if (!(e.motion.state & SDL_BUTTON_LMASK)) break;
                    glm::ivec2 p = { e.motion.x, e.motion.y };
                    input::finger_motion(999, p);
                    break;
                }

//            case SDL_FINGERDOWN:
//            case SDL_FINGERUP:
//            case SDL_FINGERMOTION: {
//                    glm::ivec2 s = gfx::screensize();
//                    glm::ivec2 p = { e.tfinger.x * s.x, e.tfinger.y * s.y };
//                    if (e.type == SDL_FINGERDOWN)   input::finger_down(e.tfinger.fingerId, p);
//                    if (e.type == SDL_FINGERUP)     input::finger_up(e.tfinger.fingerId, p);
//                    if (e.type == SDL_FINGERMOTION) input::finger_motion(e.tfinger.fingerId, p);
//                    break;
//                }
            default: break;
            }
        }


        game::draw();

        input::post_update();
    }

    game::free();
    gfx::free();

    return 0;
}
