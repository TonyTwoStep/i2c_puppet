#include "usb.h"

#include "backlight.h"
#include "keyboard.h"
#include "touchpad.h"
#include "reg.h"

#include <hardware/irq.h>
#include <pico/mutex.h>
#include <tusb.h>

#define USB_LOW_PRIORITY_IRQ	31
#define USB_TASK_INTERVAL_US	1000

static struct
{
	mutex_t mutex;
// 	bool mouse_moved;
	uint8_t mouse_btn;

	uint8_t write_buffer[2];
	uint8_t write_len;
} self;

// TODO: What about Ctrl?
// TODO: What should L1, L2, R1, R2 do
// TODO: Should touch send arrow keys as an option?

// conv_table needs to be 256 entries long because the special keys are in range 128-256 (0x80 - 0xFF)
static uint8_t conv_table[256][2] = { HID_ASCII_TO_KEYCODE };

static void low_priority_worker_irq(void)
{
	if (mutex_try_enter(&self.mutex, NULL)) {
		tud_task();

		mutex_exit(&self.mutex);
	}
}

static int64_t timer_task(alarm_id_t id, void *user_data)
{
	(void)id;
	(void)user_data;

	irq_set_pending(USB_LOW_PRIORITY_IRQ);

	return USB_TASK_INTERVAL_US;
}

static void key_cb(char key, enum key_state state)
{
	// Don't send mods over USB
	if ((key == KEY_MOD_SHL) ||
		(key == KEY_MOD_SHR) ||
		(key == KEY_MOD_ALT) ||
		(key == KEY_MOD_SYM))
		return;

	if (tud_hid_n_ready(USB_ITF_KEYBOARD) && reg_is_bit_set(REG_ID_CF2, CF2_USB_KEYB_ON)) {
		uint8_t keycode[6] = { 0 };
		uint8_t modifier   = 0;

		if (state == KEY_STATE_PRESSED) {
			printf(" conv_table[%d][0,1]=%d,%d  ",key,  conv_table[(int)key][0], conv_table[(int)key][1]); // bgb
			if (conv_table[(int)key][0]) {
				modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
			} else if (key < 0x20) { // it's a control key
				// convert control key, i.e. [Control-A] converts to [modifier=control, key=A]
				modifier=KEYBOARD_MODIFIER_RIGHTCTRL;
				key=key + 0x40;
			} else if ((key >= KEY_CAF10) && (key <= KEY_CAF9)) {
			  modifier = KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_LEFTCTRL ;
			}
			keycode[0] = conv_table[(int)key][1];
		}

		if (state != KEY_STATE_HOLD)
			tud_hid_n_keyboard_report(USB_ITF_KEYBOARD, 0, modifier, keycode);
	}

	if (tud_hid_n_ready(USB_ITF_MOUSE) && reg_is_bit_set(REG_ID_CF2, CF2_USB_MOUSE_ON)) {
		if (key == KEY_JOY_CENTER) {
			if (state == KEY_STATE_PRESSED) {
				self.mouse_btn = MOUSE_BUTTON_LEFT;
				tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, MOUSE_BUTTON_LEFT, 0, 0, 0, 0);
			} else if (state == KEY_STATE_RELEASED) {
				self.mouse_btn = 0x00;
				tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, 0x00, 0, 0, 0, 0);
			}
		} else if (key == KEY_MOUSE2) {
			if (state == KEY_STATE_PRESSED) {
				self.mouse_btn = MOUSE_BUTTON_RIGHT;
				tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, MOUSE_BUTTON_RIGHT, 0, 0, 0, 0);
			} else if (state == KEY_STATE_RELEASED) {
				self.mouse_btn = 0x00;
				tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, 0x00, 0, 0, 0, 0);
			}
		} else if (key == KEY_MOUSE3) {
			if (state == KEY_STATE_PRESSED) {
				self.mouse_btn = MOUSE_BUTTON_MIDDLE;
				tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, MOUSE_BUTTON_MIDDLE, 0, 0, 0, 0);
			} else if (state == KEY_STATE_RELEASED) {
				self.mouse_btn = 0x00;
				tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, 0x00, 0, 0, 0, 0);
			}
		}
	}
}
static struct key_callback key_callback = { .func = key_cb };

static void touch_cb(int8_t x, int8_t y)
{
	if (!tud_hid_n_ready(USB_ITF_MOUSE) || !reg_is_bit_set(REG_ID_CF2, CF2_USB_MOUSE_ON))
		return;

// 	self.mouse_moved = true;

	tud_hid_n_mouse_report(USB_ITF_MOUSE, 0, self.mouse_btn, x, y, 0, 0);
}
static struct touch_callback touch_callback = { .func = touch_cb };

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
	// TODO not Implemented
	(void)itf;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)reqlen;

	return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t len)
{
	// TODO set LED based on CAPLOCK, NUMLOCK etc...
	(void)itf;
	(void)report_id;
	(void)report_type;
	(void)buffer;
	(void)len;
}

void tud_vendor_rx_cb(uint8_t itf)
{
//	printf("%s: itf: %d, avail: %d\r\n", __func__, itf, tud_vendor_n_available(itf));

	uint8_t buff[64] = { 0 };
	tud_vendor_n_read(itf, buff, 64);
//	printf("%s: %02X %02X %02X\r\n", __func__, buff[0], buff[1], buff[2]);

	reg_process_packet(buff[0], buff[1], self.write_buffer, &self.write_len);

	tud_vendor_n_write(itf, self.write_buffer, self.write_len);
}

void tud_mount_cb(void)
{
	// Send mods over USB by default if USB connected
	reg_set_value(REG_ID_CFG, reg_get_value(REG_ID_CFG) | CFG_REPORT_MODS);
}

mutex_t *usb_get_mutex(void)
{
	return &self.mutex;
}

void usb_init(void)
{
	tusb_init();

	conv_table[KEY_ENTER][1]     = HID_KEY_ENTER;
	conv_table[KEY_BS][1]        = HID_KEY_BACKSPACE;
	conv_table[KEY_JOY_UP][1]    = HID_KEY_ARROW_UP;
	conv_table[KEY_JOY_DOWN][1]  = HID_KEY_ARROW_DOWN;
	conv_table[KEY_JOY_LEFT][1]  = HID_KEY_ARROW_LEFT;
	conv_table[KEY_JOY_RIGHT][1] = HID_KEY_ARROW_RIGHT;

	conv_table[KEY_HOME][1]      = HID_KEY_HOME;
	conv_table[KEY_END][1]       = HID_KEY_END;
	conv_table[KEY_PAGE_UP][1]   = HID_KEY_PAGE_UP;
	conv_table[KEY_PAGE_DOWN][1] = HID_KEY_PAGE_DOWN;

	conv_table[KEY_GUI][1]       = HID_KEY_GUI_LEFT;
	conv_table[KEY_APP][1]       = HID_KEY_APPLICATION;
	conv_table[KEY_MENU][1]      = HID_KEY_MENU;

	conv_table[KEY_PWR][1]       = HID_KEY_POWER;

	conv_table[KEY_ESCAPE][1]    = HID_KEY_ESCAPE;
	conv_table[KEY_DEL][1]       = HID_KEY_DELETE;
	conv_table[KEY_TAB][1]       = HID_KEY_TAB;
	conv_table[KEY_ENTER][1]     = HID_KEY_ENTER;
	conv_table[KEY_RETURN][1]    = HID_KEY_RETURN;

	conv_table[KEY_F1][1]        = HID_KEY_F1;
	conv_table[KEY_F2][1]        = HID_KEY_F2;
	conv_table[KEY_F3][1]        = HID_KEY_F3;
	conv_table[KEY_F4][1]        = HID_KEY_F4;
	conv_table[KEY_F5][1]        = HID_KEY_F5;
	conv_table[KEY_F6][1]        = HID_KEY_F6;
	conv_table[KEY_F7][1]        = HID_KEY_F7;
	conv_table[KEY_F8][1]        = HID_KEY_F8;
	conv_table[KEY_F9][1]        = HID_KEY_F9;
	conv_table[KEY_F10][1]       = HID_KEY_F10;

	conv_table[KEY_CAF1][1]      = HID_KEY_F1;
	conv_table[KEY_CAF2][1]      = HID_KEY_F2;
	conv_table[KEY_CAF3][1]      = HID_KEY_F3;
	conv_table[KEY_CAF4][1]      = HID_KEY_F4;
	conv_table[KEY_CAF5][1]      = HID_KEY_F5;
	conv_table[KEY_CAF6][1]      = HID_KEY_F6;
	conv_table[KEY_CAF7][1]      = HID_KEY_F7;
	conv_table[KEY_CAF8][1]      = HID_KEY_F8;
	conv_table[KEY_CAF9][1]      = HID_KEY_F9;
	conv_table[KEY_CAF10][1]     = HID_KEY_F10;

	keyboard_add_key_callback(&key_callback);

	touchpad_add_touch_callback(&touch_callback);

	// create a new interrupt that calls tud_task, and trigger that interrupt from a timer
	irq_set_exclusive_handler(USB_LOW_PRIORITY_IRQ, low_priority_worker_irq);
	irq_set_enabled(USB_LOW_PRIORITY_IRQ, true);

	mutex_init(&self.mutex);
	add_alarm_in_us(USB_TASK_INTERVAL_US, timer_task, NULL, true);
}
