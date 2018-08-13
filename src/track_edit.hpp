#pragma once
#include <cstdint>

void    enter_track_select();
void    enter_track_select(uint8_t& dst);
void    sprint_track_id(char* dst, int nr);
bool    draw_track_select();
void    draw_track_view();
void    select_track(uint8_t t);
uint8_t selected_track();
