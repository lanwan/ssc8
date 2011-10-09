/*
 * avr_adc.c
 *
 *  Created on: 2009-8-19
 *      Author: Steven
 */

#include "avr_modules.h"

#if USE_AVR_ADC_MODULE

// 右对齐方式读取
int8_t avr_adc_open(int8_t aref, int8_t channel){

	uint8_t admux = 0;

	switch(aref){
	case AVR_ADC_AREF_SRC_EXTERNAL:
		admux = 0x00;
		break;
	case AVR_ADC_AREF_SRC_VCC:
		admux |= _BV(REFS0);
		break;
	case AVR_ADC_AREF_SRC_INNER2_5V:
		admux |= _BV(REFS0) | _BV(REFS1);
		break;
	default:
		return FALSE;
	}

	admux |= channel;

	ADMUX = admux;

	ADCSRA = _BV(ADEN);	//使能 ADC，单次转换模式

	return TRUE;
}

void avr_adc_close(void){
	ADCSRA = 0;	//关闭 ADC
}

uint16_t avr_adc_read(uint8_t channel){
	uint16_t ret = 0;
	int i = 0;

	uint16_t vals[AVR_ADC_AREF_SRC_COVERT_COUNT] = {0};

	for(i=0; i<AVR_ADC_AREF_SRC_COVERT_COUNT; ++i){
		// start first convert
		ADCSRA |= _BV(ADSC);
		_delay_loop_1(60);

		while( ADCSRA & _BV(ADSC) ) _delay_loop_1(60);
		vals[i] = ADCL;
		vals[i] |= (ADCH<<8);
	}

	ret = avr_math_maverage((uint16_t*)&vals[0], (uint16_t*)&vals[AVR_ADC_AREF_SRC_COVERT_COUNT - 1]);

	return ret;
}

#endif /* USE_AVR_ADC_MODULE */
