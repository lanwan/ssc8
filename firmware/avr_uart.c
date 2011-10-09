#include "avr_modules.h"

#if USE_AVR_UART_MODULE

#include <avr/io.h>
#include <stdint.h>

//void avr_uart_init(void) {
//#if F_CPU < 2000000UL && defined(U2X)
//	UCSRA = _BV(U2X); /* improve baud rate error by using 2x clk */
//	UBRRL = (F_CPU / (8UL * UART_BAUD)) - 1;
//#else
//	UBRRL = (F_CPU / (16UL * UART_BAUD)) - 1;
//#endif
//	UCSRB = _BV(TXEN) | _BV(RXEN); /* tx/rx enable */
//}

#if USE_AVR_UART_C_IO
int avr_uart_putchar(char c, FILE *unused)
{
//	if( c == '\n' )
//		avr_uart_putchar('\r', unused);

	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = c;
	return 0;
}

int avr_uart_getchar(FILE *stream)
{
	loop_until_bit_is_set(UCSRA, RXC);
	if (UCSRA & _BV(FE))
	  return _FDEV_EOF;
	if (UCSRA & _BV(DOR))
	  return _FDEV_ERR;

	return UDR;
}

#endif // USE_AVR_UART_C_IO

/*
*********************************************************************************************************
*                                          INIT UART BAUD RATE
*
* Description : Initialize uart port baud rate
*
* Arguments   : ubrr_val  UBRRH value
*
* Returns     : None
*********************************************************************************************************
*/
void avr_uart_init(int16_t ubrr_val) {
    /* ���ò����� */
    UBRRH = (unsigned char)(ubrr_val>>8);
    UBRRL = (unsigned char)ubrr_val;

    /* �������뷢����ʹ�� */
    UCSRB = (1<<RXEN)|(1<<TXEN);

    /* ����֡��ʽ: 8������λ, 1��ֹͣλ*/
    UCSRC = (1<<URSEL)|(3<<UCSZ0);
}

#if USE_AVR_UART_IO
/*
*********************************************************************************************************
*                                          PUT A BYTE VALUE TO UART PORT
*
* Description : Put a byte data value to uart port
*
* Arguments   : val data value
*
* Returns     : None
*********************************************************************************************************
*/
void avr_uart_putc(uint8_t val) {
    /* �ȴ����ͻ�����Ϊ�� */
    while ( !( UCSRA & (1<<UDRE)) );

    /* �����ݷ��뻺�������������� */
    UDR = val;
}

/*
*********************************************************************************************************
*                                          GET A BYTE VALUE FROM UART PORT
*
* Description : Get a byte data value from uart port
*
* Arguments   : None
*
* Returns     : Read data
*********************************************************************************************************
*/
uint8_t avr_uart_getc(void) {
    /*�ȴ���������*/
    while ( !(UCSRA & (1<<RXC)) );

    /* �ӻ������л�ȡ����������*/
    return UDR;
}

#endif // USE_AVR_UART_IO

#if USE_AVR_UART_RW_BUFFER

/*
*********************************************************************************************************
*                                          WRITE A STREAM DATA TO UART PORT
*
* Description : Write a stream data to uart
*
* Arguments   : buf Point to stream data
*
*               len The stream data length
*
* Returns     : Read data
*********************************************************************************************************
*/
void avr_uart_write(const char* buf, uint16_t len) {
    while (len--) {
        avr_uart_putc(*buf++);
    }
}

#endif // USE_AVR_UART_RW_BUFFER

#endif /* USE_AVR_UART_MODULE */
