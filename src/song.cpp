#include "song.hpp"
#include <SDL.h>


void init_song(Song& song) {

    song.tempo = 5;
    song.table_length = 1;

    // preset some instruments/effects

    enum {
        BASS     = 27,
        KICK     = 28,
        SNARE    = 29,
        GLIDE_UP = 46,
        VIBRATO  = 47,
    };

    // bass
    {
        Instrument& i = song.instruments[BASS];
        strcpy(i.name.data(), "bass");
        i.hard_restart = 1;
        i.adsr = { 1, 8, 8, 8 };
        i.rows[0] = { NOISE | GATE, OP_SET, 13 };
        i.rows[1] = { PULSE | GATE, OP_INC, 3 };
        i.length = 2;
        i.loop = 1;
        i.filter.routing = 1;
        i.filter.rows[0] = { FILTER_LOW, 13, OP_SET, 12 };
        i.filter.rows[1] = { FILTER_LOW, 13, OP_DEC, 8 };
        i.filter.length = 2;
        i.filter.loop = 1;

        Effect& e = song.effects[BASS];
        strcpy(e.name.data(), "bass");
        e.rows[0] = 0x80 - 4 * 12;
        e.length = 1;
        e.loop = 0;
    }

    // kick
    {
        Instrument& i = song.instruments[KICK];
        strcpy(i.name.data(), "kick");
        i.adsr = { 1, 8, 8, 8 };
        i.hard_restart = 1;
        i.rows[0] = { NOISE | GATE, OP_SET, 13 };
        i.rows[1] = { PULSE | GATE, OP_INC, 3 };
        i.length = 2;
        i.loop = 1;
        i.filter.routing = 1;
        i.filter.rows[0] = { FILTER_LOW, 13, OP_SET, 13 };
        i.filter.rows[1] = { FILTER_LOW, 13, OP_SET, 20 };
        i.filter.rows[2] = { FILTER_LOW, 13, OP_SET, 5 };
        i.filter.length = 3;
        i.filter.loop = 2;

        Effect& e = song.effects[KICK];
        strcpy(e.name.data(), "kick");
        e.rows[0] = 0x80 + 4 * 12;
        e.rows[1] = 0x80 + 4 * 4;
        e.rows[2] = 0x80 - 4 * 4;
        e.rows[3] = 0x80 - 4 * 12;
        e.length = 4;
        e.loop = 3;
    }

    // snare
    {
        Instrument& i = song.instruments[SNARE];
        strcpy(i.name.data(), "snare");
        i.hard_restart = 1;
        i.adsr = { 1, 1, 7, 9 };
        i.rows[0] = { NOISE | GATE, OP_SET, 0x8 };
        i.rows[1] = { NOISE | GATE, OP_INC, 0x0 };
        i.rows[2] = { PULSE | GATE, OP_INC, 0x0 };
        i.rows[3] = { PULSE | GATE, OP_INC, 0x0 };
        i.rows[4] = { NOISE, OP_INC, 0x0 };
        i.length = 5;
        i.loop = 4;
        i.filter.routing = 1;
        i.filter.rows[0] = { FILTER_LOW, 15, OP_SET, 20 };
        i.filter.rows[1] = { FILTER_LOW, 15, OP_DEC, 3 };
        i.filter.length = 2;
        i.filter.loop = 1;

        Effect& e = song.effects[SNARE];
        strcpy(e.name.data(), "snare");
        e.rows[0] = 0x80 + 4 * 13;
        e.rows[1] = 0x80 + 4 * 13;
        e.rows[2] = 0x80 - 4 * 6;
        e.rows[3] = 0x80 - 4 * 11;
        e.rows[4] = 0x80 + 4 * 9;
        e.rows[5] = 0x80 + 4 * 13;
        e.length = 6;
        e.loop = 4;
    }

    // glide up
    {
        Effect& e = song.effects[GLIDE_UP];
        strcpy(e.name.data(), "glide up");
        e.rows[0] = 0x7c;
        e.rows[1] = 0x7d;
        e.rows[2] = 0x7e;
        e.rows[3] = 0x7f;
        e.rows[4] = 0x80;
        e.length = 5;
        e.loop = 4;
    }

    // vibrato
    {
        Effect& e = song.effects[VIBRATO];
        strcpy(e.name.data(), "vibrato");
        e.rows[0] = 0x80;
        e.rows[1] = 0x81;
        e.rows[2] = 0x82;
        e.rows[3] = 0x82;
        e.rows[4] = 0x81;
        e.rows[5] = 0x80;
        e.rows[6] = 0x7f;
        e.rows[7] = 0x7e;
        e.rows[8] = 0x7e;
        e.rows[9] = 0x7f;
        e.length = 10;
        e.loop = 0;
    }
}


bool load_song(Song& song, char const* name) {
    SDL_RWops* file = SDL_RWFromFile(name, "rb");
    if (!file) return false;
    song.tempo = SDL_ReadU8(file);
    song.swing = SDL_ReadU8(file);
    SDL_RWread(file, song.tracks.data(), sizeof(Track), song.tracks.size());
    SDL_RWread(file, song.instruments.data(), sizeof(Instrument), song.instruments.size());
    SDL_RWread(file, song.effects.data(), sizeof(Effect), song.effects.size());
    song.table_length = SDL_ReadLE16(file);
    SDL_RWread(file, song.table.data(), sizeof(Song::Block), song.table_length);
    SDL_RWclose(file);
    return true;
}


bool save_song(Song const& song, char const* name) {
    SDL_RWops* file = SDL_RWFromFile(name, "wb");
    if (!file) return false;
    SDL_WriteU8(file, song.tempo);
    SDL_WriteU8(file, song.swing);
    SDL_RWwrite(file, song.tracks.data(), sizeof(Track), song.tracks.size());
    SDL_RWwrite(file, song.instruments.data(), sizeof(Instrument), song.instruments.size());
    SDL_RWwrite(file, song.effects.data(), sizeof(Effect), song.effects.size());
    SDL_WriteLE16(file, song.table_length);
    SDL_RWwrite(file, song.table.data(), sizeof(Song::Block), song.table_length);
    SDL_RWclose(file);
    return true;
}
