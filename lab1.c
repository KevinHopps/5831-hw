#define KEVIN 0
#define AMY 1
#define CHOICE KEVIN
#if CHOICE == KEVIN
#include <pololu/orangutan.h>
#include "kio.h"
#include "kserial.h"
#include "ktimers.h"

#define RED_PIN IO_C0
#define YELLOW_PIN IO_A0
#define GREEN_PIN IO_D5

// PD5 is OC1A? Need to set DDRD pin 5 output1
// CTC = clear timer on compare match
// PWM = Pulse Width Modulation
// Setup a 16-bit timer counter with PWM to toggle an LED without using software.
// See table page 132-134.
//
void spin(volatile int32_t n)
{
	while (--n >= 0)
		continue;
}

static const uint32_t kLoopsPerSec = 2L * 424852L; // by experiment -- see spinCount.c

void kdelay_ms(int ms)
{
	spin((kLoopsPerSec * ms) / 1000);
}

void busy_blink(int nsec)
{
	KIORegs io = getIORegs(RED_PIN);
	s_println("busy_blink");
	setDataDir(&io, OUTPUT);
	int value = 1;
	nsec *= 2;
	while (--nsec >= 0)
	{
		setIOValue(&io, value);
		value ^= 1;
		kdelay_ms(500);
	}
	set_digital_output_value(&io, 0);
}

// This function is called by the timer interrupt routine
// that is setup by setup_CTC_timer, called by main().
//
static void ctc_callback(void* arg)
{
	uint32_t* n = (uint32_t*)arg;
	++(*n);
}

void ctc_blink(int nsec)
{
	struct IOStruct io;
	get_io_registers(&io, YELLOW_PIN);
	volatile uint32_t callback_count = 0;

	int whichTimer = 0;
	int frequency = 1000;
	Callback func = ctc_callback;
	void* arg = &callback_count;

	s_println("calling setup_CTC_timer(%d, %d, 0x%x, 0x%x)",
		whichTimer, frequency, func, arg);

	setup_CTC_timer(whichTimer, frequency, func, arg);

	s_println("sei()");
	sei();	// enable interrupts

	uint32_t nextBreak = 500;
	int value = 1;
	set_data_direction(&io, OUTPUT);
	set_digital_output_value(&io, value);
	nsec *= 2;
	while (--nsec >= 0)
	{
		value ^= 1;
		set_digital_output_value(&io, value);
		while (callback_count < nextBreak)
			continue;
		nextBreak += 500;
	}
	set_digital_output_value(&io, 0);

	cli(); // disable interrupts
}

int main()
{
	s_println("Hello");
	int nsec = 5;
	while (1)
	{
		ctc_blink(nsec);
		busy_blink(nsec);
	}
}
#else // #if CHOICE == KEVIN
#define ECHO2LCD

#include <pololu/orangutan.h>

#include "LEDs.h"
#include "timer.h"
#include "usart.h"
#include "menu.h"

//Gives us uintX_t (e.g. uint32_t - unsigned 32 bit int)
//On the ATMega128 int is actually 16 bits, so it is better to use
//  the int32_t or int16_t so you know exactly what is going on
#include <inttypes.h> //gives us uintX_t

// useful stuff from libc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// GLOBALS
volatile uint32_t G_yellow_ticks = 0;
volatile uint32_t G_ms_ticks = 0;

volatile uint16_t G_red_period = 1000;
volatile uint16_t G_green_period = 1000;
volatile uint16_t G_yellow_period = 1000;

volatile uint16_t G_release_red = 0;

volatile uint32_t G_red_toggles = 0;
volatile uint32_t G_green_toggles = 0;
volatile uint32_t G_yellow_toggles = 0;

volatile uint8_t G_flag = 0; // generic flag for debugging

int main(void) {
	// -------------------------------------------------------------
	// This program teaches you about the various ways to "schedule" tasks.
	// You can think of the three blinking LEDs as separate tasks with 
	// strict timing constraints.
	//
	// As you build your program, think about the guarantees that you can 
	// make regarding the meeting of deadlines. Think about how the CPU
	// is "communicating" with the LEDs. Obviously, everything is output,
	// but what if it was input? Also think about how data can be passed
	// among tasks and how conflicts might arise.
	//
	// You will construct this program in pieces.
	// First, establish WCET analysis on a for loop to use for timing.
	// Use the for loop to blink the red LED.
	// Next, set up a system 1 ms software timer, and use that to "schedule" the blink
	// inside a cyclic executive.
	//
	// Blink the yellow LED using a separate timer with a 100ms resolution.
	// Blink the LED inside the ISR.
	//
	// Finally, blink the green LED by toggling the output on a pin using
	// a Compare Match. This is the creation of a PWM signal with a very long period.
	//	
	// --------------------------------------------------------------

	int i;

	// Used to print to serial comm window
	char tempBuffer[32];
	int length = 0;
	
	// Ininitialization here.
	lcd_init_printf();	// required if we want to use printf() for LCD printing
	init_LEDs();
	init_timers();
	init_menu();	// this is initialization of serial comm through USB
	
	clear();	// clear the LCD

	//enable interrupts
	sei();
	
	while (1) {
		/* BEGIN with a simple toggle using for-loops. No interrupt timers */

		// toggle the LED. Increment a counter.
		LED_TOGGLE(RED);
		G_red_toggles++;
		length = sprintf( tempBuffer, "R toggles %d\r\n", G_red_toggles );
		print_usb( tempBuffer, length );
#ifdef ECHO2LCD
		lcd_goto_xy(0,0);
		printf("R:%d ",G_red_toggles);
#endif

		// create a for-loop to kill approximately 1 second
		for (i=0;i<100;i++) {
			WAIT_10MS;
		}
				
		// ONCE THAT WORKS, Comment out the above and use a software timer
		//	to "schedule" the RED LED toggle.
/*
		if (G_release_red) {
			LED_TOGGLE(RED);
			G_red_toggles++;
			G_release_red = 0; 
		}
*/

		// Whenever you are ready, add in the menu task.
		// Think of this as an external interrupt "releasing" the task.
/*
		serial_check();
		check_for_new_bytes_received();
*/
					
	} //end while loop
} //end main

#endif // #if CHOICE == KEVIN
