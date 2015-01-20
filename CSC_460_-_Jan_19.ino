//#include <Servo.h> 
 
//Servo myservo;  // create servo object to control a servo 
 
int potpin = 0;  // analog pin used to connect the potentiometer
int val;    // variable to read the value from the analog pin 
volatile byte b = 0; // Byte we're trying to transmit
volatile int i = 7; // Index of byte b we're comparing against.

void setup() 
{ 
  pinMode(3, OUTPUT);
//  myservo.attach(9);  // attaches the servo on pin 9 to ¨the servo object 
  Serial.begin(9600);
  
  pinMode(10, OUTPUT);

  
  //Clear timer config.
  TCCR3A = 0;
  TCCR3B = 0;  
  //Set to CTC (mode 4)
  TCCR3B |= (1<<WGM32);
  
  //Set prescaller to 256
  TCCR3B |= (1<<CS32);
  
  //Set TOP value (0.5 seconds)
  OCR3A = 31250;
  
//  //Enable interupt A for timer 3.
//  TIMSK3 |= (1<<OCIE3A);
//  
//  //Set timer to 0 (optional here).
//  TCNT3 = 0;

 
//  //PWM  
//  //Clear timer config
//  TCCR1A = 0;
//  TCCR1B = 0;
//  TIMSK1 &= ~(1<<OCIE1C);
//  //Set to Fast PWM (mode 15)
//  TCCR1A |= (1<<WGM10) | (1<<WGM11);
//  TCCR1B |= (1<<WGM12) | (1<<WGM13);
//  	
//  //Enable output C.
//  TCCR1A |= (1<<COM1C1);
//  //No prescaler
//  TCCR1B |= (1<<CS10);
//  
//  OCR1A = 800;  //50 us period
//  OCR1C = 0;  //Target
} 
 
ISR(TIMER3_COMPA_vect)
{  
  Serial.println(i);
  if ((b & (1 << i)) != 0) {
    Serial.println("High");
    digitalWrite(3, HIGH);
  } else {
        Serial.println("Low");
    digitalWrite(3, LOW);    
  }
  
  i -= 1;
  
  // All of byte was transmitted.
  if (i < 0) {
    Serial.println("DONE");
    digitalWrite(3, LOW);    
    TIMSK3 ^= (1<<OCIE3A);
    digitalWrite(10, LOW);
  }
}

void transmit(byte _b) {
  digitalWrite(10, HIGH);
  b = _b;
  i = 7;
  
  cli();
 
  //Enable interupt A for timer 3.
  TIMSK3 |= (1<<OCIE3A);
  // Clear interrupt; 
  TIFR3 |= (1<<OCF3A);
  //Set timer to 0 (optional here).
  TCNT3 = 0;
  
  sei();
}

void loop() 
{ 
  val = analogRead(potpin);            // reads the value of the potentiometer (value between 0 and 1023) 
  val = map(val, 0, 1023, 0, 180);     // scale it to use it with the servo (value between 0 and 180) 
//  Serial.println(val);
  
  int button = analogRead(A2);
    
  if (button < 10) {
    Serial.println("STARTING");
    transmit(0x0F);
  }
  
  int receiver = analogRead(A3);
//  Serial.println(receiver);
//  myservo.write(val);                  // sets the servo position according to the scaled value 
  delay(15);                           // waits for the servo to get there 
} 


