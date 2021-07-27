//======================================================
// Main program routine
// - Device name  : MC96F8316
// - Package type : 28SOP
//======================================================
// For XDATA variable : V1.041.00 ~
#define		MAIN	1

// Generated    : Tue, Jul 27, 2021 (11:47:04)
#include	"MC96F8316.h"
#include	"func_def.h"

void main()
{
	cli();          	// disable INT. during peripheral setting
	port_init();    	// initialize ports
	clock_init();   	// initialize operation clock
	UART_init();    	// initialize UART interface
	sei();          	// enable INT.
	
	// TODO: add your main code here
	
	while(1);
}

//======================================================
// interrupt routines
//======================================================

//======================================================
// peripheral setting routines
//======================================================

unsigned char UART_read()
{
	unsigned char dat;
	
	while(!(UARTST & 0x20));	// wait
	dat = UARTDR;   	// read
	return	dat;
}

void UART_init()
{
	// initialize UART interface
	// ASync. 9615bps N 8 1
	UARTCR2 = 0x02; 	// activate UART
	UARTCR1 = 0x06; 	// bit count, parity
	UARTCR2 |= 0x0D;	// interrupt, speed
	UARTCR3 = 0x00; 	// stop bit
	UARTBD = 0x0C;  	// baud rate
}

void UART_write(unsigned char dat)
{
	while(!(UARTST & 0x80));	// wait
	UARTDR = dat;   	// write
}

void clock_init()
{
	// internal RC clock (1.000000MHz)
	// Nothing to do for the default clock
}

void port_init()
{
	// initialize ports
	//   4 : P35      out 
	//   8 : RXD      in  
	//   9 : TXD      out 
	//  10 : P25      in  
	//  11 : P24      out 
	//  20 : P13      out 
	//  21 : P12      out 
	//  22 : P11      out 
	P0IO = 0xFF;    	// direction
	P0PU = 0x00;    	// pullup
	P0OD = 0x00;    	// open drain
	P03DB = 0x00;   	// bit7~6(debounce clock), bit5~0=P35,P06~02 debounce
	P0   = 0x00;    	// port initial value

	P1IO = 0xFF;    	// direction
	P1PU = 0x00;    	// pullup
	P1OD = 0x00;    	// open drain
	P12DB = 0x00;   	// debounce : P23~20, P13~10
	P1   = 0x00;    	// port initial value

	P2IO = 0xDF;    	// direction
	P2PU = 0x00;    	// pullup
	P2OD = 0x00;    	// open drain
	P2   = 0x00;    	// port initial value

	P3IO = 0xFD;    	// direction
	P3PU = 0x00;    	// pullup
	P3OD = 0x00;    	// open drain
	P3   = 0x00;    	// port initial value

	// Set port functions
	P0FSR = 0x00;   	// P0 selection
	P1FSRH = 0x00;  	// P1 selection High
	P1FSRL = 0x00;  	// P1 selection Low
	P2FSR = 0x00;   	// P2 selection
	P3FSR = 0x01;   	// P3 selection
}

