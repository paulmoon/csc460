//#include <Servo.h>

//Servo servo;

// constants won't change. They're used here to 
// set pin numbers:
const int button_pin = 2;
const int servo_x_pin = 0;
const int servo_out_pin = 9;
const int IR_pin = 13;


// Variables will change:
int ledState = HIGH;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

volatile int header = 1;
volatile byte b = 0; // Byte we're trying to transmit
volatile int i = 7; // Index of byte b we're comparing against.

void setupTimer3(){
    //Clear timer config.
  TCCR3A = 0;
  TCCR3B = 0;  
  //Set to CTC (mode 4)
  TCCR3B |= (1<<WGM32);
  
  //Set prescaller to 256
//  TCCR3B |= (1<<CS32);
  TCCR3B |= (1<<CS30);
  
  //Set TOP value (500 us seconds)
//  OCR3A = 31250;
   OCR3A = 8000;
  
//  //Enable interupt A for timer 3.
//  TIMSK  `3 |= (1<<OCIE3A);
//  //Set timer to 0 (optional here).
//  TCNT3 = 0;
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
  
  OCR1A = 410;  //38KHz
//  OCR1C = 140;  //Target
  OCR1C = 210;  //Target
  TCCR1A &= ~(1<<COM1C1); // have the IR transmitter off at first
}

void setup() {
  pinMode(button_pin, INPUT);
  pinMode(servo_x_pin, INPUT);
  pinMode(servo_out_pin, OUTPUT);
  pinMode(IR_pin, OUTPUT);
  Serial.begin(9600);
    
  // sets pin 9 to ouput
//  servo.attach(9);
  
  // debug pins
  pinMode(10, OUTPUT);
  
  setupTimer3();
  setupPWMTimer();
}


// interrupt handler for the IR transmitter
ISR(TIMER3_COMPA_vect)
{        
  if(header == 1){
    // turn on for 500us
     TCCR1A |= (1<<COM1C1);
   
     header = 2;
     return;
  }else if (header == 2){
    // turn off for 500us
    TCCR1A &= ~(1<<COM1C1);    
   
    header = 0;
    return;
  }
 
  
  if( i <= 7){
    if ((b & (1 << i)) != 0) {
      // turn on the IRT to transmit a 1
      TCCR1A |= (1<<COM1C1);
    } else {
      // turn off the IRT to transmit 0
      TCCR1A &= ~(1<<COM1C1);
    }    
  }
      
  i += 1;
  
  // All of byte was transmitted.
  if (i > 8) {
    // turn off the IRT
    TCCR1A &= ~(1<<COM1C1);
        
    // disable this interrupt
    TIMSK3 ^= (1<<OCIE3A);
    TCNT3 = 0;
    
    // debug
    digitalWrite(10, LOW);
  }
}

void transmit(byte _b) {
  // debug
  digitalWrite(10, HIGH);
    
  // set global values for the interrupt handler    
  b = _b;
  i = 0;
  header = 1;
  
  cli();
 
  //Enable interupt A for timer 3.
  TIMSK3 |= (1<<OCIE3A);
  // Clear interrupt; 
  TIFR3 |= (1<<OCF3A);
  //Set timer to 0 (optional here).
  TCNT3 = 0;
  
  sei();
}

void loop() {
  int val;
      
  // read the state of the switch into a local variable:
  val = analogRead(button_pin);
  int reading = LOW;
  if( val <= 10){
    reading = LOW;
  }else{
    reading = HIGH; 
  }

  // check to see if you just pressed the button 
  // (i.e. the input went from LOW to HIGH),  and you've waited 
  // long enough since the last press to ignore any noise:  

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  } 
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
          transmit(129);
//        ledState = !ledState;
      }
    }
  }
  
  // save the rea`ding.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
  
  
  // handle input from the servo
  // read the servo pin value
  val = analogRead(servo_x_pin);            // reads the value of the potentiometer (value between 0 and 1023) 
//  val = map(val, 0, 1023, 0, 180);     // scale it to use it with the servo (value between 0 and 180) 
  val = map(val, 0, 1024, 500, 2500);     // scale it to use it with the servo (value between 0 and 180) 
  
 //http://www.robotshop.com/blog/en/arduino-5-minute-tutorials-lesson-5-servo-motors-3636
  digitalWrite(servo_out_pin,HIGH);
  delayMicroseconds(val);
  digitalWrite(servo_out_pin,LOW);
   
//  servo.write(val);
  delay(15);                           // waits for the servo to get there 
}
