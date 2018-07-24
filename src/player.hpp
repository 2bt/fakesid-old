#pragma once
#include <array>
#include <vector>


enum {
    MIXRATE           = 44100,
    SAMPLES_PER_FRAME = MIXRATE / 50,
    CHANNEL_COUNT     = 4,
    TRACK_LENGTH      = 32,
    TRACK_COUNT       = 255,
    PAGE_LENGTH       = 16,
};


struct Track {
    struct Row {
        uint8_t instrument;
        uint8_t effect;
        uint8_t note;
    };
    std::array<Row, TRACK_LENGTH> rows;
//    int length;
};


enum Flags {
    GATE  = 0x01,
    SYNC  = 0x02,
    RING  = 0x04,
    TRI   = 0x10,
    SAW   = 0x20,
    PULSE = 0x40,
    NOISE = 0x80,
};

enum Operations {
    SET_PULSEWIDTH,
    INC_PULSEWIDTH,
};

struct Intrument {
    struct Row {
        uint8_t flags;
        uint8_t operaton;
        uint8_t value;
    };

    uint8_t          ad;
    uint8_t          sr;
    uint8_t          silence;

    std::vector<Row> rows;
    uint8_t          loop;
};


struct Tune {
    int tempo; // 4 to F
    int swing; // 0 to 4

    std::array<Track, TRACK_COUNT> tracks;


    using Block = std::array<int, CHANNEL_COUNT>;
    std::vector<Block> table;

};


namespace player {
    void  fill_buffer(short* buffer, int length);
    void  play();
    void  pause();
    void  stop();
    bool  is_playing();
    int   row();
    int   block();
    void  block(int b);
    bool  is_channel_active(int c);
    void  set_channel_active(int c, bool a);
    Tune& tune();
}
