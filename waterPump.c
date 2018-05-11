#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define STR_LEN 50

unsigned char LOOKUPTB[] = { 
   0b00111111, 0b00000110, 0b01011011, 0b01001111, 
   0b01100110, 0b01101101, 0b01111101, 0b00000111, 
   0b01111111, 0b01101111, 0b01110111, 0b01111100, 
   0b00111001, 0b01011110, 0b01111001, 0b01110001, 0b01010010};
   
unsigned char water=0;
unsigned short count=0,countM=0,countU=0;
unsigned short control_value=0;   
unsigned char on = 0;
unsigned char display_state = 0;
unsigned char open,speed;

unsigned char text[STR_LEN];
unsigned char input;
unsigned char speed_input;
unsigned char use_water;
   
unsigned char toggleMotor = 0;   
unsigned char fixedSegment = 0;

   
void USART_T(unsigned char data)
{
   while(!(UCSR0A&(1<<UDRE0)));
      UDR0 = data;
}

unsigned USART_R()
{
   while(!(UCSR0A&(1<<RXC0)));
      return UDR0;
}

void println(unsigned char data[])
{
   unsigned char i;
   for(i=0;*(data+i)!=0;i++)
      USART_T(*(data+i));
   USART_T(13);
   USART_T(10);
}

void printc(unsigned char data)
{
   USART_T(data);
   USART_T(13);
   USART_T(10);
}

void printv(unsigned short data)
{
   unsigned char t1,t2,t3,t4;
   t1 = data/1000;
   t2 = data%1000/100;
   t3 = data%100/10;
   t4 = data%10;
   
   USART_T(t1+'0');
   USART_T(t2+'0');
   USART_T(t3+'0');
   USART_T(t4+'0');
   USART_T(13);
   USART_T(10);
}

void display2digits(unsigned char d)
{
   unsigned char d1 = d/10;
   unsigned char d2 = d%10;
   
      PORTC = 0xff;
   
      PORTC = 0x02;
      PORTB = LOOKUPTB[d1];
      _delay_ms(25);
      
      PORTC = 0x01;
      PORTB = LOOKUPTB[d2];
      _delay_ms(25);
   
}

int main()
 { 
   DDRB = 0xff;
   DDRC = 0xff;
   DDRD = 0xff;
    
   PORTC = 0b00000000;
   PORTB = LOOKUPTB[0];
 
   ADMUX = 0x26;
    
   UCSR0A = 0x02;
   UCSR0B = 0x98;
   UCSR0C = 0x06;
   UBRR0H = 0;
   UBRR0L = 103;
   
   TCCR0A = 0x00;
   TCCR0B = 0x05;
   TIMSK0 = 0x01;
   TCNT0 = 248; //1ms
   sei();
 
   while(1);
   return 0;
 }
 
 ISR(USART_RX_vect)
 {
    input = UDR0;
    printc(input);
 }
 
 ISR(TIMER0_OVF_vect)
 {
    PORTD = PORTD ^ 0b10000000;
    /*--------------- Pump Water Part---------*/
    if(water<20) on = 1;
    else if(water>=99) on = 0;
    ADCSRA = 0b11000110;
    while(!(ADCSRA&(1<<ADIF)));
    unsigned char control_value = (ADCH*5/253); // 5 level : 1-> 1%/s, 2-> 2%/s  ...  5-< 5%/s
    if(control_value!=0&&on)
    {  
       count++;
       countM++;
       if(count>=(143/control_value))	// 1000/ 7 (time per round) /control_value
       {
	  if(water<99)
	  {
	    water++;
	    PORTD = PORTD ^ 0b00001000;
	  }
	  count=0;
       }
       if(countM>=(14/control_value)&&water<99)
       {
	  PORTD = PORTD ^ 0b00000100;
	  countM = 0;
       }
    }
    
    /*------------- UI Part ---------*/
    switch(display_state)
    {
       case 0:	
	  println(&"Do you want to use water ? (y/n)"); 
	  display_state++; 
	  break;
       case 1:
	  use_water = input;
          if(use_water=='y'||use_water=='Y')
	  {
	     println(&"Please enter speed (1-5)");
	     display_state++;
	  }
	  else if(input=='n')
	    display_state=3;
	  break;
      case 2:
	 speed_input = input;
	 if(speed_input>='1'&&speed_input<='5')
	 {
	    speed = speed_input-'0';
	    open = 1;
	    display_state=0;
	    
	 }
	 else if(input=='n')
	    display_state=3;
	 break;
      default:
         println(&"Closed water");
	 open = 0;
	 display_state = 0;
    }
    input = ' ';

    if(open&&water>0)
    {
       countU++;
       if(countU>=143/speed)
       {
	  water--;
	  countU=0;
       }
    }
    
   display2digits(water); //7 ms
   TCNT0 = 248;	   //1 ms
    
 }
 
