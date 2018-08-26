#include "resource.hpp"
#include "gfx.hpp"
#include <SDL_image.h>


namespace resource {
namespace {


constexpr FontSpec m_font_specs[] = {
    {
        TEX_FONT_DEFAULT, 38, 52, {
            11, 12, 15, 26, 24, 30, 26, 8, 15, 15, 21, 24, 9, 12, 13, 18,
            24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 11, 10, 21, 23, 22, 20,
            37, 28, 26, 27, 27, 24, 23, 28, 30, 12, 23, 26, 23, 36, 30, 29,
            27, 29, 25, 25, 25, 27, 27, 37, 26, 25, 25, 12, 18, 12, 18, 19,
            15, 23, 24, 22, 24, 22, 15, 24, 23, 11, 12, 22, 11, 36, 23, 24,
            24, 24, 15, 22, 14, 23, 21, 31, 21, 20, 21, 15, 12, 15, 28, 16,
        }
    },
    {
        TEX_FONT_MONO, 30, 51, {
            29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
            29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
            29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
            29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
            29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
            29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
        }
    },
};


std::array<SDL_Texture*, TEX_COUNT> m_textures;


SDL_Texture* load_texture(char const* name) {
    SDL_Surface* img = IMG_Load(name);

    // convert grayscale to alpha
    {
        if (img->format->palette) {
            auto p = img->format->palette;
            for (int i = 0; i < p->ncolors; ++i) {
                SDL_Color& c = p->colors[i];
                c = { 255, 255, 255, c.r };
            }
        }

        SDL_Surface* conv = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(img);
        img = conv;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(gfx::renderer(), img);
    SDL_FreeSurface(img);

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}


} // namespace


bool init() {
    if (!(m_textures[TEX_FONT_DEFAULT] = load_texture("font-default.png")) ||
        !(m_textures[TEX_FONT_MONO]    = load_texture("font-mono.png")) ||
        false) return false;
    return true;
}


void free() {
    for (auto &tex : m_textures) {
        SDL_DestroyTexture(tex);
        tex = nullptr;
    }
}


SDL_Texture*     texture(TextureID tex) { return m_textures[tex]; }
FontSpec const & font_spec(FontID font) { return m_font_specs[font]; }


} // namespace
