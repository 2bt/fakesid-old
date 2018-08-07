#pragma once
#include "song.hpp"


enum {
    MIXRATE               = 44100,
    SAMPLES_PER_FRAME     = MIXRATE / 50,
};


namespace player {
    void  fill_buffer(short* buffer, int length);
    void  reset();
    bool  is_playing();
    int   row();
    int   block();
    void  block(int b);
    bool  block_loop();
    void  block_loop(bool b);
    bool  is_channel_active(int c);
    void  set_channel_active(int c, bool a);
    Song& song();
}
