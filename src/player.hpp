#pragma once
#include <array>
#include <vector>


enum {
    MIXRATE               = 44100,
    SAMPLES_PER_FRAME     = MIXRATE / 50,
    CHANNEL_COUNT         = 4,
    TRACK_LENGTH          = 32,
    TRACK_COUNT           = 255,
    INSTRUMENT_COUNT      = 46,
    EFFECT_COUNT          = 46,
    MAX_INSTRUMENT_LENGTH = 16,
    MAX_EFFECT_LENGTH     = 16,
    MAX_NAME_LENGTH       = 16,

    PAGE_LENGTH           = 16,
};


struct Track {
    struct Row {
        uint8_t instrument;
        uint8_t effect;
        uint8_t note;
    };
    std::array<Row, TRACK_LENGTH> rows;
//    uint8_t length;
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


struct Instrument {
    struct Row {
        uint8_t flags;
        uint8_t operaton;
        uint8_t value;
    };
    std::array<char, MAX_NAME_LENGTH>      name;
    std::array<uint8_t, 4>                 adsr;
    uint8_t                                silence;
    uint8_t                                length;
    uint8_t                                loop;
    std::array<Row, MAX_INSTRUMENT_LENGTH> rows;
};


struct Effect {
    std::array<char, MAX_NAME_LENGTH>      name;
    uint8_t                                length;
    uint8_t                                loop;
    std::array<uint8_t, MAX_EFFECT_LENGTH> rows;
};


struct Tune {
    uint8_t tempo; // 4 to F
    uint8_t swing; // 0 to 4

    std::array<Track, TRACK_COUNT>           tracks;
    std::array<Instrument, INSTRUMENT_COUNT> instruments;
    std::array<Effect, EFFECT_COUNT>         effects;

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
    bool  block_loop();
    void  block_loop(bool b);
    bool  is_channel_active(int c);
    void  set_channel_active(int c, bool a);
    Tune& tune();
}
