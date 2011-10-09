#ifndef _AVR_CONFIG_H_
#define _AVR_CONFIG_H_

#include <avr/io.h>

/* ATMega8 - SPI Master Function */

#if defined (__AVR_ATmega8__)
/********************************************************************************************************************/
#define SPI_DDR	    		DDRB
#define SPI_PIN_MOSI    	PB3
#define SPI_PIN_SCK			PB5
#define SPI_PIN_SS			PB2
#define SPI_PIN_MISO        PB4
/********************************************************************************************************************/
#endif

#endif // _AVR_CONFIG_H_
