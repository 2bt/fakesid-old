#pragma once
#include <glm.hpp>
#include <SDL.h>


namespace input {

    void finger_down(int id, glm::ivec2 const& pos);
    void finger_up(int id, glm::ivec2 const& pos);
    void finger_motion(int id, glm::ivec2 const& pos);
    void post_update();

    struct Touch {
        enum State {
            RELEASED, JUST_PRESSED, PRESSED, JUST_RELEASED
        };

        int        id = -1;
        glm::ivec2 pos;
        glm::ivec2 mov;
        State      state;
    };

    enum { TOUCH_COUNT = 2 };
    Touch const& touch(int index);


    inline bool just_pressed() {
        return touch(0).state == Touch::JUST_PRESSED;
    }
    inline bool just_released() {
        return touch(0).state == Touch::JUST_RELEASED;
    }
    inline bool released() {
        return touch(0).state == Touch::RELEASED;
    }
}
