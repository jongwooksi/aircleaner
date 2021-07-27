//======================================================
// Main program routine
// - Device name  : MC96FR332A
// - Package type : 28TSSOP
//======================================================
// For XDATA variable : V1.041.00 ~
#define		MAIN	1

// Generated    : Thu, Mar 17, 2016 (21:23:38)
#include	"MC96FR332A.h"
#include	"func_def.h"

#define PWM0_FLAG 	0x01
#define PWM1_FLAG 	0x02
#define PWM2_FLAG 	0x04
#define PWM3_FLAG 	0x08

#define PWM_CTL_OFF		0
#define PWM_CTL_INC		1
#define PWM_CTL_DEC		2

#define GFLAG_WAKEUP	0x01

// ----------------------------------
#define PWM_CH_NUM		4


#include <intrins.h>
#include "MC96FR332A.h"		  


unsigned int _i2c_tx_len, _i2c_tx_count;
unsigned int _i2c_rx_len, _i2c_rx_count;

bit _i2c_start_capture=0;
bit _i2c_rx_complete=0;
bit _i2c_tx_complete=0;

unsigned char _i2c_rx_err=0;
unsigned char _i2c_tx_err=0;
unsigned int _i2c_rx_buf[8] = {0};



typedef struct _tagPWMINFO {
	int 	period;
	int 	duty;
	int 	default_duty;
	int 	next_duty;
	int 	count;
	int		out;
} PWMINFO;


static unsigned int _OP_MODE = 0;
static unsigned int _OP_MODE_ON_CHANGE = 0;
static unsigned int _ENABLE_COLOR_LOOP = 0;
static unsigned int _COLOR_LOOP_INDEX = 0;

static unsigned int _btn0_pressed = 0;
static unsigned int _btn1_pressed = 0;
static unsigned int _btn0_elapse_cnt = 0;
static unsigned int _btn1_elapse_cnt = 0;

static unsigned int _pwm_ctl_dec_flag = 0;
static unsigned int _pwm_ctl_inc_flag = 0;
static unsigned int _pwm_ctl_cnt = 0;

static unsigned int _pwm_flag_list[PWM_CH_NUM] = { PWM0_FLAG, PWM1_FLAG, PWM2_FLAG, PWM3_FLAG };

static unsigned int _colorDelayCount = 0;

static unsigned int _global_ctl_flag = 0;

// ----------------------------------
static int _pwm_mask = 0x0f;	// Enable Mask : - - - - PWM3 PWM2 PWM1 PWM0
PWMINFO 	_pwm[PWM_CH_NUM];


static char _pwm_out;



//======================================================
// peripheral setting routines
//======================================================

void BOD_init()
{
	// initialize BOD (Brown out detector)
	// BODR bit2~1 = BODout selection
	// - default is 00 (BODout1)
	BODR = 0x01;    	// setting
}

void clock_init()
{
	// Nothing to do for the default clock
	//SCCR |= 0x10; 	// set CBYS
}


// initialize ports
void port_init()
{
	// P0
	P0IO = 0xFF;    	// direction
	P0PU = 0xFF;    	// pullup
	P0BPC = 0xFF;   	// BPC
	P0   = 0x00;    	// port initial value

	// P1
	P1IO = 0xFF;    	// direction
	P1PU = 0x00;    	// pullup
	P1OD = 0x00;    	// open drain
	P1BPC = 0x00;   	// BPC
	P1   = 0x00;    	// port initial value

	// P2
	P2IO = 0xFF;    	// direction
	P2PU = 0x00;    	// pullup
	P2OD = 0x00;    	// open drain
	P2BPC = 0x00;   	// BPC
	P2   = 0x00;    	// port initial value

	// P3
	P3IO = 0xBF;    	// direction
	P3PU = 0xFF;    	// pullup
	P3OD = 0x00;    	// open drain
	P3BPC = 0x00;   	// BPC
	P3   = 0x00;    	// port initial value

	PSR0 = 0xC0;    	// port selection
						// SDASWAP SCLSWAP SS0SWAP XCK0SWAP INT3SWAP INT2SWAP INT1SWAP INT0SWAP
						// INT3SWAP = 1 : External interrupt 3 is triggered on P15 instead of P22
						// INT2SWAP = 1 : External interrupt 2 is triggered on P14 instead of P21
						// INT1SWAP = 1 : External interrupt 1 is triggered on P13 instead of P37
						// INT0SWAP = 1 : External interrupt 0 is triggered on P12 instead of P36
}




//======================================================
// PWM
//======================================================

// period, duty unit : T1 interrupt interval
void pwm_setup( PWMINFO *pwm, int period, int duty )
{
	pwm->period 		= period;
	pwm->duty   		= duty;
	pwm->next_duty  	= duty;
	pwm->default_duty	= duty;
	pwm->count  	= 0;
	pwm->out 		= 0;
}


void pwm_init()
{

	pwm_setup( &_pwm[0], 10, 0 );
	pwm_setup( &_pwm[1], 10, 7 );
	pwm_setup( &_pwm[2], 10, 8 );
	pwm_setup( &_pwm[3], 10, 9 );

}

//======================================================
// Timer
//======================================================
void timer_init()
{
	//cli();

	// Enable T0 Timer
	IE2 	|= 0x02;

	T0CR 	= 0x97;	// T0EN T0_PE CAP0 T0CK2 T0CK1 T0CK0 T0CN T0ST
					//   1    0     0    1    0       1    1    1   (0x97)
					//		T0EN : Enable or disable Timer0
					//		T0_PE : T0 pin out enable
					//		CAP0 : Timer0 capture mode (0=Timer mode, 1=Capture mode)
					//		T0CK[2:0] : clock source
					//			(base Fsclk : 7.3728 MHz)
					//			000 : Fsclk / 2 	: 3.6864 MHz : 0.2712 us
					//			001 : Fsclk / 4		: 1.8432 MHz : 0.5415 us
					//			010 : Fsclk / 16	: 0.4608 MHz : 2.17   us
					//			...
					//			101 : Fsclk / 1024  : 7,200  Hz  : 0.1388 ms (1000/0.13888 = 720)
					//			110 : Fsclk / 4096  : 1,800  Hz  : 0.5555 ms
					//		T0CN : pause or continue counting (0=Pause 1=Continue)
					//		T0ST : start or stop (0=Stop 1=Clear and start)

	T0DR	= 72;	

	//sei();
}


//======================================================
void pwm_setup_control( unsigned int pwmflag, unsigned int ctl )
{
	if( ctl == PWM_CTL_OFF ) 
	{
		_pwm_ctl_inc_flag &= ~pwmflag;
		_pwm_ctl_dec_flag &= ~pwmflag;
	}
	else if( ctl == PWM_CTL_DEC ) 
	{
		_pwm_ctl_dec_flag |=  pwmflag;
		_pwm_ctl_inc_flag &= ~pwmflag;
	}
	else 
	{
		_pwm_ctl_dec_flag &= ~pwmflag;
		_pwm_ctl_inc_flag |=  pwmflag;
	}
}


//======================================================
void pwm_control()
{

	// 60, 70, 80, 90, 100

	_pwm_out = 0;

	// PWM 0
	if( _pwm[0].count < _pwm[0].duty && (_pwm_mask & PWM0_FLAG) ) {
		_pwm_out |= 0x01;	// P00
	}

	_pwm[0].count++;

	
	if( _pwm[0].count >= _pwm[0].period ) {
		_pwm[0].count = 0;
		_pwm[0].duty  = _pwm[0].next_duty;
	}


	// PWM 1
	if( _pwm[1].count < _pwm[1].duty && (_pwm_mask & PWM1_FLAG) ) {
		_pwm_out |= 0x02;	// P01
	}

	_pwm[1].count++;

	if( _pwm[1].count >= _pwm[1].period ) {
		_pwm[1].count = 0;
		_pwm[1].duty  = _pwm[1].next_duty;
	}


	// PWM 2
	if( _pwm[2].count < _pwm[2].duty && (_pwm_mask & PWM2_FLAG) ) {
		_pwm_out |= 0x04;	// P02
	}

	_pwm[2].count++;

	if( _pwm[2].count >= _pwm[2].period ) {
		_pwm[2].count = 0;
		_pwm[2].duty  = _pwm[2].next_duty;
	}


	// PWM 3
	if( _pwm[3].count < _pwm[3].duty && (_pwm_mask & PWM3_FLAG) ) {
		_pwm_out |= 0x08;	// P03
	}

	_pwm[3].count++;

	
	if( _pwm[3].count >= _pwm[3].period ) {
		_pwm[3].count = 0;
		_pwm[3].duty  = _pwm[3].next_duty;
	}


	P0 = _pwm_out;


}

void pwm_enable( int pwm0, int pwm1, int pwm2, int pwm3 )
{
	// 0:Disable 1:Enable -1:Don't care(not change)

	// B/G/R/Motor
	if( pwm0 != -1 ) { if( pwm0 ) _pwm_mask |= PWM0_FLAG; else _pwm_mask &= ~PWM0_FLAG; }
	if( pwm1 != -1 ) { if( pwm1 ) _pwm_mask |= PWM1_FLAG; else _pwm_mask &= ~PWM1_FLAG; }
	if( pwm2 != -1 ) { if( pwm2 ) _pwm_mask |= PWM2_FLAG; else _pwm_mask &= ~PWM2_FLAG; }
	if( pwm3 != -1 ) { if( pwm3 ) _pwm_mask |= PWM3_FLAG; else _pwm_mask &= ~PWM3_FLAG; }
}

//======================================================
void setupOpMode()
{
	pwm_setup_control( PWM0_FLAG, PWM_CTL_OFF );
	pwm_setup_control( PWM1_FLAG, PWM_CTL_OFF );
	pwm_setup_control( PWM2_FLAG, PWM_CTL_OFF );
	pwm_setup_control( PWM3_FLAG, PWM_CTL_OFF );

}



void I2C_Init(void)
{

	_i2c_tx_count = 0;
	_i2c_rx_count = 0;


	//IE2 |= 0x20;	// Enable I2C interrupt Enable	// - - INT17E(I2C) INT16E(T3) INT15E(T2) INT14E(T1) INT13E(T0) INT12E(UART-RX1)
	//;
	IP	= 0x20;	//I2C(GROUP5) 
	IP1	= 0x20;	//I2C(GROUP5) 

	I2CMR = 0x20; // I2C RESET	//IIF IICEN RESET INTEN ACKEN MASTER STOP START
	I2CMR = 0x00; // I2C clear	//IIF IICEN RESET INTEN ACKEN MASTER STOP START
	I2CMR = 0x20; // I2C RESET	//IIF IICEN RESET INTEN ACKEN MASTER STOP START
	I2CMR = 0x40; // I2C enable	//IIF IICEN RESET INTEN ACKEN MASTER STOP START
	I2CMR = 0x50;//IICEN, I2C interrupt enable, SLAVE default mode	//IIF IICEN RESET INTEN ACKEN MASTER STOP START


 	// SCL Low Period  = tsclk * (4 * SCLLR + 1)
	// SCL High Period = tsclk * (4 * SCLHR + 3)
	// fI2C = 1/ { tsclk * (4 * (SCLLR + SCLHR) + 4) }
	//
	// fI2C = 200 kHz in (SCLLR + SCLHR) = 4 at 4MHz(tsclk = 0.25 us)
	I2CSCLLR = 2;
	I2CSCLHR = 2; // 
	I2CSDAHR = 2; // SDA hold time 
	//
	//= 0xFF;	  //GCALL TEND STOP SSEL MLOST BUSY TMODE RXACK

	//I2CSAR = SELF_ADDRESS; // self address is 0xA0
	I2CMR = 0x40 | 0x10 | 0x08;	//IICEN+INTEN+ACKEN; //IIF IICEN RESET INTEN ACKEN MASTER STOP START
}



 void I2C_Clear(void)
{
	int i;

	for(i = 0; i < 8; i++) {
		_i2c_rx_buf[i] = 0;
	}

	_i2c_rx_count = 0;
	_i2c_rx_err = 0;
}



void setPWM()
{
	if (_i2c_rx_buf == 0x00 )
		pwm_enable(0, -1, -1, -1);
	
	else if (_i2c_rx_buf == 0x01 )
		pwm_setup( &_pwm[0], 10, 6 );
	
	else if (_i2c_rx_buf == 0x02 )
		pwm_setup( &_pwm[0], 10, 7 );
	
	else if (_i2c_rx_buf == 0x04 )
		pwm_setup( &_pwm[0], 10, 8 );
	
	else if (_i2c_rx_buf == 0x08 )
		pwm_setup( &_pwm[0], 10, 9 );
	
	else if (_i2c_rx_buf == 0x10 )
		pwm_setup( &_pwm[0], 10, 10 );
	
	I2C_Clear();
}


void I2C_do_slave(void)
{
	unsigned char I2C_Data;

	if(I2CSR & 0x01) // RXACK OK?	 //GCALL TEND STOP SSEL MLOST BUSY TMODE RXACK
	{ 
		if(_i2c_rx_count < _i2c_rx_len)
		{ 
			I2C_Data = I2CDR;
			_i2c_rx_buf[ _i2c_rx_count ] = I2C_Data;

			_i2c_rx_count++;

			if(_i2c_rx_count >= _i2c_rx_len) 
			{
				_i2c_rx_complete = 1;
				_i2c_rx_err = 0;
				_i2c_start_capture = 0;
				setPWM();
			}

		}
		else  
		{
			_i2c_rx_complete = 1;
			_i2c_rx_err = 1;
			_i2c_start_capture = 0;
			I2CMR |= 0x02; // STOP generation 	//IIF IICEN RESET INTEN ACKEN MASTER STOP START
		}

	}
	else  // RXACK fail?
	{
		_i2c_rx_complete = 1;
		_i2c_rx_err = 2;
		_i2c_start_capture = 0;

		I2CMR |= 0x02; // STOP generation 	//IIF IICEN RESET INTEN ACKEN MASTER STOP START
	}

}



//======================================================
// main
//======================================================
void main()
{
	int i;

	cli();          	// disable INT. during peripheral setting
	port_init();    	// initialize ports
	clock_init();   	// initialize operation clock
	pwm_enable( 1, 1, 1, 1 );	
	pwm_init();
	timer_init();
	
	I2C_Init();
	I2C_Clear();

	sei();          	// enable INT.


	for(i = 0; i < 1000; i++) _nop_();	// init delay
	setupOpMode();

	while(1) {
		I2C_do_slave();
		pwm_control();

	}
}
