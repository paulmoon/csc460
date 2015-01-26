void setup() {
  pinMode(13, OUTPUT);
  
  //Clear timer config.
  TCCR3A = 0;
  TCCR3B = 0;  
  //Set to CTC (mode 4)
  TCCR3B |= (1<<WGM32);
  
  //Set prescaler to 256
  TCCR3B |= (1<<CS32);
  
  //Set TOP value (0.05 seconds)
  OCR3A = 3125;
  
  //Enable interupt A for timer 3.
  TIMSK3 |= (1<<OCIE3A);
  
  //Set timer to 0 (optional here).
  TCNT3 = 0;
  
  //=======================
  
  //PWM  
  //Clear timer config
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 &= ~(1<<OCIE1C);
  //Set to Fast PWM (mode 15)
  TCCR1A |= (1<<WGM10) | (1<<WGM11);
  TCCR1B |= (1<<WGM12) | (1<<WGM13);
  	
  //Enable output C.
  TCCR1A |= (1<<COM1C1);
  //No prescaler
  TCCR1B |= (1<<CS10);
  
  OCR1A = 800;  //50 us period
  OCR1C = 0;  //Target
  
}

ISR(TIMER3_COMPA_vect)
{
  //Slowly incrase the duty cycle of the LED.
  // Could change OCR1A to increase/decrease the frequency also.
  OCR1C += 10;
  if(OCR1C >= 800) {
    OCR1C = 0;
  }
}

void loop() {
  
}
