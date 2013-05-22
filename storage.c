#include <stdlib.h>
#include <avr/eeprom.h>
#include "config.h"
#include "storage.h"

//Writes a null terminated string to EEPROM character by character
uint16_t eeprom_write_string(char* s, uint8_t* addr) {
	uint16_t cnt = 0;
	do {
		eeprom_busy_wait();
		eeprom_update_byte(addr++, *s);
		cnt++;
	} while (*s++ != '\0');
	return cnt;
}

//Reads a null terminated string from EEPROM
uint16_t eeprom_read_string(char* s, uint8_t* addr) {
	uint16_t cnt = 0;
	do {
		eeprom_busy_wait();
		*s = eeprom_read_byte(addr++);
		cnt++;
	} while (*s++ != '\0');
	return cnt;
}

uint8_t read_passwords(char*** passwords) {
	uint16_t addr = 0;
	uint16_t i;
	uint8_t len;
	uint8_t hash;

	//Read hash value
	eeprom_busy_wait();
	hash = eeprom_read_byte((uint8_t*) (addr++));

	if (hash != EEPROM_HASH) {
		//EEPROM is corrupt
		//Consider no passwords stored
		return 0;
	}

	//Read total number of passwords
	eeprom_busy_wait();
	len = eeprom_read_byte((uint8_t*) (addr++));

	*passwords = malloc(len * sizeof(char*));

	for (i = 0; i < len; i++) {
		//Read number of characters in the password
		eeprom_busy_wait();
		uint8_t nr = eeprom_read_byte((uint8_t*) (addr++));

		//Read the password itself
		(*passwords)[i] = malloc(nr * sizeof(char));
		nr = eeprom_read_string((*passwords)[i], (uint8_t*) addr);
		addr += nr;
	}

	return len;
}

void write_passwords(uint8_t len, char** sarray) {
	uint16_t addr = 0;
	uint16_t i;

	//Write fixed hash value
	eeprom_busy_wait();
	eeprom_update_byte((uint8_t*) (addr++), EEPROM_HASH);

	//Write total number of passwords
	eeprom_busy_wait();
	eeprom_update_byte((uint8_t*) (addr++), len);

	for (i = 0; i < len; i++) {
		//Write number of characters in the password
		uint8_t nr = eeprom_write_string(sarray[i], (uint8_t*) (addr + 1));
		//Write the password itself
		eeprom_busy_wait();
		eeprom_update_byte((uint8_t*) addr, nr);
		addr += 1 + nr;
	}
}
