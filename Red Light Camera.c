#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint8_t cycle = 0;
volatile uint16_t count = 0;
volatile uint8_t timer0_overflow = 0;
volatile uint8_t timer2_overflow = 0;
volatile uint8_t update = 0;
volatile uint16_t duty = 0;
volatile bool state = 1;
volatile uint16_t pre_time = 0;
volatile uint16_t post_time=0;

void setup()
{
  Serial.begin(9600);
  
  cli();				//Disable All Interrupts
  
  DDRB |= (1 << DDB0);	//Output Pin 8 (Green)
  DDRB |= (1 << DDB1);	//Output Pin 9 (Orange)
  DDRB |= (1 << DDB2);	//Output Pin 10 (Red)
  DDRC |= (1 << DDC1);	//Output Pin A1 (Blue)
  DDRB |= (1 << DDB5);	//Output Pin 13 (Oscilloscope)
 
  //Initialise Timer 0, Normal Mode
  TCCR0A = 0x00;
  TCCR0B = 0x00;
  TCNT0 = 0x00;
  
  OCR0A = duty;							//Compare Match Register
  TCCR0B |= (1 << CS02) | (1 << CS00);	//1024 Prescalar
  TIMSK0 |= (1 << OCIE0A);				//Enable Timer Compare Interrupt 

  //Initialise Timer 1, Normal Mode
  TCCR1A = 0x00;
  TCCR1B = 0x00;
  TCNT1 = 0x0000;
  
  OCR1A = 15625;						//Compare Match Register
  TCCR1B |= (1 << CS12) | (1 << CS10);	//1024 Prescalar
  TIMSK1 |= (1 << OCIE1A);				//Enable Timer Compare Interrupt
 
  //Initialise Timer 2, Normal Mode
  TCCR2A = 0x00;
  TCCR2B = 0x00;
  TCNT2 = 0x00;
  
  OCR2A = 64;											//Compare Match Register
  TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);	//1024 Prescalar
  TIMSK2 |= (1 << OCIE2A);								//Enable Timer Compare Interrupt 

  //Enable External Interrupts on Pin 2
  EIMSK |= (1 << INT0);					//External Interrupt on PortD2
  EICRA |= (1 << ISC01) | (1 << ISC00);	//Sets INT0 to rising edge
  
  sei();								//Enable All Interrupts
}


void loop()
{
  asm volatile("nop");	//Do Nothing
}


//Timer 1, Controls Traffic Lights
ISR(TIMER1_COMPA_vect)
{
  TCNT1 = 0x0000;
  switch (cycle)
  {
    case 3 :	//Reset Cycle
    	cycle = 0;
    case 0 :	//Red On
    	PORTB &= ~(1 << PORTB0);
    	PORTB &= ~(1 << PORTB1);
    	PORTB |= (1 << PORTB2);
    	break;
    case 1 :	//Green On
    	PORTB |= (1 << PORTB0);
    	PORTB &= ~(1 << PORTB1);
    	PORTB &= ~(1 << PORTB2);
        break;
    case 2 :	//Orange On
    	PORTB &= ~(1 << PORTB0);
    	PORTB |= (1 << PORTB1);
    	PORTB &= ~(1 << PORTB2);
    	break;
  }
  cycle += 1;
}


//Timer 0, Controls PWM Signal on Pin 13
ISR(TIMER0_COMPA_vect)
{
  TCNT0 = 0x00;
  timer0_overflow += 1;

  if ((timer0_overflow >= 61)) //because 61 overflows@1024prescaler on timer0 == 1 second?
  {
    timer0_overflow = 0;

    //PWM Functionality
    if (duty == 0)	//Lower Limit Error Correction
    {
      PORTB &= ~(1 << PORTB5);	//LED Constantly Off
    }
    else if (duty >= 255)	//Upper Limit Error Correction
    {
      PORTB |= (1 << PORTB5);	//LED Constantly On
    }
    else if (state == 0)	//LED Off State ==> Tom: state == 0 means the LED is on right? 
    {
      //Turn LED off, change OCRA to inverse, reset timer...
      PORTB &= ~(1 << PORTB5);	//LED Off
      OCR0A = 255 - OCR0A;		//Change Duty Cycle
      state = 1;				//Reset LED State
    }
    else	//LED On State ==> Tom: Led OFF
    {
      //Turn LED on, change OCRA, reset timer...
      PORTB |= (1 << PORTB5);	//LED On
      OCR0A = duty;				//Change Duty Cycle
      state = 0;				//Reset LED State
    }
  }
 
}


//Timer 2, Controls the Flashing of the Blue LED
ISR(TIMER2_COMPA_vect)
{
  TCNT2 = 0x00;
  timer2_overflow += 1;

  //Check Overflow Counter & State of Blue LED
  if ((timer2_overflow >= 61) && ((update >= 1) && (update <= 4)))
  {
    timer2_overflow = 0;
    PORTC ^= (1 << PORTC1);
    update += 1;
  }
}


//External Interrupt on the Button, Triggering Blue LED & duty cycle change
ISR(INT0_vect)
{
  pre_time = TCNT1;
  if (cycle == 1)	//When light is red
  {
    update = 1;
    count += 1;
    if (count >= 100) {count = 100;}	//Avoid Overflow Error
    duty = floor(2.55 * (count));		//Convert to OCRA
    post_time = TCNT1;
    Serial.println(post_time-pre_time);	//Get time of execution
  }
}