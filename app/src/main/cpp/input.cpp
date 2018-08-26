#include "input.hpp"
#include "gfx.hpp"


namespace input {
namespace {


std::array<Touch, TOUCH_COUNT> m_touches;


Touch* find_touch(int id) {
    for(Touch& t : m_touches) if (t.id == id) return &t;
    return nullptr;
}


} // namespace


Touch const& touch(int index) {
    return m_touches[index];
}


void post_update() {
    for (Touch& t : m_touches) {
        if (t.state == Touch::JUST_PRESSED)  t.state = Touch::PRESSED;
        if (t.state == Touch::JUST_RELEASED) t.state = Touch::RELEASED;
    }
}


void finger_down(int id, glm::ivec2 const& pos) {
    for (int i = 0; i < TOUCH_COUNT; ++i) {
        Touch& t = m_touches[i];
        if (t.id == -1) {
            t.id = id;
            t.pos = pos;
            t.state = Touch::JUST_PRESSED;
            break;
        }
    }
}


void finger_up(int id, glm::ivec2 const& pos) {
    if (Touch* t = find_touch(id)) {
        t->id = -1;
        t->mov = {};
        t->state = Touch::JUST_RELEASED;
    }
}


void finger_motion(int id, glm::ivec2 const& pos) {
    if (Touch* t = find_touch(id)) {
        t->mov += pos - t->pos;
        //t->mov = glm::clamp(t->mov, glm::ivec2(-5, -5), glm::ivec2(5, 5));
        t->pos = pos;
    }
}


} // namespace
