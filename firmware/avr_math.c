
#include "avr_modules.h"

#if USE_AVR_MATH_MODULE

#if USE_AVR_MATH_SELECT_SORT
void avr_math_select_sort(uint16_t* start, uint16_t* stop) {
    if ( start == 0 || stop == 0 ) return;

    uint16_t temp;
    uint16_t* pcur = start;
    uint16_t* pmov = pcur + 1;

    do {
        if ( *pcur > *pmov ) {
            temp = *pcur;
            *pcur = *pmov;
            *pmov = temp;
        }

        if ( pmov == stop ) {
            pcur++;
            pmov = pcur;
        } else
            pmov++;

    } while ( pcur < stop );
}

#endif // USE_AVR_MATH_SELECT_SORT

#if USE_AVR_MATH_EACH_ADD
void avr_math_each_add(uint16_t* start, uint16_t* stop, uint16_t data) {
    if ( start == 0 || stop == 0 ) return;

    do {
        *start++ += data;
    } while ( start != stop );

}

#endif // USE_AVR_MATH_EACH_ADD

#if USE_AVR_MATH_MAVERAGE
/**
 * moving average, it will remove one max and one min value and compute the avrage value
 * @param vals	data vector
 * @param len	data vector length
 * @return	avrage value
 */
uint16_t avr_math_maverage(uint16_t* start, uint16_t* stop){
    if ( start == 0 || stop == 0 ) return 0;

	uint16_t sum = 0;
	uint16_t max_value, min_value;
	uint16_t* p = start;
	uint16_t len = 0;

	max_value = min_value = *p;
	while(p != stop){
		if( *p > max_value ){
			max_value = *p;
		}
		if( *p < min_value){
			min_value = *p;
		}
		p++;
	}

	p = start;
	while( p != stop ){
		if(*p == max_value){
			max_value = 0;
		}else{
			sum += *p;
			len++;
		}

		if(*p == min_value){
			min_value = 0;
		}else{
			sum += *p;
			len++;
		}

		p++;
	}

	return (sum / len);
}

#endif //USE_AVR_MATH_MAVERAGE

#endif /* USE_AVR_MATH_MODULE */
