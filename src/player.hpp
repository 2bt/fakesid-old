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
        int instrument;
        int effect;
        int note;
    };
    std::array<Row, TRACK_LENGTH> rows;
//    int length;
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
    bool  is_channel_active(int c);
    void  set_channel_active(int c, bool a);
    Tune& tune();
}
