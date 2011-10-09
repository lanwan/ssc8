/*
 * AVR helper functions
 *
 * Copyright (C) 2009 LDROBOT Workshop
 *
 * version 1.0.0.1
 * create date: 2009-6-17
 * author: steven.zdwang@gmail.com
 *
 */

#ifndef _AVR_MODULES_H_
#define _AVR_MODULES_H_

# define F_CPU 8000000UL

#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>
#include "avr_config.h"

/**
 * AVR ASM functions
 */
#define ASM_NOP()	asm volatile ("nop")

/**
 * HELP Macro
 */
#define STREQ(a, b) ( (*a) == (*b) && strcmp((a), (b)) == 0 )

#define SIZEOF(T)       (sizeof(T)/sizeof(T[0]))

#define TRUE		1
#define FALSE		0

/**
 * Debug functions
 */

//#define AVR_DEBUG	1
//#if AVR_DEBUG
//#define AVR_DEBUG_PRING(x)		printf(x)
//#else
//#define AVR_DEBUG_PRING(x)
//#endif

/**
 * UART FUNCTIONS
 */

#define USE_AVR_UART_MODULE		1

#define USE_AVR_UART_IO			1
#define USE_AVR_UART_RW_BUFFER	0
#define USE_AVR_UART_C_IO		1

#if USE_AVR_UART_MODULE

void avr_uart_init(int16_t ubrr_val);
#if USE_AVR_UART_IO
void avr_uart_putc(uint8_t val);
uint8_t avr_uart_getc(void);
#endif

#if USE_AVR_UART_RW_BUFFER
void avr_uart_write(const char* buf, uint16_t len);
#endif

// c language
#if USE_AVR_UART_C_IO
int avr_uart_putchar(char c, FILE *unused);
int avr_uart_getchar(FILE *stream);
#endif // USE_AVR_UART_C_IO

#endif /* USE_AVR_UART_MODULE */

/**
 * SPI FUNCTIONS
 */

#define USE_AVR_SPI_MODULE		0

#if USE_AVR_SPI_MODULE
void avr_spi_master_init(uint8_t val);
uint8_t avr_spi_master_transmit_byte(uint8_t val);
uint16_t avr_spi_master_transmit_word(uint16_t val);
void avr_spi_slave_init(void);
uint8_t avr_spi_slave_receive(void);

#define AVR_SPI_MASTER_TRANSMIT(val) do{ SPDR = val; while(!(SPSR & (1<<SPIF)));} while(0)
#define AVR_SPI_MASTER_TRANSMIT_NO_WAIT(val) {SPDR = val; ASM_NOP(); ASM_NOP(); ASM_NOP(); ASM_NOP(); }

#endif /* USE_AVR_SPI_MODULE */


/**
 *	TWI FUNCTIONS
 *	TWI可以工作于4种不同的模式，即主机发送模式(MT)、主机接收模式(MR)、从机发送模式(ST)和从机接收器模式(SR)
 */
#define USE_AVR_TWI_MODULE		1

#define USE_AVR_TWI_EEPROM      0
#define USE_AVR_TWI_BUS			0
#define USE_AVR_TWI_INT			0
#define USE_AVR_TWI_MEM			0
#define USE_AVR_TWI_CY2239X             1

#if USE_AVR_TWI_MODULE

#if USE_AVR_TWI_EEPROM
  void avr_ee24xx_init(void);
  int avr_ee24xx_write_buffer(uint16_t eeaddr, uint8_t len, uint8_t* buf);
  int avr_ee24xx_read_buffer(uint16_t eeaddr, int len, uint8_t *buf);
#endif //USE_AVR_TWI_EEPROM

#if USE_AVR_TWI_CY2239X
  void avr_cy2239x_init(void);
  int avr_cy2239x_read_buffer(uint8_t addr, int len, uint8_t *buf);
  int avr_cy2239x_write_buffer(uint8_t addr, uint8_t len, uint8_t* buf);
#endif

#if USE_AVR_TWI_BUS

// TWI Bus start
uint8_t avr_twi_start(void);
// TWI Bus stop
void avr_twi_stop(void);
// write byte to TWI bus
uint8_t avr_twi_writebyte(uint8_t c);
// read byte from TWI bus
uint8_t avr_twi_readbyte(uint8_t *c, uint8_t ack);


typedef void(*avr_twi_slave_listen_callback_t)(uint8_t twi_status, uint8_t twi_data);
void avr_twi_slave_init(uint8_t slave_device_addr, avr_twi_slave_listen_callback_t twi_slave_callback);
#if USE_AVR_TWI_INT
	// use TWI interrupt
#else
void avr_twi_slave_loop(void);
#endif

#if USE_AVR_TWI_MEM
	// read and write a block memory device
	#define AVR_TWI_BLOCK_MEM_DEVICE_ADDR	0x00
	uint8_t avr_twi_block_read(uint8_t mem_addr, uint8_t *buf, uint8_t len);
	uint8_t avr_twi_block_write(uint8_t mem_addr, uint8_t *buf, uint8_t len);
#endif	// USE_AVR_TWI_MEM

#endif // USE_AVR_TWI_BUS

#endif  /* USE_AVR_TWI_MODULE */

/**
 * math & arithmetic FUNCTIONS
 */

#define USE_AVR_MATH_MODULE			1
#define USE_AVR_MATH_SELECT_SORT	0
#define USE_AVR_MATH_EACH_ADD		0
#define USE_AVR_MATH_MAVERAGE		1

#if USE_AVR_MATH_MODULE
#if USE_AVR_MATH_SELECT_SORT
void avr_math_select_sort(uint16_t* start, uint16_t* stop);
#endif //USE_AVR_MATH_SELECT_SORT
#if USE_AVR_MATH_EACH_ADD
void avr_math_each_add(uint16_t* start, uint16_t* stop, uint16_t data);
#endif // USE_AVR_MATH_EACH_ADD
#if USE_AVR_MATH_MAVERAGE
uint16_t avr_math_maverage(uint16_t* start, uint16_t* stop);
#endif //USE_AVR_MATH_MAVERAGE
#endif /* USE_AVR_MATH_MODULE */


/**
 * ADC Module
 */
#define USE_AVR_ADC_MODULE			1
#if USE_AVR_MATH_MODULE

#define AVR_ADC_AREF_SRC_EXTERNAL		0
#define AVR_ADC_AREF_SRC_VCC			1
#define AVR_ADC_AREF_SRC_INNER2_5V		3

#define AVR_ADC_AREF_SRC_COVERT_COUNT	5
int8_t avr_adc_open(int8_t aref, int8_t channel);
uint16_t avr_adc_read(uint8_t channel);
void avr_adc_close(void);

#endif /* USE_AVR_MATH_MODULE */


#endif //_AVR_MODULES_H_
