/*
 * config.h
 *
 *  Created on: 22 May 2013
 *      Author: addu
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define F_CPU 12000000L

//Address in the eeprom to start storing the password
#define EEPROM_START_ADDRESS	0
//This is a hash code kept in the eeprom to confirm an eeprom valid state
#define EEPROM_HASH				0xAA

#define DEBOUNCE_PERIOD			200

#define PASSWORD_MAX_LENGTH		16

#endif /* CONFIG_H_ */
