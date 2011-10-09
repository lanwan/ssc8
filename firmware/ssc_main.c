/*
 *  ssc_main.c
 *
 *  Created on: 2009-6-10
 *
 *  Copyright (c) 2009 LDRobot.com
 *
 *  Author: Steven Wang
 *
 *	F(clk) = 8MHz
 *	TIMER0 256 分频, T=2.5ms
 *	TIMER1 8分频, Offset=-17us, hold 500us, you may set 500-17us
 *
 *	Support AT commands:
 *
 *	AT*SMV		AT*SMV=P0,1500,30,P1,1300,20
 *	AT*VER      tell version
 *	AT*RESET    reset
 *	AT*SBR      set baud rate
 *	AT*RADV     read ADC value
 *	AT*EER		read e2prom
 *	AT*EEW		write e2prom
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

# define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "avr_modules.h"

/**
 * Definition Board IO
 */

// control pins
#define SVR1    PC0
#define SVR2    PC1
#define SVR3    PC2
#define SVR4    PC3
#define SVR5    PD4
#define SVR6    PD5
#define SVR7    PD6
#define SVR8    PD7

/*
 * global definition
 */

#define DEBUG	1
#define TRUE    1
#define FALSE   0

/*
 * AT MODULE definition
 */

// AT module ID&P&V
static const char RTL_M_ID[] PROGMEM = "\r\n*VER:SSC8.V2\r\n";
static const char RTL_M_CMDREADY[] PROGMEM = "\r\n*CSTU\r\n";

// Adjust code time
#define ADJUST_CODE_TIME_US 	17
#define MIN_POSITION        	450
#define MAX_POSITION        	2400
#define UNUSED					0

static const uint8_t g_svr_gpios[8] =
{ SVR1, SVR2, SVR3, SVR4, SVR5, SVR6, SVR7, SVR8 };

struct SVR_PRO
{
	int16_t position_us; // position us
	int16_t hold_time_count; // hold time, -1 means hold all time, it's n*2.5ms
	int16_t save_position_us; // save position
	int16_t speed; // speed per 2.5ms
};

static /*volatile*/int g_active_svr_idx = 0;
static struct SVR_PRO g_runtime_svrs[8] =
{
{ 1500, -1, 1500, 0 },
{ 1500, -1, 1500, 0 },
{ 1500, -1, 1500, 0 },
{ 1500, -1, 1500, 0 },
{ 1500, -1, 1500, 0 },
{ 1500, -1, 1500, 0 },
{ 1500, -1, 1500, 0 },
{ 1500, -1, 1500, 0 } };

/**
 * E2PROM variants
 */

unsigned char g_eprom_baud_ubrr __attribute__((section(".eeprom")));
unsigned char g_eprom_auto_run __attribute__((section(".eeprom")));

static int8_t g_startup_run_pg = FALSE;
static uint16_t g_eprom_address = 0;
enum LD_BASIC_CMMD_TYPE
{
	LDB_CMMD_MOOV = 0,
	LDB_CMMD_SLEEP,
	LDB_CMMD_JUMP,
	LDB_CMMD_IF_JUMP,
	LDB_CMMD_END,
};

struct LD_BASIC_CMMD_MOOV
{
	uint8_t pin;
	int16_t postion;
	int16_t speed;
};

struct LD_BASIC_CMMD_SLEEP
{
	uint16_t sleep_ms;
};

struct LD_BASIC_CMMD_JUMP
{
	uint16_t address;
};

struct LD_BASIC_CMMD_IF_JUMP
{
	void* void_variant;
	uint16_t true_address;
	uint16_t false_address;
};

/*static uint8_t
		LD_BAISIC_CMMD_LENS[] =
		{ sizeof(struct LD_BASIC_CMMD_MOOV),
				sizeof(struct LD_BASIC_CMMD_SLEEP),
				sizeof(struct LD_BASIC_CMMD_JUMP),
				sizeof(struct LD_BASIC_CMMD_IF_JUMP) };
*/
union LD_BASIC_CMMD
{
	struct LD_BASIC_CMMD_MOOV cmmd_moov;
	struct LD_BASIC_CMMD_SLEEP cmmd_sleep;
	struct LD_BASIC_CMMD_JUMP cmmd_jump;
	struct LD_BASIC_CMMD_IF_JUMP cmmd_if_jump;
};

/**
 * AT-COMMAND support functions and definition
 */

#define MAX_COMMAND_PARAMS_COUNT 	20
struct RTCMMD_HANDLES
{
	const char* cmmd;
	int8_t
	(*cmmd_handle)(void);
};

struct RTCMMD
{
	char* cmmd;
	int8_t param_count;
	char* params[MAX_COMMAND_PARAMS_COUNT];
};

static struct RTCMMD g_user_rtcmmd;

static int8_t
rt_ver(void); // show version
static int8_t
rt_reset(void); // reset
static int8_t
rt_sbr(void); // set UART baud rate

static int8_t
rt_smvr(void); // read move arm postion
static int8_t
rt_smv(void); // move arm
static int8_t
rt_radv(void); // Read AD value
/*
int8_t
rt_eew(void); // write command to eeprom
int8_t
rt_eer(void); // clear eeprom
int8_t
rt_run(void); // run eeprom
*/
static int8_t
rt_cywt(void); // write byte to cy22393
static int8_t
rt_cyrd(void); // read byte from cy22393

//static int8_t
//run_user_programm(void);

static struct RTCMMD_HANDLES g_cmmd_handles[] =
{
{ "AT*SMVR", &rt_smvr },
{ "AT*SMV", &rt_smv },
{ "AT*VER", &rt_ver },
{ "AT*RESET", &rt_reset },
{ "AT*SBR", &rt_sbr },
{ "AT*RADV", &rt_radv },
//{ "AT*EEW", &rt_eew },H:\i-Robot\projects\com.ssc\src\default
//{ "AT*EER", &rt_eer },
//{ "AT*RUN", &rt_run },
{ "W", &rt_cywt },
{ "R", &rt_cyrd }, };

int8_t rt_smvr(void)
{
	// check parameters
	int i;
	for (i = 0; i < 8; ++i)
	{
		printf("\r\nSMVR:%d,%d,%d,%d,%d\r\n", i,
				g_runtime_svrs[i].save_position_us,
				g_runtime_svrs[i].position_us,
				g_runtime_svrs[i].hold_time_count, g_runtime_svrs[i].speed);
	}

	return TRUE;
}

int8_t rt_smv(void)
{

	// check parameters
	if (g_user_rtcmmd.param_count <= 0)
		return FALSE;
	if (g_user_rtcmmd.param_count % 3 != 0)
		return FALSE;

	struct SVR_PRO temp_action[8];
	memcpy(&temp_action, &g_runtime_svrs, sizeof(g_runtime_svrs));

	int i;
	int j = 0;

	for (i = 0; i < g_user_rtcmmd.param_count; i += 3)
	{

		// read pin number
		sscanf(g_user_rtcmmd.params[i], "P%d", &j);
		// check pin number
		if (j < 0 || j > 7)
			return FALSE;

		// read position & hold time
		temp_action[j].save_position_us = atol(g_user_rtcmmd.params[i + 1]);
		temp_action[j].speed = atol(g_user_rtcmmd.params[i + 2]);

		// check data
		if (temp_action[j].save_position_us < MIN_POSITION
				|| temp_action[j].save_position_us > MAX_POSITION)
			return FALSE;

		// calculate speed
		if (temp_action[j].position_us >= MIN_POSITION)
		{
			if (temp_action[j].speed == 0)
			{
				temp_action[j].speed = (temp_action[j].save_position_us
						- temp_action[j].position_us);
			}
			else
			{
				if (temp_action[j].save_position_us
						< temp_action[j].position_us)
				{
					temp_action[j].speed = -temp_action[j].speed;
				}
			}
		}
		else
		{
			temp_action[j].position_us = temp_action[j].save_position_us;
		}

	}

	memcpy(&g_runtime_svrs, &temp_action, sizeof(temp_action));

	return TRUE;
}

int8_t rt_ver(void)
{
	printf_P(RTL_M_ID);
	return TRUE;
}

int8_t rt_radv(void)
{

	// check parameters
	if (g_user_rtcmmd.param_count < 1)
	{
		return FALSE;
	}

	int8_t channel = atoi(g_user_rtcmmd.params[0]);
	int16_t adc_value = 0;

	if (channel >= 0 && channel <= 7)
	{

		if (avr_adc_open(AVR_ADC_AREF_SRC_INNER2_5V, channel))
		{

			adc_value = avr_adc_read(channel);
			avr_adc_close();

			printf("\r\n*RADV:%d,%d\r\n", channel, adc_value);
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * AT*SBR=<baud rate>
 * <baud rate> accepted values 921600 - 460800 – 230400 – 115200 – 57600 – 9600
 * Default baud rate is 115200 bauds
 */
int8_t rt_sbr(void)
{
	// check parameters
	if (g_user_rtcmmd.param_count < 1)
	{
		return FALSE;
	}

	uint16_t baud_rate = atol(g_user_rtcmmd.params[0]);
	uint8_t baud_ubrr = 0;
	switch (baud_rate)
	{
	case 2400:
		baud_ubrr = 207;
		break;
	case 4800:
		baud_ubrr = 103;
		break;
	case 9600:
		baud_ubrr = 51;
		break;
	case 19200: // default band rate
		baud_ubrr = 25;
		break;
	case 28800:
		baud_ubrr = 16;
		break;
	case 31250:
		baud_ubrr = 15;
		break;
	case 38400:
		baud_ubrr = 12;
		break;
	}

	if (baud_ubrr)
	{
		uint8_t baud_ubrr_temp = 0;
		eeprom_busy_wait();
		eeprom_write_byte(&g_eprom_baud_ubrr, baud_ubrr);
		eeprom_busy_wait();
		baud_ubrr_temp = eeprom_read_byte(&g_eprom_baud_ubrr);

		// double verify save value
		if (baud_ubrr_temp == baud_ubrr)
		{
			printf("\r\n*SBR:%d\r\n", baud_ubrr);
		}

		return TRUE;
	}

	return FALSE;
}

void
(*reset)(void) = 0x0000;

int8_t rt_reset(void)
{
	g_eprom_address = 0;
	reset();
	return TRUE;
}

/*
int8_t rt_eew(void)
{
	// check parameters
	if (g_user_rtcmmd.param_count > 9 || g_user_rtcmmd.param_count < 2)
	{
		return FALSE;
	}

	TWBR = 8;

	int8_t i;
	uint8_t temp[8];
	memset(temp, 0, sizeof(temp));
	uint16_t dev_addr = atol(g_user_rtcmmd.params[0]);
	for (i = 0; i < g_user_rtcmmd.param_count - 1; ++i)
	{
		temp[i] = atoi(g_user_rtcmmd.params[i + 1]);
	}

	int ret = avr_ee24xx_write_buffer(dev_addr, i, (uint8_t*) &temp[0]);
	//printf("ret=%d\r", ret);

	return ret;
}


int8_t rt_eer(void)
{
	// check parameters
	if (g_user_rtcmmd.param_count < 2)
	{
		return FALSE;
	}

	int8_t i = 0;
	uint8_t temp[8];
	memset(temp, 0, sizeof(temp));
	uint16_t dev_addr = atol(g_user_rtcmmd.params[0]);
	int8_t len = atoi(g_user_rtcmmd.params[1]);
	if (len < 0 || len > 8)
		return FALSE;
	//printf("len=%d\r", len);

	TWBR = 8;

	int8_t ret = avr_ee24xx_read_buffer(dev_addr, len, (uint8_t*) &temp[0]);
	//printf("ret=%d\r", ret);
	if (ret == TRUE)
	{
		printf("\r\n*EER:");
		for (i = 0; i < len; ++i)
		{
			if (i != len - 1)
				printf("%d,", temp[i]);
			else
				printf("%d", temp[i]);
		}
		printf("\r\n");
		return TRUE;
	}

	return FALSE;
}

*/

#define S0			PC4
#define S1 		PC5
#define S2 		PC3
int8_t rt_cywt(void)
{
//	// check parameters
//	if (g_user_rtcmmd.param_count > 9 || g_user_rtcmmd.param_count < 2)
//	{
//		return FALSE;
//	}
//
//	TWBR = 8;
//
//	int8_t i;
//	uint8_t temp[8] = {0};
//	memset(temp, 0, sizeof(temp));
//	uint16_t dev_addr = atol(g_user_rtcmmd.params[0]);
//	for (i = 0; i < g_user_rtcmmd.param_count - 1; ++i)
//	{
//		temp[i] = atoi(g_user_rtcmmd.params[i + 1]);
//	}

//		PORTC

        int ret = FALSE;
        {
            uint8_t temp[] = {0x00,0x00,0x05,0x05,0x01,
							  0x00,0x04,0x50,0x55,0x00,
							  0x00,0x00,0x00,0x00,0x00,
							  0xAA,0x00,0x00,0xE9,0x08};

            ret = avr_cy2239x_write_buffer(0x08, SIZEOF(temp), (uint8_t*) &temp[0]);
            printf("ret=%d\r", ret);
        }

        {
            uint8_t temp[] = {0x35,0x92,0xE5,0x35,0x92,0xE5,0x35,0x92,0xE5,0x35,0x92,0xE5,0x35,0x92,0xE5,0x35,0x92,0xE5,0x35,0x92,0xE5,0x35,0x92,0xE5};
            ret = avr_cy2239x_write_buffer(0x40, SIZEOF(temp), (uint8_t*) &temp[0]);
            printf("ret=%d\r", ret);
        }

	return ret;
}

int8_t rt_cyrd(void)
{
	// check parameters
//	if (g_user_rtcmmd.param_count < 2)
//	{
//		return FALSE;
//	}
//
//	int8_t i = 0;
//	uint8_t temp[8];
//	memset(temp, 0, sizeof(temp));
//	uint8_t dev_addr = atol(g_user_rtcmmd.params[0]);
//	int8_t len = atoi(g_user_rtcmmd.params[1]);
//	if (len < 0 || len > 8)
//		return FALSE;
//	printf("len=%d\r", len);

	TWBR = 8;

	int8_t i = 0;
	int8_t len = 24;
	uint8_t temp[24] = {0};
//	memset(temp, 0, sizeof(temp));
//	int8_t ret = avr_cy2239x_read_buffer(0x08, len, (uint8_t*) &temp[0]);
//	printf("s1,ret=%d\r", ret);
//	if (ret == TRUE)
//	{
//		printf("\r\n*EER:");
//		for (i = 0; i < len; ++i)
//		{
//			if (i != len - 1)
//				printf("%d,", temp[i]);
//			else
//				printf("%d", temp[i]);
//		}
//		printf("\r\n");
//		return TRUE;
//	}

	TWBR = 8;
	memset(temp, 0, sizeof(temp));
	int8_t ret = avr_cy2239x_read_buffer(0x40, len, (uint8_t*) &temp[0]);
        printf("S2,ret=%d\r", ret);
        if (ret == TRUE)
        {
                printf("\r\n*EER:");
                for (i = 0; i < len; ++i)
                {
                        if (i != len - 1)
                                printf("%d,", temp[i]);
                        else
                                printf("%d", temp[i]);
                }
                printf("\r\n");
                return TRUE;
        }

	return FALSE;
}

/*
int8_t rt_run(void)
{
	// check parameters
	if (g_user_rtcmmd.param_count < 1)
	{
		return FALSE;
	}

	g_eprom_address = 0;

	g_startup_run_pg = atoi(g_user_rtcmmd.params[0]);

	eeprom_busy_wait();
	eeprom_write_byte(&g_eprom_auto_run, g_startup_run_pg);

	if (g_startup_run_pg)
	{
		return run_user_programm();
	}

	return TRUE;
}
;


static int8_t run_user_programm(void)
{
	uint8_t cmmd_type = 0;
	uint8_t cmmd_len = 0;
	union LD_BASIC_CMMD basic_cmmd;
	while (TRUE)
	{

		// read command
		if (TRUE != avr_ee24xx_read_buffer(g_eprom_address, 1, &cmmd_type))
		{
			return FALSE;
		}
		//printf("t%d\r", cmmd_type);

		g_eprom_address += sizeof(cmmd_type);

		// command with data
		if (cmmd_type < LDB_CMMD_END)
		{
			cmmd_len = LD_BAISIC_CMMD_LENS[cmmd_type];
			//printf("l%d\r", cmmd_len);

			if (TRUE != avr_ee24xx_read_buffer(g_eprom_address, cmmd_len,
					(uint8_t*) &basic_cmmd))
			{
				return FALSE;
			}

			g_eprom_address += cmmd_len;

			switch (cmmd_type)
			{
			case LDB_CMMD_MOOV:
			{
				//printf("p%d,p%d,s%d\r", basic_cmmd.cmmd_moov.pin, basic_cmmd.cmmd_moov.postion, basic_cmmd.cmmd_moov.speed);
				if (basic_cmmd.cmmd_moov.postion >= MIN_POSITION
						&& basic_cmmd.cmmd_moov.postion <= MAX_POSITION)
				{

					int16_t
							pos_space =
									basic_cmmd.cmmd_moov.postion
											- g_runtime_svrs[basic_cmmd.cmmd_moov.pin].save_position_us;
					g_runtime_svrs[basic_cmmd.cmmd_moov.pin].save_position_us
							= basic_cmmd.cmmd_moov.postion;

					g_runtime_svrs[basic_cmmd.cmmd_moov.pin].speed = pos_space
							> 0 ? abs(basic_cmmd.cmmd_moov.speed)
							: -abs(basic_cmmd.cmmd_moov.speed);
					if (g_runtime_svrs[basic_cmmd.cmmd_moov.pin].speed == 0)
					{
						g_runtime_svrs[basic_cmmd.cmmd_moov.pin].speed
								= pos_space;
					}
				}
			}
				break;
			case LDB_CMMD_SLEEP:
			{
				//printf("sl%d\r", basic_cmmd.cmmd_sleep.sleep_ms);
				while (basic_cmmd.cmmd_sleep.sleep_ms--)
				{
					_delay_loop_2(4000);
					_delay_loop_2(4000);
				}
			}
				break;
			case LDB_CMMD_JUMP:
			{
				//printf("jump %d\r", basic_cmmd.cmmd_jump.address);
				g_eprom_address = basic_cmmd.cmmd_jump.address;
				return TRUE;
			}
				break;
			}
		}
		// command without data
		else
		{
			switch (cmmd_type)
			{
			case LDB_CMMD_END:
				return TRUE;
			}

			return FALSE;
		}

	}
}
*/

/**
 * AT-Command main handle
 */
void rtcmd_main_handle(void)
{
	const char* ERROR_STR = "\r\nERROR\r\n";
	const char* OK_STR = "\r\nOK\r\n";
	int i;
	for (i = 0; i < SIZEOF(g_cmmd_handles); ++i)
	{
		if (0 == strcmp(g_cmmd_handles[i].cmmd, g_user_rtcmmd.cmmd))
		{
			if (g_cmmd_handles[i].cmmd_handle)
			{
				if (g_cmmd_handles[i].cmmd_handle())
					printf(OK_STR);
				else
					printf(ERROR_STR);
			};
			return;
		}
	}

	if (i >= SIZEOF(g_cmmd_handles))
	{
		printf(ERROR_STR);
	}
}

/**
 * Initialize Timer for generating PWM signal
 */
static void InitTimer(void)
{

	// TIMER0 预分频 ck/256
	TCCR0 = _BV(CS02);
	TIMSK |= _BV(TOIE0);
	TCNT0 = 0xB2;

	// TIMER1, clkI/O/8
	TCCR1B |= _BV(CS11);
}

/**
 * Initialize SVR Motor PWM IO Pins
 */
static void InitSVRGPIO(void)
{
	// OUTPUT
	DDRC |= _BV(SVR1) | _BV(SVR2) | _BV(SVR3) | _BV(SVR4);
	DDRD |= _BV(SVR5) | _BV(SVR6) | _BV(SVR7) | _BV(SVR8);

	// enable upload R
	PORTC = _BV(PC4); // i2c SCL
	PORTC = _BV(PC5); // i2c SDA

	//avr_ee24xx_init();
	TWBR = 8;
	TWCR|=1<<TWEN;
}

/**
 * TIMER1 to hold time for SVR PWM
 *
 * T = delay time 500us ~ 2400us
 */
ISR( TIMER1_OVF_vect )
{
	if (g_active_svr_idx >= 0 && g_active_svr_idx < 4)
	{
		PORTC &= ~_BV(g_svr_gpios[g_active_svr_idx]);
	}

	else
	{
		PORTD &= ~_BV(g_svr_gpios[g_active_svr_idx]);
	}

	TIMSK &= ~_BV(TOIE1);
	TIFR &= ~_BV(TOV1);
}

/**
 * TIMER0 to hold 2.5ms for 8 channels
 */
ISR( TIMER0_OVF_vect )
{
	// reload counter
	TCNT0 = 0xB2;

	g_active_svr_idx++;
	g_active_svr_idx = g_active_svr_idx > 7 ? 0 : g_active_svr_idx;

	// set timer1 to set svr PWM
	if (g_runtime_svrs[g_active_svr_idx].save_position_us >= MIN_POSITION
			&& g_runtime_svrs[g_active_svr_idx].save_position_us
					>= MIN_POSITION)
	{

		// set high
		if (g_active_svr_idx >= 0 && g_active_svr_idx < 4)
		{
			PORTC |= _BV(g_svr_gpios[g_active_svr_idx]);
		}

		else
		{
			PORTD |= _BV(g_svr_gpios[g_active_svr_idx]);
		}

		// 中断标志清零
		TIFR &= ~_BV(TOV1);

		// 计算计数值
		TCNT1H = 0xFF - (g_runtime_svrs[g_active_svr_idx].position_us >> 8); // must be first
		TCNT1L = 0xFF - (g_runtime_svrs[g_active_svr_idx].position_us & 0xFF);

		// 中断允许
		TIMSK |= _BV(TOIE1);
		sei();

		// assign new position_us
		if (abs(g_runtime_svrs[g_active_svr_idx].save_position_us - g_runtime_svrs[g_active_svr_idx].position_us)
				<= abs(g_runtime_svrs[g_active_svr_idx].speed))
		{
			g_runtime_svrs[g_active_svr_idx].position_us
					= g_runtime_svrs[g_active_svr_idx].save_position_us;
		}
		else
		{
			g_runtime_svrs[g_active_svr_idx].position_us
					+= g_runtime_svrs[g_active_svr_idx].speed;
		}
	}

}

/**
 * Initialize SSC component board devices
 */
static void init_board(void)
{

	uint8_t baud_ubrr_temp = 0;
	/*
	 *	Initialize AVR MCU device
	 */

	// Disable interrupt
	cli();

	// Initialize SVR Motor PWM IO Pins
	InitSVRGPIO();

	// Initialize Timer for generating PWM signal
	InitTimer();

	// Initialize UART register and set band rate for online debugging
	eeprom_busy_wait();
	baud_ubrr_temp = eeprom_read_byte(&g_eprom_baud_ubrr);

	if (baud_ubrr_temp == 255 || baud_ubrr_temp == 0)
	{
		avr_uart_init(25);
	}
	else
	{
		avr_uart_init(baud_ubrr_temp);
	}

	eeprom_busy_wait();
	g_startup_run_pg = eeprom_read_byte(&g_eprom_auto_run);
	// enable watch dog, 128K CLK
	//wdt_enable(3);

	// Enable interrupt
	sei();
}
/**
 * Initialize C language stdio and redirect to UART port
 */
FILE mystdout =
FDEV_SETUP_STREAM(avr_uart_putchar, avr_uart_getchar, _FDEV_SETUP_WRITE);
static void init_c_language(void)
{
	stdout = &mystdout;
}

int main(void)
{

	char rcv_buf[100] = { 0 };
	int rcv_char = 0;
	int16_t rcv_buf_end = 0;
	int8_t cmd_is_busy = 0;
	g_active_svr_idx = -1;

	init_c_language();

	init_board();

	printf_P(RTL_M_CMDREADY);

	for (;;)
	{
		while (1)
		{

			// feed watch dog
			//wdt_reset();

			// receive a char data
			rcv_char = avr_uart_getc();
			//avr_uart_putc(rcv_char);
			//rcv_char = getchar();
			if (EOF == rcv_char || cmd_is_busy)
			{
				continue;
			}
			putchar(rcv_char);
			if (rcv_char != '\r')
			{
				if (rcv_buf_end == 100)
					rcv_buf_end = 0;
				rcv_buf[rcv_buf_end++] = rcv_char;

			}
			else
			{
				cmd_is_busy = TRUE;

				rcv_buf[rcv_buf_end] = '\0';
				rcv_buf_end = 0;
				printf("%s",rcv_buf);

				char* p = rcv_buf;
				char* buf_ptr = rcv_buf;

				while (TRUE)
				{
					if (*p == '=')
					{
						*p++ = '\0';
						g_user_rtcmmd.cmmd = buf_ptr;
						buf_ptr = p;
					}
					else if (*p == ',')
					{
						*p++ = '\0';
						g_user_rtcmmd.params[g_user_rtcmmd.param_count++]
								= buf_ptr;
						buf_ptr = p;
					}
					else if (*p == '\0')
					{
						if (buf_ptr != p)
						{
							if (NULL == g_user_rtcmmd.cmmd)
							{
								g_user_rtcmmd.cmmd = buf_ptr;
							}
							else
							{
								g_user_rtcmmd.params[g_user_rtcmmd.param_count++]
										= buf_ptr;
							}
						}

						break;
					}
					else
					{
						p++;
					}
				}

				// execute user command
				rtcmd_main_handle();

				// clear user at command buf.
				memset(&g_user_rtcmmd, 0, sizeof(g_user_rtcmmd));

				cmd_is_busy = FALSE;
			}
		}
	}
	return 0;
}

