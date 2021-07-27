
#ifndef __UART_H__
#define __UART_H__

/*

*/

// ---------------------------------------------
// Constant Definition
// ---------------------------------------------



// ---------------------------------------------
// Global Variables
// ---------------------------------------------
extern unsigned int _uart_tx_count;
extern unsigned int _uart_rx_count;
extern unsigned int _uart_tx_len;
extern unsigned int _uart_rx_len;

extern bit _uart_tx_complete;
extern bit _uart_rx_complete;

extern unsigned char _uart_tx_err;
extern unsigned char _uart_rx_err;



// ---------------------------------------------
// Function Prototypes
// ---------------------------------------------
void UART1_Init(void);
void UART1_disable(void);
void UART1_Clear(void);
void uart1_TXstart( const char * cmd );
void uart1_check_recv_data(void);


int uart1_wait_tx_complete(void);

extern void uart0_TXstart( const char * cmd );


#endif//__UART_H__