#ifndef _KEYBOARD_STATE_H
#define _KEYBOARD_STATE_H

#include <stdint.h>

class KeyboardState
{
public:
    uint32_t rate;
    uint32_t delay;

    bool pressed;       // Keyboard is pressed.
    uint32_t key;       // Pressed key.
    bool repeat;        // Keyboard is repeating.
    bool processed;     // Key processed first time.
    uint64_t time;
    uint64_t pressed_time;  // Key pressed time in milliseconds.
    uint64_t elapsed_time;  // Key elapsed time in milliseconds.

    bool repeating() const
    {
        if (pressed_time == 0) {
            return false;
        }
        return elapsed_time - pressed_time >= delay;
    }

    bool should_processed(uint32_t key)
    {
        if (this->key == key && (!this->processed || this->repeating())) {
            return true;
        }

        return false;
    }
};

#endif /* _KEYBOARD_STATE_H */
