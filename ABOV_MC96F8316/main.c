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
#include <stdio.h>
#include <string.h>


#define PWM0_FLAG 	0x01
#define PWM1_FLAG 	0x02
#define PWM2_FLAG 	0x04
#define PWM3_FLAG 	0x08

#define PWM_CTL_OFF		0
#define PWM_CTL_INC		1
#define PWM_CTL_DEC		2

#define PWM_CH_NUM		4

//======================================================
// interrupt routines
//======================================================

//======================================================
// peripheral setting routines
//======================================================


typedef struct _tagPWMINFO {
	int 	period;
	int 	duty;
	int 	default_duty;
	int 	next_duty;
	int 	count;
	int		out;
} PWMINFO;



static unsigned int _pwm_ctl_dec_flag = 0;
static unsigned int _pwm_ctl_inc_flag = 0;
static unsigned int _pwm_ctl_cnt = 0;

static unsigned int _pwm_flag_list[PWM_CH_NUM] = { PWM0_FLAG, PWM1_FLAG, PWM2_FLAG, PWM3_FLAG };



// ----------------------------------
static int _pwm_mask = 0x0f;	// Enable Mask : - - - - PWM3 PWM2 PWM1 PWM0
PWMINFO 	_pwm[PWM_CH_NUM];


static char _pwm_out;
static char pin15;


void UART1_Clear()
{
	UARTDR = 0xFF;
}



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

	pwm_setup( &_pwm[0], 10, 0 ); // 35
	pwm_setup( &_pwm[1], 12, 4 ); // 11
	pwm_setup( &_pwm[2], 12, 8 ); // 12
	pwm_setup( &_pwm[3], 12, 12 ); // 13

}

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
static char _pwm_out_motor;

void pwm_control_Motor()
{
	_pwm_out_motor = 0;
	// PWM Motor
	if( _pwm[0].count < _pwm[0].duty && (_pwm_mask & PWM0_FLAG) ) {
		_pwm_out_motor |= 0x20;	// P35
		P3 |= _pwm_out_motor;
	}

	else
	{
		P3 &= 0xDF;
	}
	
	_pwm[0].count++;

	
	if( _pwm[0].count >= _pwm[0].period ) {
		_pwm[0].count = 0;
		_pwm[0].duty  = _pwm[0].next_duty;
		
	}
	

}


void pwm_control_LED()
{

	_pwm_out = 0;
	
	// PWM 1
	if( _pwm[1].count < _pwm[1].duty && (_pwm_mask & PWM1_FLAG) ) {
		_pwm_out |= 0x02;	// P11
	}

	_pwm[1].count++;

	if( _pwm[1].count >= _pwm[1].period ) {
		_pwm[1].count = 0;
		_pwm[1].duty  = _pwm[1].next_duty;
	}


	// PWM 2
	if( _pwm[2].count < _pwm[2].duty && (_pwm_mask & PWM2_FLAG) ) {
		if( _pwm[2].count >= 4)
			_pwm_out |= 0x04;	// P12
	}

	_pwm[2].count++;

	if( _pwm[2].count >= _pwm[2].period ) {
		_pwm[2].count = 0;
		_pwm[2].duty  = _pwm[2].next_duty;
	}


	// PWM 3
	if( _pwm[3].count < _pwm[3].duty && (_pwm_mask & PWM3_FLAG) ) {
		if( _pwm[3].count >= 8)
			_pwm_out |= 0x08;	// P13
	}

	_pwm[3].count++;

	
	if( _pwm[3].count >= _pwm[3].period ) {
		_pwm[3].count = 0;
		_pwm[3].duty  = _pwm[3].next_duty;
	}

	
	
	
	_pwm_out |= 0xF1; // init p10 
	pin15 = P1 & 0xF0;
	
	P1 = ( pin15 | ~_pwm_out);

	
	/*
	  ex) pin 13, 12 high, 11 low 00001100
	       _pwm_out = 0b11111101
	      ~_pwm_out = 0b00000010
	      
				if P15 low:
				 pin 15 = 0b00100000
	       P1     = 0b00100010
	
				else (P15 high):
				 pin 15 = 0b00000000
				 P1 = 0b00000010	
	
	*/
	
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


void setMotorPWM()
{	

	if((UARTDR & 0x0F)== 0x00)
	{
		P1 &= 0x0F;
		pwm_enable(0, -1, -1, -1);
		pwm_setup( &_pwm[0], 10, 0 );
		UART1_Clear();
	}
	
	else
	{
		pwm_enable( 1, -1,-1,-1 );
		P1 |= 0x20;

		if((UARTDR & 0x0F) == 0x01) { pwm_setup( &_pwm[0], 10, 6 );}
		else if((UARTDR & 0x0F) == 0x02) { pwm_setup( &_pwm[0], 10, 7 );}
		else if((UARTDR & 0x0F) == 0x03) { pwm_setup( &_pwm[0], 10, 8 );}
		else if((UARTDR & 0x0F) == 0x04) { pwm_setup( &_pwm[0], 10, 9 );}
		else if((UARTDR & 0x0F) == 0x05) { pwm_setup( &_pwm[0], 10, 10 );}

		UART1_Clear();
	}
	
}

void setLedPWM(int color) { 
	
	/* 
		B : On
		G : Good
		Y : So So
		M : Bad
		R : very Bad
	
	*/

	if(color == 0){ // B : on
		pwm_enable(-1, 1, 1, 0);
		pwm_setup_control( PWM1_FLAG, PWM_CTL_DEC );
		pwm_setup_control( PWM2_FLAG, PWM_CTL_DEC );
		pwm_setup_control( PWM3_FLAG, PWM_CTL_INC );
		UART1_Clear();
	}
	
	else if(color == 1){ // G : Good
		pwm_enable(-1, 1, 0, 1);
		pwm_setup_control( PWM1_FLAG, PWM_CTL_DEC );
		pwm_setup_control( PWM2_FLAG, PWM_CTL_INC );
		pwm_setup_control( PWM3_FLAG, PWM_CTL_DEC );
		UART1_Clear();
	}
		
	else if(color == 2){ // Y : So So
		pwm_enable(-1, 0, 0, 1);
		pwm_setup_control( PWM1_FLAG, PWM_CTL_INC );
		pwm_setup_control( PWM2_FLAG, PWM_CTL_INC );
		pwm_setup_control( PWM3_FLAG, PWM_CTL_DEC );
		UART1_Clear();
	}

	
	else if(color == 3){ // M : Bad
		pwm_enable(-1, 0, 1, 0);
		pwm_setup_control( PWM1_FLAG, PWM_CTL_INC );
		pwm_setup_control( PWM2_FLAG, PWM_CTL_DEC );
		pwm_setup_control( PWM3_FLAG, PWM_CTL_INC );
		UART1_Clear();
	}

	else if(color == 4){ // R : very Bad
		pwm_enable(-1, 0, 1, 1);
		pwm_setup_control( PWM1_FLAG, PWM_CTL_INC );
		pwm_setup_control( PWM2_FLAG, PWM_CTL_DEC );
		pwm_setup_control( PWM3_FLAG, PWM_CTL_DEC );
		UART1_Clear();
	}

	
}

/*
void setRelayCtrl(int color) {
	
}

*/

void UART_init()
{
	// initialize UART interface
	// ASync. 9615bps N 8 1
	UARTCR2 = 0x02; 	// activate UART
	UARTCR1 = 0x06; 	// bit count, parity
	UARTCR2 |= 0x0D;	// interrupt, speed
	UARTCR3 = 0x00; 	// stop bit
	UARTBD = 0x0C;  	// baud rate
	UARTST=0x60;	  //UDRE TXC RXC WAKE SOFTRST DOR FE PE 
  // 0x40
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

static unsigned int _COLOR_LOOP_INDEX = 0;
	
void main()
{
	//int i;
	
	cli();          	// disable INT. during peripheral setting
	port_init();    	// initialize ports
	clock_init();   	// initialize operation clock
	UART_init();    	// initialize UART interface
	pwm_enable( 0, 1, 1, 1 ); // Motor/ R/ G/ B	
	pwm_init();
	pwm_control_Motor();
	pwm_control_LED();
	sei();          	// enable INT.


	// TODO: add your main code here
	UARTDR = 0xFF;
	
	setupOpMode();
	
	while(1)
	{	
		
		if ((UARTDR & 0x0F) < 0x06)
			setMotorPWM();		

		/*
		F0 : Motor Power On
		F1 : Motor Power 1
		F2 : Motor Power 2
		F3 : Motor Power 3
		F4 : Motor Power 4
		F5 : Motor Power 5 (Max)
		*/
		
		if (((UARTDR & 0xF0) >> 4) < 0x05)
		{
			switch ((UARTDR & 0xF0) >> 4)
			{
				case 0x00: 
					setLedPWM(0); 
					break;
				case 0x01: 
					setLedPWM(1);
					break;
				case 0x02: 
					setLedPWM(2);
					break;
				case 0x03: 
					setLedPWM(3);
					break;
				case 0x04:
					setLedPWM(4);
					break;

			}
		}
		
		pwm_control_Motor();
		pwm_control_LED();
		
		/*
		0F : LED Power On
		1F : Dust Good
		2F : Dust So So
		3F : Dust Bad
		4F : Dust Very Bad  (Max)
		*/
		
		
		/*
		for(i = 0; i < 1000; i++) _nop_();	// init delay

		switch (_COLOR_LOOP_INDEX)
		{
			case 0:
				setLedPWM(0);
				_COLOR_LOOP_INDEX++;
				break;
			
			case 1:
				setLedPWM(1);
				_COLOR_LOOP_INDEX++;
				break;
			
			case 2:
				setLedPWM(2);
				_COLOR_LOOP_INDEX++;
				break;
			
			case 3:
				setLedPWM(3);
				_COLOR_LOOP_INDEX++;
				break;
			
			case 4:
				setLedPWM(4);
				_COLOR_LOOP_INDEX++;
				break;
					
		}
		if (_COLOR_LOOP_INDEX == 4)
			_COLOR_LOOP_INDEX = 0;
		
		
		for (i= 0; i < 10; i++)
		{
			pwm_control_Motor();
			pwm_control_LED();
		}
		
		*/
		
		
	}
}
