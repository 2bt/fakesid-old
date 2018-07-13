#pragma once
#include <array>
#include <SDL.h>


enum TextureID {
    TEX_FONT_DEFAULT,
    TEX_FONT_MONO,

    TEX_COUNT
};


enum FontID {
    FONT_DEFAULT,
    FONT_MONO,
};


struct FontSpec {
    TextureID            texture;
    int                  width;
    int                  height;
    std::array<int, 128> spacing;
};


namespace resource {
    bool init();
    void free();
    SDL_Texture*     texture(TextureID tex);
    FontSpec const & font_spec(FontID);
}
