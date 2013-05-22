#ifndef STORAGE_H_
#define STORAGE_H_

uint16_t eeprom_write_string(char* s, uint8_t* addr);

uint16_t eeprom_read_string(char* s, uint8_t* addr);

uint8_t read_passwords(char*** passwords);

void write_passwords(uint8_t len, char** sarray);

#endif /* STORAGE_H_ */
