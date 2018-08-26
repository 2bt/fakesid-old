#pragma once
#include "resource.hpp"
#include <SDL.h>
#include <glm.hpp>


using Vec = glm::ivec2;


namespace gfx {
    bool init();
    void free();
    bool process_event(const SDL_Event& e);
    void present();
    void clear();
    void render(TextureID tex, SDL_Rect const& src, SDL_Rect const& dst, int flip=0);

    SDL_Renderer* renderer();
    SDL_Window*   window();
    Vec const&    screensize();

    void font(FontID font);
    void color(SDL_Color color);
    void print(Vec const& pos, char const* str);
    void printf(Vec const& pos, char const* fmt, ...);
    Vec text_size(char const* str);
    void rectangle(Vec const& pos, Vec const& size, int style);
}
