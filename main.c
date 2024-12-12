#include "fsl_device_registers.h"

volatile unsigned int nr_overflows = 0;
unsigned int t1, t2, period, pulse_width;
unsigned int t1_prev, t2_prev;
unsigned int duty_cycle = 0;
unsigned int number = 0;

void setup_pins();
void setup_timer();
void FTM3_IRQHandler(void);
void delayby1ms(int k);
void display_number(uint8_t number);

void setup_pins() {
	// Clock gating and pin configuration
    SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTB_MASK; // Enable clock to Port B, C and D
    SIM_SCGC3 |= SIM_SCGC3_FTM3_MASK; // FTM3 clock enable
    SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK;

    PORTC_PCR10 = 0x300; // Port C Pin 10 as FTM3_CH6 (ALT3)
    PORTC_GPCLR = 0x01BF0100; // Configure Port C pin [0:5],[7,8] as GPIO
    GPIOC_PDDR = 0x000001BF;

    PORTD_GPCLR = 0x003F0100;// Configure Port D pin 0 to 4 as GPIO
    GPIOD_PDDR = 0x0000003F; //Set Port D [0:5] as output

	PORTB_PCR3 = 0x100; // Configure Port B pin 3 as GPIO
	GPIOB_PDDR |= (0 << 3); // Set Port B pin 3 as input

    // FTM3 configuration
    FTM3_MODE = 0x5; // Enable FTM3
    FTM3_MOD = 0xFFFF;
    FTM3_SC = 0x0E; // System clock / 64

    // Enable FTM3 interrupts
    NVIC_EnableIRQ(FTM3_IRQn);

    FTM3_SC |= 0x40; // Enable TOF

}

void FTM3_IRQHandler(void) {
    nr_overflows++;
    uint32_t SC_VAL = FTM3_SC;
    FTM3_SC &= 0x7F; // clear TOF
}

void setup_timer() {
    SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK;
    SIM_SCGC3 |= SIM_SCGC3_FTM3_MASK;
    FTM3_MODE = 0x5;
    FTM3_MOD = 0xFFFF;
    FTM3_SC = 0x0E;
}

void delayby1ms(int k) {
    FTM3_C6SC = 0x1C;
    FTM3_C6V = FTM3_CNT + 333;
    for (int i = 0; i < k; i++) {
        while(!(FTM3_C6SC & 0x80));
        FTM3_C6SC &= ~(1 << 7);
        FTM3_C6V = FTM3_CNT + 333;
    }
    FTM3_C6SC = 0;
}

void display_number(uint8_t number) {
    // 7-segment display mapping for digits 0 to 9
    static const uint8_t segment_map[] = {0x3F, 0x06, 0x9B, 0x8F, 0xA6, 0xAD, 0xBD, 0x07, 0xFF, 0xAF};
    GPIOC_PDOR = segment_map[number];
}

void step_motor(int duty_cycle) {
	int delay = 100;
	uint32_t portB2 = GPIOB_PDIR;
	if((portB2 & (1 << 3)) == 0){
		GPIOD_PDOR = 0x00;
		number = 0;
		display_number(number);
	}
	else{
		if(duty_cycle >= 50){
			delay = 1;
			number = 3;

			GPIOD_PDOR = 0x36;
			delayby1ms(delay);
			GPIOD_PDOR = 0x35;
			delayby1ms(delay);
			GPIOD_PDOR = 0x39;
			delayby1ms(delay);
			GPIOD_PDOR = 0x3A;
			delayby1ms(delay);
			display_number(number);
		}
		else if (duty_cycle >= 25){
			delay = 5;
			number = 2;

			GPIOD_PDOR = 0x36;
			delayby1ms(delay);
			GPIOD_PDOR = 0x35;
			delayby1ms(delay);
			GPIOD_PDOR = 0x39;
			delayby1ms(delay);
			GPIOD_PDOR = 0x3A;
			delayby1ms(delay);
			display_number(number);
		}
		else if (duty_cycle >= 10){
			delay = 20;
			number = 1;

			GPIOD_PDOR = 0x36;
			delayby1ms(delay);
			GPIOD_PDOR = 0x35;
			delayby1ms(delay);
			GPIOD_PDOR = 0x39;
			delayby1ms(delay);
			GPIOD_PDOR = 0x3A;
			delayby1ms(delay);
			display_number(number);
		}
		else {
			number = 0;
			display_number(number);
			GPIOD_PDOR = 0x00;
		}
	}
}

int main(void) {
	setup_pins();
	setup_timer();

	while (1){
		FTM3_CNT = 0;
	    pulse_width = 0;
	    nr_overflows = 0; // initialize counters

	    FTM3_C6SC = 0x4; // rising edge
	    while(!(FTM3_C6SC & 0x80)); // wait for CHF
	    FTM3_C6SC &= ~(1 << 7);
	    t1_prev = FTM3_C6V; // first edge

	    FTM3_C6SC = 0x8; // falling edge
	    while(!(FTM3_C6SC & 0x80)); // wait for CHF
	    FTM3_C6SC = 0; // stop C6
	    t2_prev = FTM3_C6V; // second edge

	    FTM3_C6SC = 0x4; // rising edge
	    while(!(FTM3_C6SC & 0x80)); // wait for CHF
	    FTM3_C6SC &= ~(1 << 7);
	    t1 = FTM3_C6V; // first edge

	    FTM3_C6SC = 0x8; // falling edge
	    while(!(FTM3_C6SC & 0x80)); // wait for CHF
	    FTM3_C6SC = 0; // stop C6
	    t2 = FTM3_C6V; // second edge

	    period = t2-t2_prev;

	    if (t2 >= t1){
	    	pulse_width = (nr_overflows << 16) + (t2 - t1);
	    }
	    else{
	    	pulse_width = ((nr_overflows-1) << 16) + (t2 - t1);
	    }
	    pulse_width=pulse_width*100;
	    duty_cycle = pulse_width/period;
		//duty_cycle = 50;
	    step_motor(duty_cycle);
	}
}


