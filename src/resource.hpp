#pragma once
#include <array>
#include <SDL.h>


enum TextureID {
    TEX_FONT_DEFAULT,
    TEX_FONT_MONO,
    TEX_FONT_SMALL,

    TEX_COUNT
};


enum FontID {
    FONT_DEFAULT,
    FONT_MONO,
    FONT_SMALL,
};


struct FontSpec {
    TextureID           texture;
    int                 width;
    int                 height;
    std::array<int, 96> spacing;
};


namespace resource {
    bool init();
    void free();
    SDL_Texture*     texture(TextureID tex);
    FontSpec const & font_spec(FontID);
}
