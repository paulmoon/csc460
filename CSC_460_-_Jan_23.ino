#include <scheduler.h>


// constants won't change. They're used here to 
// set pin numbers:
const int button_pin = 2;
const int servo_x_pin = 0;
const int servo_out_pin = 9;
const int IR_pin = 13;

volatile unsigned bit_seq = 0; // Byte we're trying to transmit
volatile int bit_index = 0; // Index of byte b we're comparing against.

void setupTimer3(){
  //Clear timer config.
  TCCR3A = 0;
  TCCR3B = 0;  
  //Set to CTC (mode 4)
  TCCR3B |= (1<<WGM32);
  
  //Set prescaller to 1
  TCCR3B |= (1<<CS30);
  
  //Set TOP value (500 us seconds)
   OCR3A = 8000;
}

void setupPWMTimer(){
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
  
  OCR1A = 420;  //38KHz
  OCR1C = 210;  //50% duty cycle
  TCCR1A &= ~(1<<COM1C1); // have the IR transmitter off at first
}

void check_button(){
  // read the state of the switch into a local variable:
  int val = analogRead(button_pin);
  if( val < 10 ){
     transmit('D'); 
  }
}

void handle_servo(){
  int val = analogRead(servo_x_pin);            // reads the value of the potentiometer (value between 0 and 1023) 
  val = map(val, 0, 1023, 500, 2500);     // scale it to use it with the servo (value between 0 and 180) 
     
 //http://www.robotshop.com/blog/en/arduino-5-minute-tutorials-lesson-5-servo-motors-3636
  digitalWrite(servo_out_pin,HIGH);
  delayMicroseconds(val);
  digitalWrite(servo_out_pin,LOW);   
}

void transmit(byte _b) {
  // debug
  digitalWrite(10, HIGH);
    
  // set global values for the interrupt handler    
  bit_seq = _b;
  
  // set the byte sequence to send 
  // the 1,0 header bits
  bit_seq <<= 2;
  bit_seq += 1;
  
  // start from lower order bit first
  bit_index = 0;   
  
  cli();
 
  //Enable interupt A for timer 3.
  TIMSK3 |= (1<<OCIE3A);
  // Clear interrupt; 
  TIFR3 |= (1<<OCF3A);
  //Set timer to 0 (optional here).
  TCNT3 = 0;
  
  sei();
}

// interrupt handler for the IR transmitter
ISR(TIMER3_COMPA_vect)
{          
  if( bit_index < 10 ){
    // we still have bits to send
    
    if( (bit_seq & ( 1 << bit_index )) != 0){
        // bit is set to one
        // turn on the IRT
        TCCR1A |= (1<<COM1C1);
    }else {
        // bit is set to zero
        // turn off the IRT
        TCCR1A &= ~(1<<COM1C1);          
    }
    
    // increment the bit index    
    bit_index++;    
  }else{
    // all of the bytes have been sent.
    // turn off the IRT
    TCCR1A &= ~(1<<COM1C1);
        
    // disable this interrupt
    TIMSK3 ^= (1<<OCIE3A);
    TCNT3 = 0;
    
    // debug
    digitalWrite(10, LOW);    
  }
}

void setup() {
  pinMode(button_pin, INPUT);
  pinMode(servo_x_pin, INPUT);
  pinMode(servo_out_pin, OUTPUT);
  pinMode(IR_pin, OUTPUT);
  Serial.begin(9600);
  
  setupTimer3();    
  setupPWMTimer();  
  
  Scheduler_Init();
  Scheduler_StartTask(0,20,check_button);
  Scheduler_StartTask(5,20,handle_servo);
  
  // debug pins
  pinMode(10, OUTPUT);  

}
void idle(uint32_t idle_period){
  delay(idle_period); 
}

void loop() {
  uint32_t idle_period = Scheduler_Dispatch();
  if( idle_period){
     idle(idle_period); 
  }
}
