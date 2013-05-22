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
	//edge = 0;
	GICR |= (1 << INT2); //enable INT2 interrupt
	MCUCSR = (0 << ISC2);   // INT2 interrupt on falling edge
}

void kb_init(void)
{
	ps2_init();
	kb_clear_buff();
}

void kb_clear_buff(void) {
	inpt =  kb_buffer;                        // Initialize buffer
	outpt = kb_buffer;
	buffcnt=0;
}

void decode(unsigned char sc)
{
    static unsigned char is_up = 0, shift = 0, ext=0;
    unsigned char i;

    if (!is_up)                // previous data received was the up-key identifier
    {
        switch (sc)
        {
          case 0xF0 :        // The up-key identifier
            is_up = 1;
            break;

		  case 0xE0:		//do a lookup of extended keys
			ext = 1;
			break;

          case 0x12 :        // Left SHIFT
            shift = 1;
            break;

          case 0x59 :        // Right SHIFT
            shift = 1;
            break;

          default:

			if(ext) { //extended key lookup

				for(i = 0; (pgm_read_byte(&extended[i][0])!=sc) && pgm_read_byte(&extended[i][0]); i++)
					;
				if (pgm_read_byte(&extended[i][0]) == sc)
					//ext_char = pgm_read_byte(&extended[i][1]);
					put_kbbuff(pgm_read_byte(&extended[i][1]));
			}
			else {

				if(!shift)           // If shift not pressed, do a table look-up
				{
						for(i = 0; (pgm_read_byte(&unshifted[i][0])!=sc) && pgm_read_byte(&unshifted[i][0]); i++)
							;
						if (pgm_read_byte(&unshifted[i][0])== sc)
							put_kbbuff(pgm_read_byte(&unshifted[i][1]));

				}
				else {               // If shift pressed

						for(i = 0; (pgm_read_byte(&shifted[i][0])!=sc) && pgm_read_byte(&shifted[i][0]); i++)
							;
						if (pgm_read_byte(&shifted[i][0])== sc)
							put_kbbuff(pgm_read_byte(&shifted[i][1]));

				}


			}

        }

    }

	else {			// is_up = 1

		is_up = 0;  // Two 0xF0 in a row not allowed
		ext=0;
        switch (sc)
        {
          case 0x12 :                        // Left SHIFT
            shift = 0;
            break;

          case 0x59 :                        // Right SHIFT
            shift = 0;
            break;

        }
    }

}

static void put_kbbuff(unsigned char c)
{
    if (buffcnt<BUFF_SIZE)                        // If buffer not full
    {
        *inpt = c;                                // Put character into buffer
        inpt++;                                    // Increment pointer

        buffcnt++;

        if (inpt >= (kb_buffer + BUFF_SIZE))        // Pointer wrapping
            inpt = kb_buffer;
    }
}


uint8_t kb_get_char(void)
{
    uint8_t byte;
    while(buffcnt == 0);                        // Wait for data

    byte = *outpt;                                // Get byte
    outpt++;                                    // Increment pointer

    if ( outpt >= (kb_buffer + BUFF_SIZE) )            // Pointer wrapping
        outpt = kb_buffer;

    buffcnt--;                                    // Decrement buffer count

    return byte;

}



ISR(INT2_vect) {
	static uint8_t byteIn;

	sei();

	if (bitcount > 2 && bitcount < 11) {

		byteIn = (byteIn >> 1);
		if (PIN_DATA & (1 << DATA_PIN))
			byteIn |= 0x80;
	}

	if (--bitcount == 0) {

		decode(byteIn);
		bitcount = 11;
	}
}
