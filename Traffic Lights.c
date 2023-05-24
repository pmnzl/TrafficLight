#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t cycle = 0;
volatile bool config = 0;
volatile uint16_t period = 1;
volatile uint8_t flash = 0;

void setup()
{
  cli();	//Disable All Interrupts
  
  DDRB |= (1 << DDB0);	//Output Pin 8 (Green)
  DDRB |= (1 << DDB1);	//Output Pin 9 (Orange)
  DDRB |= (1 << DDB2);	//Output Pin 10 (Red)
  
  DDRC |= (0 << DDC0);	//Input Pin A0 (Pot)
  DDRC |= (1 << DDC1);	//Output Pin A1 (Blue)
  
  //Initialise ADC
  ADMUX = 0x00; 
  ADMUX |= (1 << REFS0);	//Voltage Reference AVcc w/ external cap @ AREF
  ADCSRA |= (1 << ADEN);	//Enable ADC
  
  //Initialise Timer 1
  TCCR1A = 0x00;
  TCCR1B = 0x00;
  TCNT1 = 0x0000;
  
  OCR1A = 15625;						//Compare Match Register
  TCCR1B |= (1 << WGM12);				//CTC Mode
  TCCR1B |= (1 << CS12) | (1 << CS10);	//1024 Prescalar
  TIMSK1 |= (1 << OCIE1A);				//Enable Timer Compare Interrupt
 
  EIMSK |= (1 << INT0);					//External Interrupt on PortD2
  EICRA |= (1 << ISC01) | (1 << ISC00);	//Sets INT0 to rising edge
  
  sei();	//Enable All Interrupts
}


void loop()
{
  asm volatile("nop");
}


ISR(TIMER1_COMPA_vect)
{
  switch (cycle)
  {
    case 3 :	//Reset Cycle
    	cycle = 0;
    	if (config == 1){config_mode();}
    case 0 :	//Red
    	PORTB = (1 << PORTB2);
    	break;
    case 1 :	//Green
    	PORTB = (1 << PORTB0);
        break;
    case 2 :	//Orange
    	PORTB = (1 << PORTB1);
    	break;
    case 4 :	//Blue
    	PORTC = (1 << PORTC1);
        break;
    case 5 :	//Blue Off/Delay
    	PORTC = (0 << PORTC1);
    	cycle = 3;
    	flash += 1;
    	break;
  }
  cycle += 1;
}


ISR(INT0_vect)
{
  config ^= 1;	//Toggle on rising edge
}


void config_mode()
{	
  sei();	//Enable interrupts
  while (config == 1)
  {
    PORTB = (1 << PORTB2);						//Only Red LED on
    period = 1 + floor((read_adc())/256.0);		//Convert ADC Signal to Period
    
    //Setup Timer for 1 Second Pluse
    TCNT1 = 0x0000;
    cycle = 4;
    OCR1A = floor(15625/(period*2));
   
    //Pulse
    while(flash < period){}
    flash = 0;
    
    //Setup Timer for 2 Second Delay
    TCNT1 = 0x0000;
    cycle = 5;
    OCR1A = 15625*2;
    
    //2 Second Delay
    while(flash < 1){}
    flash = 0;
  }
  
  OCR1A = 15625*period;		//Change compare to correct period
  PORTC = (0 << PORTC1);	//Blue LED off
  TCNT1 = 0x0000;			//Reset timer
  cycle = 0;				//Reset cycle
}


uint16_t read_adc()
{
  ADCSRA |= (1 << ADSC);		//Start ADC
  while(ADCSRA & (1 << ADSC));	//Wait for ADC Completion
  return ADC;
}