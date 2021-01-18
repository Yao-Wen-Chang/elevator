//B073040044

/******************************************************************************/
/* MEASURE.C: Remote Measurement Recorder                                     */
/******************************************************************************/
/* This file is part of the uVision/ARM development tools.                    */
/* Copyright (c) 2005-2006 Keil Software. All rights reserved.                */
/* This software may only be used under the terms of a valid, current,        */
/* end user licence from KEIL for a compatible version of KEIL software       */
/* development tools. Nothing else gives you the right to use this software.  */
/******************************************************************************/

#include <stdio.h>                       /* standard I/O .h-file              */
#include <math.h>
#include <ctype.h>                       /* character functions               */
#ifndef MCB2130
  #include <LPC21xx.H>                   /* LPC21xx definitions               */
#else
  #include <LPC213x.H>                   /* LPC213x definitions               */
  #define  ADCR    AD0CR
  #define  ADDR    AD0DR
#endif

#include "measure.h"                     /* global project definition file    */

const char menu[] =
   "\n"
   "+***************** REMOTE MEASUREMENT RECORDER *****************+\n"
   "| This program is a simple Measurement Recorder. It is based on |\n"
   "| the LPC2129 and records the state of Port 0 and the voltage   |\n"
   "| on the four analog inputs AIN0 trough AIN3.                   |\n"
   "+ command -+ syntax -----+ function ----------------------------+\n"
   "| Read     | R [n]       | read <n> recorded measurements       |\n"
   "| Display  | D           | display current measurement values   |\n"
   "| Time     | T hh:mm:ss  | set time                             |\n"
   "| Interval | I mm:ss.ttt | set interval time                    |\n"
   "| Clear    | C           | clear measurement records            |\n"
   "| Quit     | Q           | quit measurement recording           |\n"
   "| Start    | S           | start measurement recording          |\n"
   "+----------+-------------+--------------------------------------+\n";

struct interval setinterval;                /* interval setting values        */
struct interval interval;                   /* interval counter               */

volatile int measurement_interval = 0;      /* measurement interval over      */
volatile int mdisplay = 0;                  /* measurement display requested  */
volatile int startflag = 0;                 /* start measurement recording    */

struct mrec current;                        /* current measurements           */

#define SCNT  6                             /* number of records              */

struct mrec save_record[SCNT];              /* buffer for measurements        */
int sindex;                                 /* save index                     */
int savefirst;                              /* save first index               */

char ERROR [] = "\n*** ERROR: %s\n";        /* ERROR message string in code   */

#define WRONGINDEX 0xffff                   /* error signal for wrong index   */


int time_counter; // count time period
int tmp_counter = 0;
int tmp = 0;
unsigned int bit_arr_tokens[32] = {0};
unsigned int bit_state[32] = {0};
unsigned int button[32] = {0}; // store current lighted button
/* Default Interrupt Function: may be called when timer ISR is disabled */
void DefISR (void) __irq  {
  ;
}

/******************************************************************************/
/*                Timer Counter 0 interrupt service function                  */
/*                executes each 1ms                                           */
/******************************************************************************/


void petrinet(){
	unsigned int read_port0;
	unsigned int read_port1;
	int i, j;
	read_port0 = IOPIN0;
	read_port1 = IOPIN1;
	//printf("%x\n", read_port0);
	read_port0 = read_port0 >> 10;
	
	for(i = 10; i <= 25; i++) {
		bit_arr_tokens[i] = (read_port0 % 2);
		if(bit_arr_tokens[i] && i >= 10 && i <= 23)
			button[i] = 1;
		else if(bit_arr_tokens[i] && i == 24)
			button[8] = 1;
		else if(bit_arr_tokens[i] && i == 25)
			button[9] = 1;
		read_port0 /= 2;
		
	}
	//printf("%lx\n", IOPIN0);
	read_port0 = IOPIN0;
	read_port1 = IOPIN1;
	
	
	// store left elevator state  
	// elevetor start at 1st floor
	//bit_state[0] = 0; // 3Fclosed
	//bit_state[1] = 0; // 3F<->
	//bit_state[2] = 0; // 3F>-<
	//bit_state[3] = 0; // 3Fopen
	//bit_state[4] = 0; // 2Fclosed
	//bit_state[5] = 0; // 2F<->
	//bit_state[6] = 0; // 2F>-<
	//bit_state[7] = 0; // 2Fopen
	//bit_state[8] = 1; // 1Fclosed
	//bit_state[9] = 0; // 1F<->
	//bit_state[10] = 0; // 1F>-<
	//bit_state[11] = 0; // 1Fopen
	//bit_state[12] = 0; // 2.5Fdown
	//bit_state[13] = 0; // 1.5Fdown
	//bit_state[14] = 0; // 1.5Fup
	//bit_state[15] = 0; // 2.5Fup
	if(bit_state[0]) { // 3Fclosed
		//read_port1 &= 0xcfffffff; // set P1.28.29 go low
		if((button[11] && button[19]) || (bit_arr_tokens[11] && bit_arr_tokens[19])) { // token either down & 3
			button[8] = 0;
			button[11] = 1;
			button[19] = 1;
			bit_state[1] = 1;
			bit_state[0] = 0;
		}
		else if(button[8] || bit_arr_tokens[24]) { // token<->
			button[8] = 1;
			button[11] = 0;
			button[19] = 0;
			bit_state[1] = 1;
			bit_state[0] = 0;
		}
		else if(button[11] || bit_arr_tokens[11]) { // token 3
			button[8] = 0;
			button[11] = 1;
			button[19] = 0;
			bit_state[1] = 1;
			bit_state[0] = 0;
		}
		else if(button[19] || bit_arr_tokens[19])	{ // token down
			button[8] = 0;
			button[11] = 0;
			button[19] = 1;
			bit_state[1] = 1;
			bit_state[0] = 0;
		}
		else if(button[9] || bit_arr_tokens[25]) { // 1F inner button lighted
			button[9] = 1;
			bit_state[0] = 0;
			bit_state[12] = 1;
		} 
		else if(button[10] || bit_arr_tokens[10]) { // 2F inner button lighted
			button[10] = 1;
			bit_state[0] = 0;
			bit_state[12] = 1;
		}
		else if(button[16] || bit_arr_tokens[16]) { // 1F outer button up lighted
			button[16] = 1;
			bit_state[0] = 0;
			bit_state[12] = 1;
		}
		else if(button[17] || bit_arr_tokens[17]) { // 2F outer button up lighted
			button[17] = 1;
			bit_state[0] = 0;
			bit_state[12] = 1;
		}
		else if(button[18] || bit_arr_tokens[18]) { // 2F outer button down lighted
			button[18] = 1;
			bit_state[0] = 0;
			bit_state[12] = 1;
		}	
	}
	else if(bit_state[1]) { // 3F<->
		IOSET1 = (1 << 28) | (1 << 29); // set P1.28.29 go high
		if((read_port0 & 0x100000) == 0x100000) { // P0.20 go high door opened
			button[8] = 0;
			button[11] = 0;
			button[19] = 0;
			IOCLR1 = (1 << 28) | (1 << 29); // set P1.28.29 go low
			bit_state[3] = 1;
			bit_state[1] = 0;
		}	
	}
	else if(bit_state[2]) { // 3F>-<
		if(bit_arr_tokens[24]) { // door reopen
			bit_state[1] = 1;
			bit_state[2] = 0;
		}	
		else if((read_port0 & 0x100000) == 0x100000) {
			bit_state[0] = 1;
			bit_state[2] = 0;
		}	
		
	}
	else if(bit_state[3]) { // 3Fopen
		if(time_counter == 5000) { //door closed
			IOSET1 = (1 << 28) | (1 << 29); // set P1.28.29 go high
			bit_state[2] = 1;
			bit_state[3] = 0;
			tmp_counter = 0;
			time_counter = 0;
		}	
		else if((read_port1 & 0xf0000000) == 0x00000000 && tmp_counter == 0) { // door opened
			time_counter = 0;
			tmp_counter ++;
		}	
		else 
			time_counter ++;
		
	}
	else if(bit_state[4]) { // 2F closed
		//IOCLR1 = (1 << 28) | (1 << 29); // set P1.28.29 go low
		
		if((button[10] && button[18] && button[17]) || (bit_arr_tokens[10] && bit_arr_tokens[18] && bit_arr_tokens[17])) { // token either down & up & 2
			button[8] = 0;
			button[10] = 1;
			button[17] = 1;
			button[18] = 1;
			bit_state[5] = 1;
			bit_state[4] = 0;
		}
		else if(button[8] || bit_arr_tokens[24]) { // token<->
			button[8] = 1;
			button[10] = 0;
			button[17] = 0;
			button[18] = 0;
			bit_state[5] = 1;
			bit_state[4] = 0;
		}
		else if(button[10] || bit_arr_tokens[10]) { // token 3
			button[8] = 0;
			button[10] = 1;
			button[17] = 0;
			button[18] = 0;
			bit_state[5] = 1;
			bit_state[4] = 0;
			
		}
		else if(button[18] || bit_arr_tokens[18])	{ // token down
			button[8] = 0;
			button[10] = 0;
			button[17] = 0;
			button[18] = 1;
			bit_state[5] = 1;
			bit_state[4] = 0;
			
		}
		else if(button[17] || bit_arr_tokens[17])	{ // token up
			button[8] = 0;
			button[10] = 0;
			button[17] = 1;
			button[18] = 0;
			bit_state[5] = 1;
			bit_state[4] = 0;
			
		} 
		else if(button[9] || bit_arr_tokens[25]) { // 1F innner button lighted
			button[9] = 1;
			bit_state[4] = 0;
			bit_state[13] = 1; // 1.5F down
		}	
		else if(button[16] || bit_arr_tokens[16]) { // 1F outer button up lighted
			button[16] = 1;
			bit_state[4] = 0;
			bit_state[13] = 1; // 1.5F down
		}	
		else if(button[11] || bit_arr_tokens[11]) { // 3F inner button lighted
			button[11] = 1;
			bit_state[4] = 0;
			bit_state[15] = 1; // 2.5F up
		}	
		else if(button[19] || bit_arr_tokens[19]) { // 3F outer button down lighted
			button[19] = 1;
			bit_state[4] = 0;
			bit_state[15] = 1; // 2.5F up
		}		
	}
	else if(bit_state[5]) { // 2F <->
		IOSET1 = (1 << 28) | (1 << 29); // set P1.28.29 go high
		if((read_port0 & 0x100000) == 0x100000) { // P0.20 go high door opened
			button[8] = 0;
			button[10] = 0;
			button[17] = 0;
			button[18] = 0;
			IOCLR1 = (1 << 28) | (1 << 29); // set P1.28.29 go low
			bit_state[7] = 1;
			bit_state[5] = 0;
		}
	}
	else if(bit_state[6]) {  // 2F >-<
		if(bit_arr_tokens[24] == 1) { // door reopen
			bit_state[5] = 1;
			bit_state[6] = 0;
		}	
		else if((read_port0 & 0x100000) == 0x100000) {
			bit_state[4] = 1;
			bit_state[6] = 0;
		}
	
	}
	else if(bit_state[7]) {  // 2F open 
		if(time_counter == 5000) { //door closed
			IOSET1 = (1 << 28) | (1 << 29); // set P1.28.29 go high
			bit_state[6] = 1;
			bit_state[7] = 0;
			tmp_counter = 0;
			time_counter = 0;
		}	
		else if((read_port1 & 0xf0000000) == 0x00000000 && tmp_counter == 0) { // door opened
			time_counter = 0;
			tmp_counter ++;
		}	
		else 
			time_counter ++;
	}
	else if(bit_state[8]) { // 1F closed
		//IOCLR1 = (1 << 28) | (1 << 29); // set P1.28.29 go low
		if((button[9] && button[16]) || (bit_arr_tokens[25] && bit_arr_tokens[16])) { // token either down & up & 2
			button[8] = 0;
			button[9] = 1;
			button[16] = 1;
			//printf("1\n");
			bit_state[9] = 1;
			bit_state[8] = 0;
			//time_counter = 0;
			//read_port1 |= (3 << 28);
		}
		else if(button[8] || bit_arr_tokens[24]) { // token<->
			button[8] = 1;
			button[9] = 0;
			button[16] = 0;
			//printf("2\n");
			bit_state[9] = 1;
			bit_state[8] = 0;
			//read_port1 |= (3 << 28);
		}
		else if(button[9] || bit_arr_tokens[25]) { // token 1
			button[8] = 0;
			button[9] = 1;
			button[16] = 0;
			//printf("3\n");
			bit_state[9] = 1;
			bit_state[8] = 0;
			//read_port1 |= (3 << 28);
		}
		else if(button[16] || bit_arr_tokens[16])	{ // token up
			//printf("check\n");
			button[8] = 0;
			button[9] = 0;
			button[16] = 1;
			bit_state[9] = 1;
			bit_state[8] = 0;
			//read_port1 |= (3 << 28);
		}	
		else if(button[10] || bit_arr_tokens[10]) { // 2F inner button lighted
			button[10] = 1;
			bit_state[14] = 1;
			bit_state[8] = 0;
			
		}
		else if(button[11] || bit_arr_tokens[11]) { // 3F innner button lighted
			button[11] = 1;
			bit_state[14] = 1;
			bit_state[8] = 0;
		}
		else if(button[19] || bit_arr_tokens[19]) { // 3F out button down lighted
			button[19] = 1;
			bit_state[14] = 1;
			bit_state[8] = 0;
		}
		else if(button[17] || bit_arr_tokens[17]) { // 2F out button up lighted
			button[17] = 1;
			bit_state[14] = 1;
			bit_state[8] = 0;
		}
		else if(button[18] || bit_arr_tokens[18]) { // 2F out button down lighted
			button[18] = 1;
			bit_state[14] = 1;
			bit_state[8] = 0;
		}
		
	}
	else if(bit_state[9]) { // 1F <->
		IOSET1 = (1 << 28) | (1 << 29); // set P1.28.29 go high
		//printf("%x\n", read_port0);
		if((read_port0 & 0x100000) == 0x100000) { // P0.20 go high door opened
			button[8] = 0;
			button[9] = 0;
			button[16] = 0;
			IOCLR1 = (1 << 28) | (1 << 29); // set P1.28.29 go low
			bit_state[11] = 1;
			bit_state[9] = 0;
			
		}
	}
	else if(bit_state[10]) { // 1F >-<
		if(bit_arr_tokens[24]) { // door reopen
			bit_state[9] = 1;
			bit_state[10] = 0;
		}	
		else if((read_port0 & 0x100000) == 0x100000) {
			bit_state[8] = 1;
			bit_state[10] = 0;
		}
	
	}
	else if(bit_state[11]) { // 1F open 
		if(time_counter == 5000) { //door closed
			IOSET1 = (1 << 28) | (1 << 29); // set P1.28.29 go high
			bit_state[10] = 1;
			bit_state[11] = 0;
			tmp_counter = 0;
			time_counter = 0;
		}	
		else if((read_port1 & 0xf0000000) == 0x00000000 && tmp_counter == 0) { // door opened
			time_counter = 0;
			tmp_counter ++;
		}	
		else 
			time_counter ++;
	}
	else if(bit_state[12]) { // 2.5F down
		IOSET1 = 1 << 29; // set P1.29 to high
		IOCLR1 = 1 << 28; // set P1.28 to low
		if((read_port0 & 0x200000) == 0x200000) { // midfloor go low
			if(button[10] || button[17] || button[18]) {
				bit_state[4] = 1;
				IOSET1 = 1 << 28;
			}	
			else if(button[9] || button[16]) {
				bit_state[13] = 1;
			}
			bit_state[12] = 0;
			
		}	
	}
	else if(bit_state[13]) { // 1.5F down
		IOSET1 = 1 << 29; // set P1.29 to high
		IOCLR1 = 1 << 28; // set P1.28 to low
		if((read_port0 & 0x200000) == 0x200000) { // midfloor go low
			bit_state[8] = 1;
			bit_state[13] = 0;
			IOCLR1 = (1 << 28) | (1 << 29);
		}	
	}
	else if(bit_state[14]) { // 1.5F up
		IOSET1 = 1 << 29; // set P1.29 to high
		IOCLR1 = 1 << 28; // set P1.28 to low
		
		if((read_port0 & 0x200000) == 0x200000) { // midfloor high
			if(button[10] || button[17] || button[18]) {
				bit_state[4] = 1;
				IOCLR1 = (1 << 28) | (1 << 29);
			}
			else if(button[11] || button[19]) { // keep going up
				bit_state[15] = 1;
			}
			bit_state[14] = 0;
			
		}	
	}
	else if(bit_state[15]) { // 2.5F up
		IOSET1 = 1 << 29; // set P1.29 to high
		IOCLR1 = 1 << 28; // set P1.28 to low
		
		if((read_port0 & 0x200000) == 0x200000) { // midfloor go low
			bit_state[0] = 1;
			bit_state[15] = 0;
			IOCLR1 = (1 << 28) | (1 << 29);
		}	
	}

}
__irq void tc0 (void) {
	unsigned long read_port0;
	unsigned long read_port1;
	int i;
	
	//read_port0 = IOPIN0; // read P0.8..23
	
	petrinet(); // update token
	
	read_port0 = IOPIN0; // read P0.8..23
	read_port1 = IOPIN1; // read P1.16..31
	
	
	// set button to P1
	//printf("check\n");
	for(i = 8; i <= 19; i++) {
		if(button[i])
			IOSET1 = (1 << (i+8));
		else 
			IOCLR1 = (1 << (i+8)); 
	}
	
	
	//IOPIN1 = (((read_port0 & 0xffff00) >> 8)& 65535) << 16; // test random
	T0IR = 1;                                    /* Clear interrupt flag        */
	VICVectAddr = 0;                             /* Acknowledge Interrupt       */
}





/******************************************************************************/
/***************************      MAIN PROGRAM      ***************************/
/******************************************************************************/
int main (void)  {                             /* main entry for program      */
	






	PINSEL1 = 0x15400000;                        /* Select AIN0..AIN3           */
	IODIR0  = 0x00000000;                          /* P0.8..23 defined as Iutputs*/	
	IODIR1  = 0xffff0000;							/* P1.16..31 defined as Outputs*/	
	IOCLR1  = 0xffffffff;
	ADCR    = 0x002E0401;                        /* Setup A/D: 10-bit @ 3MHz    */
	// set initial elevaor state
	
	
	init_serial ();                              /* initialite serial interface */

	/* setup the timer counter 0 interrupt */
	T0MR0 = 14999;                               /* 1mSec = 15.000-1 counts     */
	T0MCR = 3;                                   /* Interrupt and Reset on MR0  */
	T0TCR = 1;                                   /* Timer0 Enable               */
	VICVectAddr0 = (unsigned long)tc0;           /* set interrupt vector in 0   */
	VICVectCntl0 = 0x20 | 4;                     /* use it for Timer 0 Interrupt*/
	VICIntEnable = 0x00000010;                   /* Enable Timer0 Interrupt     */
	VICDefVectAddr = (unsigned long) DefISR;     /* un-assigned VIC interrupts  */
	bit_state[0] = 0; // 3Fclosed
	bit_state[1] = 0; // 3F<->
	bit_state[2] = 0; // 3F>-<
	bit_state[3] = 0; // 3Fopen
	bit_state[4] = 0; // 2Fclosed
	bit_state[5] = 0; // 2F<->
	bit_state[6] = 0; // 2F>-<
	bit_state[7] = 0; // 2Fopen
	bit_state[8] = 1; // 1Fclosed
	bit_state[9] = 0; // 1F<->
	bit_state[10] = 0; // 1F>-<
	bit_state[11] = 0; // 1Fopen
	bit_state[12] = 0; // 2.5Fdown
	bit_state[13] = 0; // 1.5Fdown
	bit_state[14] = 0; // 1.5Fup
	bit_state[15] = 0; // 2.5Fup
	
	
	

	while(1) {
		printf("\t  [");
		if(button[19])
			printf("v]\n\n");
		else 
			printf(" ]\n\n");
		if(button[9])
			printf("[%d",1);
		else 
			printf("[ ");
		if(button[10])
			printf("%d",2);
		else 
			printf(" ");
		if(button[11])
			printf("%d]  ",3);
		else 
			printf(" ]  ");
		if(button[17])
			printf("  [^");
		else
			printf("  [ ");
		if(button[18])
			printf("v]\n");
		else 
			printf(" ]\n");
		// <=> opening , >=< closing , === opened, ||| closed 
		if(bit_state[3] || bit_state[7] || bit_state[11])
			printf("|===|\n\n");
		else if(bit_state[0] || bit_state[4] || bit_state[8])
			printf(" ||| \n\n");
		else if(bit_state[1] || bit_state[5] || bit_state[9])
			printf("|<=>|\n\n");
		else if(bit_state[2] || bit_state[6] || bit_state[10])
			printf("|>=<|\n\n");
		else if(bit_state[12] || bit_state[13])
			printf("vvvvvvv\n\n");
		else if(bit_state[14] || bit_state[15])
			printf("^^^^^^^\n\n");
		printf("\t  ");
		if(button[16])
			printf("[^]\n************\n");
		else 
			printf("[ ]\n************\n");
		/*
		if(bit_state[0]) { // 3F door closed
			printf("3F door closed\n");
		}	
		else if(bit_state[3]) { // 3F door opened
			printf("3F door opened\n");	
		}	
		else if(bit_state[1]) { // 3F <-->
			printf("3F door is opening\n");
		}
		else if(bit_state[2]) { // 3F >--<	
			printf("3F door is closing\n");
		}	
		else if(bit_state[4]) { // 2F door closed
			printf("2F door closed\n");
		}	
		else if(bit_state[7]) { // 2F door opened 
			printf("2F door opened\n");
		}	
		else if(bit_state[5]) { // 2F <-->
			printf("2F door is opening\n");
		}
		else if(bit_state[6]) { // 2F >--<
			printf("2F door is closing\n");
		}	
		else if(bit_state[8]) { // 1F door closed	
			printf("1F door closed\n");
		}	
		else if(bit_state[11]) { // 1F door opened
			printf("1F door opened\n");
		}
		else if(bit_state[9]) { // 1F <-->
			printf("1F door is opening\n");
		}
		else if(bit_state[10]) { // 1F >--<
			printf("1F door is closing\n");
		}	
		else if(bit_state[12]) { // 2.5Fdown
			printf("2.5F down\n");
		}	
		else if(bit_state[13]) { // 1.5Fdown
			printf("1.5F down\n");
		}	
		else if(bit_state[14]) { // 1.5Fup
			printf("1.5F up\n");
		}	
		else if(bit_state[15]) { // 2.5Fup
			printf("2.5F up\n");
		}
		else 
			printf("stay\n");
		*/	
	}
 
}
