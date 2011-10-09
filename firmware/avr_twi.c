/*
 * avr_twi.c
 *
 *  Created on: 2009-6-25
 *      Author: Steven
 */

#include "avr_modules.h"

#if USE_AVR_TWI_MODULE

#define DEBUG		1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#define TWI_BUS_START()                  ( TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) )
#define TWI_BUS_STOP()                   ( TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN) )
#define TWI_BUS_WAIT()                   { while ((TWCR & _BV(TWINT)) == 0); }
#define TWI_BUS_STATUS                   TW_STATUS
#define TWI_BUS_SET_ACK()                ( TWCR |= _BV(TWEA) )
#define TWI_BUS_SET_NACK()               ( TWCR &= ~_BV(TWEA) )
#define TWI_BUS_START_READ()             ( TWCR = (_BV(TWINT) | _BV(TWEN)) )
#define TWI_BUS_WRITE_BYTE(x)            { TWDR = (x); TWCR = _BV(TWINT) | _BV(TWEN); }

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#if USE_AVR_TWI_EEPROM

/**
 *      You need define TWI_SLA_ADDR_24CXX, it means E2PROM TWI address.
 *      Please see chip datasheet.
 */

/*
 * Note [3]
 * TWI address for 24Cxx EEPROM:
 *
 * 1 0 1 0 E2 E1 E0 R/~W        24C01/24C02
 * 1 0 1 0 E2 E1 A8 R/~W        24C04
 * 1 0 1 0 E2 A9 A8 R/~W        24C08
 * 1 0 1 0 A10 A9 A8 R/~W       24C16
 */
#define TWI_SLA_ADDR_24CXX   0xa0    /* E2 E1 E0 = 0 0 0 */

void avr_ee24xx_init(void)
{
	/* initialize TWI clock: 100 kHz clock, TWPS = 0 => prescaler = 1 */
#if defined(TWPS0)
	/* has prescaler (mega128 & newer) */
	TWSR = 0;
#endif

#if F_CPU < 3600000UL
	TWBR = 10; /* smallest TWBR value, see note [5] */
#else
	TWBR = (F_CPU / 100000UL - 16) / 2;
#endif
}
;

int avr_ee24xx_read_buffer(uint16_t eeaddr, int len, uint8_t *buf)
{
	TWI_BUS_START(); // start
	TWI_BUS_WAIT();

	TWI_BUS_WRITE_BYTE(TWI_SLA_ADDR_24CXX | TW_WRITE); // control byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MT_SLA_ACK)
		return -1; // ACK

	TWI_BUS_WRITE_BYTE(eeaddr >> 8); // address high byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MT_DATA_ACK)
		return -2; // ACK

	TWI_BUS_WRITE_BYTE(eeaddr & 0xFF); // address low byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MT_DATA_ACK)
		return -3; // ACK

	TWI_BUS_START(); // start
	TWI_BUS_WAIT();

	TWI_BUS_WRITE_BYTE(TWI_SLA_ADDR_24CXX | TW_READ); // control byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MR_SLA_ACK)
		return -4; // ACK

	TWI_BUS_START_READ();

	int i;
	for (i = 0; i < len; ++i)
	{
		if (i == len - 1)
			TWI_BUS_SET_NACK(); // send NACK
		else
			TWI_BUS_SET_ACK(); // send ACK

		TWI_BUS_WAIT();
		*buf = TWDR;
		//printf("%x",*buf);
		buf++;
	}

	TWI_BUS_STOP();

	return TRUE;
}

int avr_ee24xx_write_buffer(uint16_t eeaddr, uint8_t len, uint8_t* buf)
{
	TWI_BUS_START(); // start
	TWI_BUS_WAIT();

	TWI_BUS_WRITE_BYTE(TWI_SLA_ADDR_24CXX | TW_WRITE); // control byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MT_SLA_ACK)
		return -1; // ACK

	TWI_BUS_WRITE_BYTE(eeaddr >> 8); // address high byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MT_DATA_ACK)
		return -2; // ACK

	TWI_BUS_WRITE_BYTE(eeaddr & 0xFF); // address low byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MT_DATA_ACK)
		return -3; // ACK


	uint8_t i;
	for (i = 0; i < len; ++i)
	{
		//printf("%x",*buf);
		TWI_BUS_WRITE_BYTE(*buf);
		TWI_BUS_WAIT();
		if (TWI_BUS_STATUS != TW_MT_DATA_ACK)
			return -4; // ACK
		buf++;
	}

	TWI_BUS_STOP();

	return TRUE;
}

#endif // USE_AVR_TWI_EEPROM
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#if USE_AVR_TWI_CY2239X

/**
 *      You need define TWI_SLA_ADDR_CY2293X, it means CY2293X device TWI address.
 */

#define TWI_SLA_ADDR_CY2293X            (0x69 << 1) // default serial interface address
void avr_cy2239x_init(void)
{
	/* initialize TWI clock: 100 kHz clock, TWPS = 0 => prescaler = 1 */
#if defined(TWPS0)
	/* has prescaler (mega128 & newer) */
	TWSR = 0;
#endif

#if F_CPU < 3600000UL
	TWBR = 10; /* smallest TWBR value, see note [5] */
#else
	TWBR = (F_CPU / 100000UL - 16) / 2;
#endif
}
;

int avr_cy2239x_read_buffer(uint8_t addr, int len, uint8_t *buf)
{
	TWI_BUS_START(); // start
	TWI_BUS_WAIT();

	TWI_BUS_WRITE_BYTE(TWI_SLA_ADDR_CY2293X | TW_WRITE); // control byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MT_SLA_ACK)
		return -1; // ACK

	TWI_BUS_WRITE_BYTE(addr & 0xFF); // address byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MT_DATA_ACK)
		return -3; // ACK

	TWI_BUS_START(); // start
	TWI_BUS_WAIT();

	TWI_BUS_WRITE_BYTE(TWI_SLA_ADDR_CY2293X | TW_READ); // control byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MR_SLA_ACK)
		return -4; // ACK

	TWI_BUS_START_READ();

	int i;
	for (i = 0; i < len; ++i)
	{
		if (i == len - 1)
			TWI_BUS_SET_NACK(); // send NACK
		else
			TWI_BUS_SET_ACK(); // send ACK

		TWI_BUS_WAIT();
		*buf = TWDR;
		//printf("%x",*buf);
		buf++;
	}

	TWI_BUS_STOP();

	return TRUE;
}

int avr_cy2239x_write_buffer(uint8_t addr, uint8_t len, uint8_t* buf)
{
	TWI_BUS_START(); // start
	TWI_BUS_WAIT();

	TWI_BUS_WRITE_BYTE(TWI_SLA_ADDR_CY2293X | TW_WRITE); // control byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MT_SLA_ACK)
		return -1; // ACK

	TWI_BUS_WRITE_BYTE(addr & 0xFF); // address low byte
	TWI_BUS_WAIT();
	if (TWI_BUS_STATUS != TW_MT_DATA_ACK)
		return -3; // ACK

	uint8_t i;
	for (i = 0; i < len; ++i)
	{
		//printf("%x",*buf);
		TWI_BUS_WRITE_BYTE(*buf);
		TWI_BUS_WAIT();
		if (TWI_BUS_STATUS != TW_MT_DATA_ACK)
			return -4; // ACK
		buf++;
	}

	TWI_BUS_STOP();

	return TRUE;
}

#endif //USE_AVR_TWI_CY2239X
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

#if USE_AVR_TWI_BUS
/**
 * TWI Master Mode
 */

// TWI Bus start
uint8_t avr_twi_start(void)
{
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
	while ((TWCR & _BV(TWINT)) == 0);

	return TW_STATUS;
}

// TWI Bus stop
void avr_twi_stop(void)
{
	TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
}

/**
 * write byte to TWI bus
 * @param c write data
 * @return	TWI bus status.
 */
uint8_t avr_twi_writebyte(uint8_t c)
{
	TWDR = c;
	TWCR = _BV(TWINT) | _BV(TWEN);
	while ((TWCR & _BV(TWINT)) == 0);
	return TW_STATUS;
}

/**
 * read a byte data from TWI bus
 * @param c pointer to read data
 * @param ack if ack is TRUE, it will send ACK signal, otherwise will not.
 * @return TWI bus status.
 */
uint8_t avr_twi_readbyte(uint8_t *c, uint8_t ack)
{
	uint8_t tmp = _BV(TWINT)|_BV(TWEN);

	if(ack)
	tmp |= _BV(TWEA);
	TWCR = tmp;
	while ((TWCR & _BV(TWINT)) == 0);

	*c = TWDR;

	return TW_STATUS;
}

/**
 * Initialize TWI slave device
 * @param slave_device_addr slave device twi address
 */
avr_twi_slave_listen_callback_t g_twi_slave_callback = NULL;
void avr_twi_slave_init(uint8_t slave_device_addr, avr_twi_slave_listen_callback_t twi_slave_callback)
{
	// TWI 广播识别使能
	TWAR = slave_device_addr | _BV(TWGCE);

#if USE_AVR_TWI_INT
	TWCR = _BV(TWEA) | _BV(TWEN) | _BV(TWIE);
#else
	TWCR = _BV(TWEA) | _BV(TWEN);
#endif

	g_twi_slave_callback = twi_slave_callback;
}

#if USE_AVR_TWI_INT
ISR( TWI_vect )
{
	if( g_twi_slave_callback == NULL ) return;

	uint8_t twi_status = TW_STATUS;

	g_twi_slave_callback(twi_status, TWDR);

	TWCR &= ~_BV(TWINT); // clear flag
}

#else

void avr_twi_slave_loop( void )
{
	uint8_t twi_status;
	uint8_t j = 0;
	if( NULL == g_twi_slave_callback ) return;

	while(1)
	{
		while ((TWCR & _BV(TWINT)) == 0);

		twi_status = TW_STATUS;
		g_twi_slave_callback(twi_status, TWDR);

		switch(twi_status)
		{
			case TW_SR_SLA_ACK:
			printf("START\nSLA+W\n");
			break;
			case TW_SR_DATA_ACK:
			if(j==0)
			printf("收到:%d",TWDR);
			else
			printf(" %d",TWDR);
			j++;
			break;
			case TW_SR_STOP:
			printf(";\nSTOP\n\n");
			j=0;
			break;
			default:
			printf("error:%x",(int)twi_status);
			break;
		}

		// clear TWINT flag and continue receive data
		TWCR = _BV(TWEA) | _BV(TWEN) | _BV(TWINT);
	}
}
#endif

#if USE_AVR_TWI_MEM
/**
 * Write a block data to TWI device
 * @param mem_addr memory address
 * @param buf
 * @param len
 * @return
 */
uint8_t avr_twi_block_write(uint8_t mem_addr, uint8_t *buf, uint8_t len)
{
	uint8_t i;

	avr_twi_start();
	avr_twi_writebyte( AVR_TWI_BLOCK_MEM_DEVICE_ADDR | TW_WRITE );
	avr_twi_writebyte(mem_addr); //write address

	for(i=0; i<len; i++)
	{
		avr_twi_writebyte(buf[i]);
	}

	avr_twi_stop();
	return 0;
}

#define TW_ACK		1
#define TW_NACK 	0
/**
 * Read a block memory from TWI device
 * @param addr memory address
 * @param buf  pointer to read block buffer
 * @param len  read block buffer size
 * @return
 */
uint8_t avr_twi_block_read(uint8_t mem_addr, uint8_t *buf, uint8_t len)
{
	uint8_t i;

	//set read address
	avr_twi_block_write(mem_addr, 0, 0);
	_delay_ms(10);

	// start read data
	avr_twi_start();
	avr_twi_writebyte( AVR_TWI_BLOCK_MEM_DEVICE_ADDR | TW_READ );
	for(i=0;i<len-1;i++){
		avr_twi_readbyte(buf+i, TW_ACK);
	}
	avr_twi_readbyte(buf+i, TW_NACK);

	avr_twi_stop();

	return 0;
}
#endif //USE_AVR_TWI_MEM
#endif //USE_AVR_TWI_BUS
#endif /* USE_AVR_TWI_MODULE */
