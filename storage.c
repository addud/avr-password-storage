#include <avr/eeprom.h>
#include "config.h"
#include "storage.h"

uint16_t eeprom_write_string(char* s, uint8_t* addr) {
	uint16_t cnt = 0;
	do {
		eeprom_busy_wait();
		eeprom_write_byte(addr++, *s);
		cnt++;
	} while (*s++ != '\0');
	return cnt;
}

uint16_t eeprom_read_string(char* s, uint8_t* addr) {
	uint16_t cnt = 0;
	do {
		eeprom_busy_wait();
		*s = eeprom_read_byte(addr++);
		cnt++;
//		lcd_putc(*s);
	} while (*s++ != '\0');
	return cnt;
}

uint8_t read_passwords(char*** passwords) {
	uint16_t addr = 0;
	uint16_t i;
	uint8_t len;
	uint8_t hash;

	eeprom_busy_wait();
	hash = eeprom_read_byte((uint8_t*) (addr++));

	if (hash != EEPROM_HASH) {
//		lcd_clrscr();
//		lcd_puts("EEPROM corrupt");
		return 0;
	}

	eeprom_busy_wait();
	len = eeprom_read_byte((uint8_t*) (addr++));

	*passwords = malloc(len * sizeof(char*));

	for (i = 0; i < len; i++) {
		eeprom_busy_wait();
		uint8_t nr = eeprom_read_byte((uint8_t*) (addr++));

		(*passwords)[i] = malloc(nr * sizeof(char));
		nr = eeprom_read_string((*passwords)[i], (uint8_t*) addr);
		lcd_puts((*passwords)[i]);
		addr += nr;
	}

	return len;
}

void write_passwords(uint8_t len, char** sarray) {
	uint16_t addr = 0;
	uint16_t i;

	eeprom_busy_wait();
	eeprom_write_byte((uint8_t*) (addr++), EEPROM_HASH);

	eeprom_busy_wait();
	eeprom_write_byte((uint8_t*) (addr++), len);

	for (i = 0; i < len; i++) {
		uint8_t nr = eeprom_write_string(sarray[i], (uint8_t*) (addr + 1));
		eeprom_busy_wait();
		eeprom_write_byte((uint8_t*) addr, nr);
		addr += 1 + nr;
	}
}
