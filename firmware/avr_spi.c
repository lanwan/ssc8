#include <avr/io.h>
#include <avr/interrupt.h>

#include "avr_modules.h"

#if USE_AVR_SPI_MODULE

// Define the SPI_USEINT key if you want SPI bus operation to be
// interrupt-driven.  The primary reason for not using SPI in
// interrupt-driven mode is if the SPI send/transfer commands
// will be used from within some other interrupt service routine
// or if interrupts might be globally turned off due to of other
// aspects of your program
//
// Comment-out or uncomment this line as necessary

#define SPI_USEINT  0L


#ifndef SPI_DDR
# warning "SPI_DDR not defined!!!!"
#endif

#ifndef SPI_PIN_MOSI
# warning "SPI_PIN_MOSI not defined!!!!"
#endif

#ifndef SPI_PIN_MISO
# warning "SPI_PIN_MISO not defined!!!!"
#endif

#ifndef SPI_PIN_SCK
# warning "SPI_PIN_SCK not defined!!!!"
#endif

#ifndef SPI_PIN_SS
# warning "SPI_PIN_SS not defined!!!!"
#endif

#define SPI_USEINT  0L

#if(SPI_USEINT)
static volatile uint8_t s_spi_transfer_complete;

SIGNAL(SIG_SPI) {
    s_spi_transfer_complete = TRUE;
}
#endif


// Init SPI
void avr_spi_master_init(uint8_t val) {
    // 设置 MOSI和 SCK为输出，其他为输入
    SPI_DDR |= (1<<SPI_PIN_MOSI)|(1<<SPI_PIN_SCK)|(1<<SPI_PIN_SS);

    // 允许SPI, 使能SPI主机模式，设置时钟速率为fck/32
    switch (val) {

    case 2:
        SPCR |= (1<<SPE)|(1<<MSTR); // fck/2
        SPSR |= (1<<SPI2X);
        break;

    case 4:
        SPCR |= (1<<SPE)|(1<<MSTR); // fck/4
        break;

    case 8:
        SPCR |= (1<<SPE)|(1<<MSTR)|(1<<SPR0);   // fck/8
        SPSR |= (1<<SPI2X);
        break;

    case 16:
        SPCR |= (1<<SPE)|(1<<MSTR)|(1<<SPR0); // fck/16
        break;

    case 32:
        SPCR |= (1<<SPE)|(1<<MSTR)|(1<<SPI2X)|(1<<SPR1);  // fck/32
        break;

    };

#if(SPI_USEINT)
    s_spi_transfer_complete = TRUE;
    SPCR |= _BV(SPIE);
#endif

}

// 启动数据传输
uint8_t avr_spi_master_transmit_byte(uint8_t val) {
#if (SPI_USEINT)
    // set a flag and send a byte data
    s_spi_transfer_complete = FALSE;
    SPDR = val;
    // wait flag
    while (!s_spi_transfer_complete);
#else
    // begin send a byte data
    SPDR = val;
    // wait register flag
    while (!(SPSR & (1<<SPIF)));
#endif
    // return data
    return (SPDR);
}

uint16_t avr_spi_master_transmit_word(uint16_t val) {
    uint16_t result = 0;

    // send MS byte of given data
    result = (avr_spi_master_transmit_byte((val>>8) & 0x00FF))<<8;

    // send LS byte of given data
    result |= (avr_spi_master_transmit_byte(val & 0x00FF));

    return result;
}

void avr_spi_slave_init(void) {
    // 设置MISO为输出，其他为输入
    SPI_DDR = (1<<SPI_PIN_MISO);

    // 使能 SPI
    SPCR = (1<<SPE);
}

uint8_t avr_spi_slave_receive(void) {
    // 等待接收结束
    while (!(SPSR & (1<<SPIF)));

    // 返回数据
    return SPDR;
}

#endif /* USE_AVR_SPI_MODULE */
