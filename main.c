#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "usbdrv/usbdrv.h"
#include "lcd.h"
#include "hid_descriptor.h"
#include "keyboard.h"
#include "storage.h"
#include "config.h"

#define MENU			PD6
#define SELECT			PD5
#define CYCLE 			PD4

#define STATE_PRESSED 		0
#define STATE_RELEASED 		1
#define STATE_SEND 			2
#define STATE_DONE 			3

#define NUM_LOCK 1
#define CAPS_LOCK 2
#define SCROLL_LOCK 4

#define MODE_SEND			0
#define MODE_ADD			1
#define MODE_REMOVE			2
#define MODE_CHANGE			3
#define MODE_MENU			4

#define MENU_LENGTH			4

#define ESC					27
#define BACKSPACE			8

// The buffer needs to accommodate a password
#define MSG_BUFFER_SIZE 	(PASSWORD_MAX_LENGTH + 1)

#define MOD_SHIFT_LEFT (1<<1)

typedef struct {
	uint8_t modifier;
	uint8_t reserved;
	uint8_t keycode[6];
} keyboard_report_t;

// Store strings in program memory area
char string_1[] PROGMEM = "SEND PASS";
char string_2[] PROGMEM = "ADD PASS";
char string_3[] PROGMEM = "REMOVE PASS";
char string_4[] PROGMEM = "CHANGE PASS";
PGM_P menu_items[] PROGMEM =
{
	string_1,
	string_2,
	string_3,
	string_4,
};

static char** passwords;

static keyboard_report_t keyboard_report; // sent to PC
volatile static uchar LED_state = 0xff; // received from PC
static uchar idleRate; // repeat rate for keyboards

static uchar messageState = STATE_DONE;
static char stringBuffer[MSG_BUFFER_SIZE] = "";
static uchar messagePtr = 0;
static uchar messageCharNext = 1;

static uint8_t pass_no;

static uint8_t mode;

void init(void) {
	PORTD |= _BV(SELECT);
	PORTD |= _BV(CYCLE);
	PORTD |= _BV(MENU);

	DDRB = 1 << PB0; // PB0 as output

	kb_init();
	usbInit();
	// initialize LCD
	lcd_init(LCD_DISP_ON);
	lcd_puts("select..");
	_delay_ms(20);

}

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
	usbRequest_t *rq = (void *) data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
		switch (rq->bRequest) {
		case USBRQ_HID_GET_REPORT: // send "no keys pressed" if asked here
			// wValue: ReportType (highbyte), ReportID (lowbyte)
			usbMsgPtr = (void *) &keyboard_report; // we only have this one
			keyboard_report.modifier = 0;
			keyboard_report.keycode[0] = 0;
			return sizeof(keyboard_report);
		case USBRQ_HID_SET_REPORT: // if wLength == 1, should be LED state
			return (rq->wLength.word == 1) ? USB_NO_MSG : 0;
		case USBRQ_HID_GET_IDLE: // send idle rate to PC as required by spec
			usbMsgPtr = &idleRate;
			return 1;
		case USBRQ_HID_SET_IDLE: // save idle rate as required by spec
			idleRate = rq->wValue.bytes[1];
			return 0;
		}
	}

	return 0; // by default don't return any data
}

usbMsgLen_t usbFunctionWrite(uint8_t * data, uchar len) {
	if (data[0] == LED_state)
		return 1;
	else
		LED_state = data[0];

	// LED state changed
	if (LED_state & CAPS_LOCK)
		PORTB |= 1 << PB0; // LED on
	else
		PORTB &= ~(1 << PB0); // LED off

	return 1; // Data read, not expecting more
}

// The buildReport is called by main loop and it starts transmitting
// characters when messageState == STATE_SEND. The message is stored
// in messageBuffer and messagePtr tells the next character to send.
// Remember to reset messagePtr to 0 after populating the buffer!
uchar buildReport() {
	uchar ch;

	if (messageState == STATE_DONE || messagePtr >= sizeof(stringBuffer)
			|| stringBuffer[messagePtr] == 0) {
		keyboard_report.modifier = 0;
		keyboard_report.keycode[0] = 0;
		return STATE_DONE;
	}

	if (messageCharNext) { // send a keypress
		ch = stringBuffer[messagePtr++];

		// convert character to modifier + keycode
		if (ch >= '0' && ch <= '9') {
			keyboard_report.modifier = 0;
			keyboard_report.keycode[0] = (ch == '0') ? 39 : 30 + (ch - '1');
		} else if (ch >= 'a' && ch <= 'z') {
			keyboard_report.modifier = (LED_state & 2) ? MOD_SHIFT_LEFT : 0;
			keyboard_report.keycode[0] = 4 + (ch - 'a');
		} else if (ch >= 'A' && ch <= 'Z') {
			keyboard_report.modifier = (LED_state & 2) ? 0 : MOD_SHIFT_LEFT;
			keyboard_report.keycode[0] = 4 + (ch - 'A');
		} else {
			keyboard_report.modifier = 0;
			keyboard_report.keycode[0] = 0;
			switch (ch) {
			case '.':
				keyboard_report.keycode[0] = 0x37;
				break;
			case '_':
				keyboard_report.modifier = MOD_SHIFT_LEFT;
			case '-':
				keyboard_report.keycode[0] = 0x2D;
				break;
			case ' ':
				keyboard_report.keycode[0] = 0x2C;
				break;
			case '\t':
				keyboard_report.keycode[0] = 0x2B;
				break;
			case '\n':
				keyboard_report.keycode[0] = 0x28;
				break;
			}
		}
	} else { // key release before the next keypress!
		keyboard_report.modifier = 0;
		keyboard_report.keycode[0] = 0;
	}

	messageCharNext = !messageCharNext; // invert

	return STATE_SEND;
}

void toggle_led(uint8_t pin) {
	PORTB ^= _BV(pin);
}

uint8_t button_is_pressed(uint8_t pin, uint8_t button) {
	return (bit_is_clear(pin, button));

}

uint8_t poll_buttons() {

	static uint8_t menu_count = 0, select_count = 0, next_count = 0;
	static uint8_t menu_state = STATE_PRESSED, select_state = STATE_PRESSED,
			next_state = STATE_PRESSED;

	if (button_is_pressed(PIND, MENU)) {
		if (menu_state == STATE_RELEASED) {
			if (menu_count == DEBOUNCE_PERIOD) {
				menu_state = STATE_PRESSED;
				return MENU;
			} else {
				menu_count++;
			}
		}
	} else {
		menu_state = STATE_RELEASED;
		menu_count = 0;
	}

	if (button_is_pressed(PIND, SELECT)) {
		if (select_state == STATE_RELEASED) {
			if (select_count == DEBOUNCE_PERIOD) {
				select_state = STATE_PRESSED;
				return SELECT;
			} else {
				select_count++;
			}
		}
	} else {
		select_state = STATE_RELEASED;
		select_count = 0;
	}

	if (button_is_pressed(PIND, CYCLE)) {
		if (next_state == STATE_RELEASED) {
			if (next_count == DEBOUNCE_PERIOD) {
				next_state = STATE_PRESSED;
				return CYCLE;
			} else {
				next_count++;
			}
		}
	} else {
		next_state = STATE_RELEASED;
		next_count = 0;
	}

	return UINT8_MAX;
}

uint8_t lcd_backspace(uint8_t cnt) {
	if (cnt == 0) {
		return cnt;
	}

	if (cnt != LCD_DISP_LENGTH) {
		lcd_command(LCD_MOVE_CURSOR_LEFT);
		lcd_putc(' ');
		lcd_command(LCD_MOVE_CURSOR_LEFT);
	} else {
		lcd_gotoxy(LCD_DISP_LENGTH - 1, 0);
		lcd_putc(' ');
		lcd_gotoxy(LCD_DISP_LENGTH - 1, 0);
	}

	return cnt - 1;
}

uint8_t input_password() {
	uint8_t cnt = 0;
	uchar c;

	lcd_clrscr();

	kb_clear_buff();
	c = kb_get_char();
	while (c != '\r') {
		if (c == ESC) {
			return 0;
		} else if (c == BACKSPACE) {
			cnt = lcd_backspace(cnt);
		} else if ((c < 0x80) && (cnt < LCD_LINES * LCD_DISP_LENGTH)) {
			//If ASCII character is printable
			lcd_putc(c);
			stringBuffer[cnt++] = c;
		}
		c = kb_get_char();
	}
	return cnt;
}

int main() {
	uint8_t index;
	uint8_t menulen;
	uint8_t button_pressed = UINT8_MAX - 1;
	uint16_t i;
	uint8_t pass_len;

//	wdt_enable(WDTO_1S); // enable 1s watchdog timer

	init();

	for (i = 0; i < sizeof(keyboard_report); i++) // clear report initially
		((uchar *) &keyboard_report)[i] = 0;

	usbDeviceDisconnect(); // enforce re-enumeration
	for (i = 0; i < 250; i++) { // wait 500 ms
		wdt_reset();
		// keep the watchdog happy
		_delay_ms(2);
	}
	usbDeviceConnect();

	TCCR1B |= (1 << CS01); // timer 0 at clk/8 will generate randomness

	// Enable interrupts after re-enumeration
	sei();

//	eeprom_write_byte(0,0);

	pass_no = read_passwords(&passwords);
	index = 0;
	mode = MODE_MENU;
	menulen = MENU_LENGTH;

//	while (c = KbGetChar()) {
//		if (c == '\r') {
//			break;
//		}
//		lcd_putc(c);
//	}

	while (1) {
		wdt_reset();
		// keep the watchdog happy
		usbPoll();

		if (button_pressed != UINT8_MAX) {
			lcd_clrscr();
			if (mode == MODE_MENU) {
				strcpy_P(stringBuffer,
						(PGM_P) pgm_read_word(&(menu_items[index])));
				lcd_puts(stringBuffer);
			} else {
				lcd_puts(passwords[index]);
			}
		}

		button_pressed = poll_buttons();

		switch (button_pressed) {
		case MENU:

			mode = MODE_MENU;
			index = 0;
			menulen = MENU_LENGTH;

			toggle_led(PB0);
			break;

		case CYCLE:
			if (index < menulen - 1) {
				lcd_clrscr();
				index++;
			} else {
				index = 0;
			}
			toggle_led(PB0);
			break;

		case SELECT:
			if (mode == MODE_MENU) {
				if (index == MODE_ADD) {

					uint8_t pass_len;

					pass_len = input_password();

					if (pass_len > 0) {
						stringBuffer[pass_len] = '\0';

						passwords = realloc(passwords,
								(pass_no + 1) * sizeof(char*));
						passwords[pass_no] = malloc(pass_len * sizeof(char));
						strcpy(passwords[pass_no], stringBuffer);
						pass_no++;
						menulen = pass_no;
						write_passwords(pass_no, passwords);
					}
					toggle_led(PB0);
				} else {
					mode = index;
					index = 0;
					menulen = pass_no;
				}
			} else {
				//in SEND, REMOVE or CHANGE mode
				if (index < menulen) {
					switch (mode) {
					case MODE_SEND:
						strcpy(stringBuffer, passwords[index]);
						messagePtr = 0;
						messageState = STATE_SEND;
						break;

					case MODE_REMOVE:
						free(passwords[index]);
						for (i = index; i < pass_no - 1; i++) {
							passwords[i] = passwords[i + 1];
						}
						pass_no--;
						passwords = realloc(passwords, pass_no * sizeof(char*));
						index = 0;
						menulen = pass_no;
						write_passwords(pass_no, passwords);
						break;

					case MODE_CHANGE:
						pass_len = input_password();

						if (pass_len > 0) {
							stringBuffer[pass_len] = '\0';

							passwords[index] = realloc(passwords[index],
									pass_len);
							strcpy(passwords[index], stringBuffer);
							write_passwords(pass_no, passwords);
						}
//						mode = MODE_MENU;
						index = 0;
//						menulen = MENU_LENGTH;
						toggle_led(PB0);
						break;
					}
				}

			}
			break;
		}
		// characters are sent when messageState == STATE_SEND and after receiving
		// the initial LED state from PC (good way to wait until device is recognized)
		if (usbInterruptIsReady() && messageState == STATE_SEND
				&& LED_state != 0xff) {
			messageState = buildReport();
			usbSetInterrupt((void *) &keyboard_report, sizeof(keyboard_report));
		}
	}

	return 0;
}
