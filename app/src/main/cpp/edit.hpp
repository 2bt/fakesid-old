#pragma once

enum EView {
    VIEW_PROJECT,
    VIEW_SONG,
    VIEW_TRACK,
    VIEW_INSTRUMENT,
    VIEW_EFFECT,
};

namespace edit {
    bool is_playing();
    void set_playing(bool p);
    void set_view(EView v);
    void set_popup(void (*func)(void));

    bool init();
    void draw();
    void free();
}
