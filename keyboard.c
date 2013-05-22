#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "scancodes.h"
#include "keyboard.h"

#define BUFF_SIZE 16
#define DDR_CLOCK DDRB
#define PORT_CLOCK PORTB
#define PIN_CLOCK PINB
#define CLOCK_PIN 2

#define DDR_DATA DDRB
#define PORT_DATA PORTB
#define PIN_DATA PINB
#define DATA_PIN 1

#define TRUE 1
#define FALSE 0

static void put_kbbuff(unsigned char c);

static volatile uint8_t bitcount, toDevice;
static volatile uint8_t buffcnt = 0;
static uint8_t kb_buffer[BUFF_SIZE];
static uint8_t *inpt, *outpt;

void ps2_init(void) {
	bitcount = 11;
	toDevice = FALSE;
	//enable INT2 interrupt
	GICR |= (1 << INT2);
	// INT2 interrupt on falling edge
	MCUCSR = (0 << ISC2);
}

void kb_init(void) {
	ps2_init();
	kb_clear_buff();
}

// Initialize buffer
void kb_clear_buff(void) {
	inpt = kb_buffer;
	outpt = kb_buffer;
	buffcnt = 0;
}

void decode(unsigned char sc) {
	static unsigned char is_up = 0, shift = 0, ext = 0;
	unsigned char i;

	// previous data received was the up-key identifier
	if (!is_up) {
		switch (sc) {
		// The up-key identifier
		case 0xF0:
			is_up = 1;
			break;

			//do a lookup of extended keys
		case 0xE0:
			ext = 1;
			break;

			// Left SHIFT
		case 0x12:
			shift = 1;
			break;

			// Right SHIFT
		case 0x59:
			shift = 1;
			break;

		default:
			//extended key lookup
			if (ext) {

				for (i = 0;
						(pgm_read_byte(&extended[i][0]) != sc)
								&& pgm_read_byte(&extended[i][0]); i++)
					;
				if (pgm_read_byte(&extended[i][0]) == sc)
					put_kbbuff(pgm_read_byte(&extended[i][1]));
			} else {

				// If shift not pressed, do a table look-up
				if (!shift) {
					for (i = 0;
							(pgm_read_byte(&unshifted[i][0]) != sc)
									&& pgm_read_byte(&unshifted[i][0]); i++)
						;
					if (pgm_read_byte(&unshifted[i][0]) == sc)
						put_kbbuff(pgm_read_byte(&unshifted[i][1]));

				} else {

					// If shift pressed
					for (i = 0;
							(pgm_read_byte(&shifted[i][0]) != sc)
									&& pgm_read_byte(&shifted[i][0]); i++)
						;
					if (pgm_read_byte(&shifted[i][0]) == sc)
						put_kbbuff(pgm_read_byte(&shifted[i][1]));

				}

			}

		}

	}
	// is_up = 1
	else {
		// Two 0xF0 in a row not allowed
		is_up = 0;
		ext = 0;
		switch (sc) {

		// Left SHIFT
		case 0x12:
			shift = 0;
			break;

			// Right SHIFT
		case 0x59:
			shift = 0;
			break;

		}
	}

}

static void put_kbbuff(unsigned char c) {
	// If buffer not full
	if (buffcnt < BUFF_SIZE) {
		// Put character into buffer
		*inpt = c;
		// Increment pointer
		inpt++;

		buffcnt++;

		// Pointer wrapping
		if (inpt >= (kb_buffer + BUFF_SIZE))
			inpt = kb_buffer;
	}
}

uint8_t kb_get_char(void) {
	uint8_t byte;
	// Wait for data
	while (buffcnt == 0)
		;

	// Get byte
	byte = *outpt;
	// Increment pointer
	outpt++;

	// Pointer wrapping
	if (outpt >= (kb_buffer + BUFF_SIZE))
		outpt = kb_buffer;

	// Decrement buffer count
	buffcnt--;

	return byte;

}

ISR(INT2_vect) {
	static uint8_t byteIn;

	sei();

	//If data bit
	if (bitcount > 2 && bitcount < 11) {
		byteIn = (byteIn >> 1);
		if (PIN_DATA & (1 << DATA_PIN))
			byteIn |= 0x80;
	}

	//If scancode transfer complete
	if (--bitcount == 0) {
		decode(byteIn);
		bitcount = 11;
	}
}
