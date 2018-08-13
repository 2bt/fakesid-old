#pragma once

enum EView {
    VIEW_PROJECT,
    VIEW_SONG,
    VIEW_TRACK,
    VIEW_INSTRUMENT,
    VIEW_EFFECT,
};

namespace edit {
    void set_playing(bool p);
    bool is_playing();
    void set_view(EView v);

    void    select_instrument(int i);
    int     selected_instrument();
    void    select_effect(int e);
    int     selected_effect();

    void draw_instrument_cache();
    void draw_effect_cache();

    void sprint_inst_effect_id(char* dst, int nr);


    bool init();
    void draw();
    void free();
}
