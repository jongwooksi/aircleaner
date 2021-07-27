//
//	Device  	: MC96FR332A(R)
//
#include <intrins.h>
#include <stdio.h>
#include "MC96FR332A.h"		  
#include "uart.h"


unsigned int _uart_tx_count, _uart_rx_count;
unsigned int _uart_tx_len,   _uart_rx_len;

bit _uart_tx_complete = 0;
bit _uart_rx_complete = 0;

unsigned char _uart_tx_err = 0;
unsigned char _uart_rx_err = 0;
unsigned char UART_BUFFER_SIZE = 8;
unsigned char _uart_rx_buf[8] = {0};
unsigned char _uart_tx_buf[8] = {0};

unsigned int _IdleCounter = 0;



void UART1_Clear()
{
	int i;

	_uart_tx_count = 0;
	_uart_rx_count = 0;

	for(i=0; i < UART_BUFFER_SIZE; i++) {
		_uart_rx_buf[i] = 0;
		_uart_tx_buf[i] = 0;
	}

}


void UART1_Init()
{
	char cc01, cc02, cc03;

	//UDRIE TXCIE RXCIE WAKEIE TXE RXE USARTEN U2X
	cc02 = ( 0 << 7 ) // 0   : Transmit data empty interrupt disable 
						//_1   : Transmit data empty interrupt enable
			|( 1 << 6 ) // 0   : Transmit completed interrupt disable 
						//_1   : Transmit completed interrupt enable
			|( 1 << 5 ) // 0   : Recv completed Interrupt disable 
						//_1   : Recv completed Interrupt enable
			|( 0 << 4 ) // 0   : Wake up interrupt disable
						//_1   : Wake up interrupt enable
			|( 1 << 3 ) // 0   : TX disable
						//_1   : TX enable
			|( 1 << 2 ) // 0   : Rx disable
						//_1   : Rx enable
			|( 1 << 1 ) // 0   : USART disable
						//_1   : USART enable
			|( 1 << 0 );// 0   : Double speed disable in Asyncronous mode
						//_1   : Double speed enable in Asyncronous mode

	//Synchronous mode 
	//UMSEL1 UMSEL0 UPM1 UPM0 USIZE2 USIZE1(UDORD) USIZE0(UCPHA) UCPOL
	cc01 = ( 0 << 7 ) // 00  : Asyncronous mode( Uart ) 
			|( 0 << 6 ) // 01  : Syncronouse mode
						//_11  : SPI mode
			|( 0 << 5 ) // 00  : No parity
			|( 0 << 4 ) // 10  : Even parity
						//_11  : Odd parity
			|( 0 << 3 ) // 000 : 5 bit data frame 
			|( 1 << 2 ) // 001 : 6 bit data frame   
			|( 1 << 1 ) // 010 : 7 bit data frame    
						// 011 : 8 bit data frame
						//_111 : 9 bit data frame
			|( 0 << 0 );// 0   : In syncronouse mode -> TXD change at rising, RXD change at falling
						// 1   : In syncronouse mode -> RXD change at rising, TXD change at falling


	//MASTER LOOPS DISXCK SPISS - USBS TX8 RX8
	cc03 = ( 0 << 7 ) 	// 0   : Slave mode ( XCL in ) in SPI and Syncronous mode
						//_1   : Master mode ( XCL out ) in SPI and Syncronous mode
			|( 0 << 6 ) // 0   : Normal mode
						//_1   : Loop back mode
			|( 0 << 5 ) // 0   : XCK out always ( when only USART is enabled ) ,in Sync mode
						//_1   : XCK out when only TX is tranmitting           ,in Sync mode
			|( 0 << 4 ) // 0   : SS out disable in SPI Master mode
						//_1   : SS out enable in SPI Master mode
			|( 0 << 3 ) //     : nothing
			|( 0 << 2 ) // 0   : 1 stop bit in UART mode	//V1.4
						//_1   : 2 stop bit in UART mode
			|( 0 << 1 ) // x   : 9th TX data bit in UART mode
			|( 0 << 0 );// x   : 9th RX data bit in UART mode 


	UCTRL12=0x00;
	UCTRL11=0x00;
	UCTRL13=0x00;

	UCTRL12=0x02;	//UDRIE TXCIE RXCIE WAKEIE TXE RXE USARTEN U2X
	UCTRL12=cc02;	//UDRIE TXCIE RXCIE WAKEIE TXE RXE USARTEN U2X
	UCTRL11=cc01;	//UMSEL1 UMSEL0 UPM1 UPM0 USIZE2 USIZE1(UDORD) USIZE0(UCPHA) UCPOL
	UCTRL13=cc03;  //MASTER LOOPS DISXCK SPISS - USBS TX8 RX8

	//UBAUD1 = 3;  //115200 bps : 4.000000 MHz, ubaud=3  (ERROR:8.5%, U2X=1)
	//UBAUD1 = 7;  //115200 bps : 7.372800 MHz, ubaud=7  (ERROR:0%, U2X=1)
	//UBAUD1 = 95; //9600	  bps : 7.372800 MHz, ubaud=95 (ERROR:0%, U2X=1)
	UBAUD1 = 95;

	USTAT1=0x80;	  //UDRE TXC RXC WAKE SOFTRST DOR FE PE 

	// Interrupt Priority Register (IP, IP1)
	//IP	|= 0x04;	//RX0(GROUP2)
	//IP1	|= 0x04;	//RX0(GROUP2)


	//IE1	|= 0x20;  //- - INT11E(RX1) INT10E(TX0) INT9E(RX0) INT8E(BOD) INT7E(IRI) INT6E(Reserved)
	//IE2	|= 0x01;  //- - INT17E(I2C) INT16E(T3) INT15E(T2) INT14E(T1) INT13E(T0) INT12E(TX1)

	_uart_rx_count = 0;
	_uart_tx_count = 0;


	//cc01 = UDATA1;
}




void RX1_Int()
{
	unsigned char RX0Data;

	RX0Data = UDATA1;

	if(USTAT1 & 0x03) 	//UDRE TXC RXC WAKE SOFTRST DOR FE PE
	{
	}

	else if(_uart_rx_count == 0)
	{
		_uart_rx_len = 8;
		_uart_rx_buf[0] = RX0Data;
		_uart_rx_count = 1;
	}
	
	while (_uart_rx_count < 7)
	{
		
		_uart_rx_buf[_uart_rx_count] = RX0Data;
		_uart_rx_count++;

		//RESET_IDLE;
	}
}


