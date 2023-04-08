#pragma once

#include <stdbool.h>
#include <stdint.h>

enum key_state
{
	KEY_STATE_IDLE = 0,
	KEY_STATE_PRESSED,
	KEY_STATE_HOLD,
	KEY_STATE_RELEASED,
};

enum key_mod
{
	KEY_MOD_ID_NONE = 0,
	KEY_MOD_ID_SYM,
	KEY_MOD_ID_ALT,
	KEY_MOD_ID_SHL,
	KEY_MOD_ID_SHR,

	KEY_MOD_ID_LAST,
};

#define KEY_JOY_UP     0x81
#define KEY_JOY_DOWN   0x82
#define KEY_JOY_LEFT   0x83
#define KEY_JOY_RIGHT  0x84
#define KEY_JOY_CENTER 0x85

#define KEY_BTN_LEFT1  0x86
#define KEY_BTN_RIGHT1 0x87
#define KEY_BTN_LEFT2  0x88
#define KEY_BTN_RIGHT2 0x89

#define KEY_ESCAPE     0x90
// 0x08 - BACKSPACE
// 0x09 - TAB
// 0x0A - NEW LINE
// 0x0D - CARRIAGE RETURN

#define KEY_MOD_ALT    0x9A
#define KEY_MOD_SHL    0x9B // Left Shift
#define KEY_MOD_SHR    0x9C // Right Shift
#define KEY_MOD_SYM    0x9D

#define KEY_GUI        0xA0
#define KEY_APP        0xA1
#define KEY_MENU       0xA2

#define KEY_PWR        0xA8

#define KEY_PAGE_UP    0xB2
#define KEY_PAGE_DOWN  0xB3
#define KEY_HOME       0xB4
#define KEY_END        0xB5

#define KEY_F10        0xC0
#define KEY_F1         0xC1
#define KEY_F2         0xC2
#define KEY_F3         0xC3
#define KEY_F4         0xC4
#define KEY_F5         0xC5
#define KEY_F6         0xC6
#define KEY_F7         0xC7
#define KEY_F8         0xC8
#define KEY_F9         0xC9

#define KEY_CAF10      0xD0
#define KEY_CAF1       0xD1
#define KEY_CAF2       0xD2
#define KEY_CAF3       0xD3
#define KEY_CAF4       0xD4
#define KEY_CAF5       0xD5
#define KEY_CAF6       0xD6
#define KEY_CAF7       0xD7
#define KEY_CAF8       0xD8
#define KEY_CAF9       0xD9

struct key_callback
{
	void (*func)(char, enum key_state);
	struct key_callback *next;
};

struct key_lock_callback
{
	void (*func)(bool, bool);
	struct key_lock_callback *next;
};

void keyboard_inject_event(char key, enum key_state state);

bool keyboard_is_key_down(char key);
bool keyboard_is_mod_on(enum key_mod mod);

void keyboard_add_key_callback(struct key_callback *callback);
void keyboard_add_lock_callback(struct key_lock_callback *callback);

bool keyboard_get_capslock(void);
bool keyboard_get_numlock(void);

void keyboard_init(void);
