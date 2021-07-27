//======================================================
// Function and global variables definition
//======================================================

void port_init();             	// initialize ports
void clock_init();            	// initialize operation clock
void UART_init();             	// initialize UART interface
void UART_write(unsigned char dat);	// write UART
unsigned char UART_read();    	// read UART

