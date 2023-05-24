#include <avr/io.h>
#include <avr/interrupt.h>

volatile double overflow = 0;
volatile double multiplier = 0;
volatile double deltaCount = 0;
volatile double totalCount = 0;
volatile double elapsed = 0;
volatile double speed = 0;

void setup()
{
  cli();    
  
  Serial.begin(9600);
  
  DDRB |= (1 << DDB1);   //pinMode(9, OUTPUT);
  DDRC |= (1 << DDC3);   //pinMode(A3, OUTPUT);
  DDRC |= (1 << DDC4);   //pinMode(A4, OUTPUT);
  DDRD &= !(1 << DDD2);  //pinMode(2, INPUT);
  DDRD &= !(1 << DDD3);  //pinMode(3, INPUT);

  
  //Initialise Timer 1 - Used for producing PWM. Not enabled here 
  TCCR1A = 0x00; //Operate in normal mode
  TCCR1B = 0x00; //Operate in normal mode
  TCNT1 = 0x0000;
  TCCR1B |= (1 << CS12) | (0 << CS11)| (1 << CS10);	//1024 Prescalar
  OCR1A = 0x0000; //Compare match not set
  OCR1B = 15624; //Compare match B every second 


  //Initialise Timer 2 - Used to find delta t
  TCCR2A = 0x00; //Operate in normal mode
  TCCR2B = 0x00; //Operate in normal mode
  TCNT2 = 0x0000;
  TCCR2B |= (1 << CS02) | (1 << CS01)| (1 << CS00);	//1024 Prescalar
  TIMSK2 = (1<<TOIE2);  //Timer 2 overflow interrupt enabled

  //External interrupts initialised
  EIMSK |= (1 << INT0);	//External Interrupt on PortD2
  EICRA |= (1 << ISC01) | (1 << ISC00);	//Sets INT0/D2 to rising edge
  EIMSK |= (1 << INT1);	//External Interrupt on PortD3
  EICRA |= (1 << ISC11) | (1 << ISC10);	//Sets INT1/D3 to rising edge
   
  sei();
}


void loop()
{
  asm volatile("nop");
}


//Triggered when LB1 is breached. Disables interrupts on Timer1
//and resets timer 2 count.
ISR(INT0_vect)
{
  //Reset counters
  TCNT2 = 0;
  overflow = 0;   
  
  PORTC &= !(1 << PC4);   //digitalWrite(A4,LOW);
  PORTC |= (1 << PC3);    //digitalWrite(A3,HIGH);
  PORTB &= !(1 << PB1);   //digitalWrite(9,LOW);


  TIMSK1 &= ~(1 << OCIE1A);   //Disable Timer1 MatchA Interrupt
  TIMSK1 &= ~(1 << OCIE1B);   //Disable Timer1 MatchB Interrupt
}


//This is triggered when LB2 is breached. It calls the supplementary
//function calcualteSpeed(), defines output compare A for timer1
//and enables timer 1 interrupts.
ISR(INT1_vect)
{
  deltaCount = TCNT2;
  multiplier = overflow;
  
  calculateSpeed();
  PORTC |= (1 << PC4);//digitalWrite(A4,HIGH);
  
  TCNT1 = 0;  //Reset counter
  OCR1A = 15624*speed/100; //Set timer1 matchA - percent of a second
  TIMSK1 |= (1 << OCIE1B);   //Enable Compare Match Interrupt
  TIMSK1 |= (1 << OCIE1A);   //Enable Compare Match Interrupt
}


//Counts overflows of timer2
ISR(TIMER2_OVF_vect)
{
  overflow += 1;  
}


//Pin 9 goes low after its duty cycle is complete. 
ISR(TIMER1_COMPA_vect)
{
  PORTB &= !(1 << PB1); //digitalWrite(9,LOW)
}


//Resets count1 and sends pin 9 high. 
ISR(TIMER1_COMPB_vect)
{
  TCNT1 = 0;
  PORTB |= (1 << PB1);  //digitalWrite(9,HIGH)
} 


//Calculates speed based on time elapsed between INT0 and INT1
void calculateSpeed()
{
  totalCount = multiplier*256  + deltaCount;
  elapsed = totalCount * 0.000064; //8-bit@1024 prescaler ==> 1024/16MHz = 0.000064 seconds/count 
  speed = 0.02/(elapsed/3600); // m/s ==> km/h
  
  //Print to serial monitor time and speed info
  Serial.print("Elapsed time: ");
  Serial.print(elapsed);
  Serial.println(" seconds .");
  Serial.print("Speed: ");
  Serial.print(speed);
  Serial.println(" km/h");
  
  //Any speed > 100 has a duty cycle of 100
  if (speed >= 100) {speed = 100;}  
}  