#pragma once
#include <array>
#include <vector>


enum {
    MIXRATE               = 44100,
    SAMPLES_PER_FRAME     = MIXRATE / 50,
    CHANNEL_COUNT         = 4,
    TRACK_LENGTH          = 32,
    TRACK_COUNT           = 21 * 12,
    INSTRUMENT_COUNT      = 48,
    EFFECT_COUNT          = INSTRUMENT_COUNT,
    MAX_INSTRUMENT_LENGTH = 16,
    MAX_FILTER_LENGTH     = 16,
    MAX_EFFECT_LENGTH     = 16,
    MAX_NAME_LENGTH       = 16,
    MAX_SONG_LENGTH       = 256,
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


enum {
    GATE  = 0x01,
    SYNC  = 0x02,
    RING  = 0x04,
    TRI   = 0x10,
    SAW   = 0x20,
    PULSE = 0x40,
    NOISE = 0x80,
};


enum {
    OP_INC,
    OP_SET,
    OP_DEC,
};

enum {
    FILTER_LOW  = 1,
    FILTER_BAND = 2,
    FILTER_HIGH = 4,
};


struct Filter {
    struct Row {
        uint8_t type;
        uint8_t resonance;
        uint8_t operation;
        uint8_t value;
    };

    uint8_t                            routing;
    uint8_t                            length;
    uint8_t                            loop;
    std::array<Row, MAX_FILTER_LENGTH> rows;
};


struct Instrument {
    struct Row {
        uint8_t flags;
        uint8_t operation;
        uint8_t value;
    };

    std::array<char, MAX_NAME_LENGTH>      name;
    std::array<uint8_t, 4>                 adsr;
    uint8_t                                silence;

    uint8_t                                length;
    uint8_t                                loop;
    std::array<Row, MAX_INSTRUMENT_LENGTH> rows;

    Filter                                 filter;
};

struct Effect {
    std::array<char, MAX_NAME_LENGTH>      name;
    uint8_t                                length;
    uint8_t                                loop;
    std::array<uint8_t, MAX_EFFECT_LENGTH> rows;
};

struct Tune {
    using Block = std::array<int, CHANNEL_COUNT>;

    uint8_t tempo; // 4 to F
    uint8_t swing; // 0 to 4

    std::array<Track, TRACK_COUNT>           tracks;
    std::array<Instrument, INSTRUMENT_COUNT> instruments;
    std::array<Effect, EFFECT_COUNT>         effects;
    std::array<Block, MAX_SONG_LENGTH>       table;
    uint16_t                                 table_length;
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
