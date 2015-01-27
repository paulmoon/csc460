#include <SoftwareSerial.h>


#include <nRF24L01.h>
#include <packet.h>
#include <radio.h>
#include <spi.h>
#include <scheduler.h>

// constants won't change. They're used here to 
// set pin numbers:
const int button_pin = 2;
const int servo_x_pin = 0;
const int servo_out_pin = 9;
const int IR_pin = 13;

volatile uint8_t rxflag = 0; 
// packets are transmitted to this address
uint8_t station_addr[5] = { 0xAB, 0xAB, 0xAB, 0xAB, 0xAB };
// this is this radio's address
uint8_t my_addr[5] = { 0x18,0x2d,0x44,0x54,0xfb }; 
//uint8_t my_addr[5] = {'Z','Z','Z','Z','Z' }; 
radiopacket_t packet;


void check_button(){
  // read the state of the switch into a local variable:
  int val = analogRead(button_pin);
//  Serial.println(val);
  if( val < 50 ){
   // send the data
//   Serial.println((char *)packet.payload.message.messagecontent);
//   Serial.println((char *)packet.payload.message.address);

   for(int i = 0; i < 24; ++i){
    Serial.print(packet.payload.message.messagecontent[i]);
   }
   
   
   Radio_Transmit(&packet, RADIO_WAIT_FOR_TX);
  }
}

void recv_packet(){
  if (rxflag)
  {
        // remember always to read the packet out of the radio, even
       // if you don't use the data.
       if (Radio_Receive(&packet) != RADIO_RX_MORE_PACKETS)
       {
          // if there are no more packets on the radio, clear the receive flag;
          // otherwise, we want to handle the next packet on the next loop iteration.
          digitalWrite(4, HIGH);
          rxflag = 0;
       }
       
       if (packet.type == MESSAGE)
       {
         digitalWrite(13, HIGH);
       }
    }  
}

//void handle_servo(){
//  int val = analogRead(servo_x_pin);            // reads the value of the potentiometer (value between 0 and 1023) 
//  val = map(val, 0, 1023, 500, 2500);     // scale it to use it with the servo (value between 0 and 180) 
//     
//  //http://www.robotshop.com/blog/en/arduino-5-minute-tutorials-lesson-5-servo-motors-3636
//  // Thanks Geoff and Adam!
//  digitalWrite(servo_out_pin,HIGH);
//  delayMicroseconds(val);
//  digitalWrite(servo_out_pin,LOW);   
//}
  
void setup() {
  pinMode(button_pin, INPUT);
//  pinMode(servo_x_pin, INPUT);
//  pinMode(servo_out_pin, OUTPUT);
//  pinMode(IR_pin, OUTPUT);

//   pinMode(13, OUTPUT);
   pinMode(4, OUTPUT);
   pinMode(10, OUTPUT); 
  
   digitalWrite(4, LOW);
   digitalWrite(10, LOW);
   
   delay(100);
   digitalWrite(10, HIGH);
   delay(100);
   
   Serial.begin(9600);
   
   Radio_Init();
 
   // configure the receive settings for radio pipe 0
   Radio_Configure_Rx(RADIO_PIPE_0, my_addr, ENABLE);
   // configure radio transceiver settings.
   Radio_Configure(RADIO_2MBPS, RADIO_HIGHEST_POWER);
 
   // put some data into the packet
   packet.type = MESSAGE;
//   memcpy(packet.payload.message.address, my_addr, RADIO_ADDRESS_LENGTH);
//   Serial.println((char *)packet.payload.message.address);
//   Serial.println((char *) my_addr);   
   for(int i = 0;i < 5; ++i){
    packet.payload.message.address[i] = my_addr[i];
   }

//   packet.payload.message.messageid = 55;
//   Serial.println((char *)packet.payload.message.messagecontent);
//   memset(packet.payload.message.messagecontent, 0, sizeof(packet.payload.message.messagecontent));
//   Serial.println((char *)packet.payload.message.messagecontent);
//   char *msg = "Test message.";
//   memcpy(packet.payload.message.messagecontent,msg,strlen(msg));
   for (int i = 0; i < 24; ++i) {
     packet.payload.message.messagecontent[i] = 'A' + i;
   }
//   snprintf((char*)packet.payload.message.messagecontent, sizeof(packet.payload.message.messagecontent), "Test message.");
//   Serial.println((char *)packet.payload.message.messagecontent);
   // The address to which the next transmission is to be sent
   Radio_Set_Tx_Addr(station_addr);
   
  // Thanks Neil!  
  Scheduler_Init();
  Scheduler_StartTask(0,20,check_button);
  Scheduler_StartTask(5,20,recv_packet);
//  Scheduler_StartTask(5,20,handle_servo);
}

void idle(uint32_t idle_period){
  delay(idle_period); 
}

void radio_rxhandler(uint8_t pipe_number)
{
   rxflag = 1;
}

void loop() {
  uint32_t idle_period = Scheduler_Dispatch();
  if( idle_period){
     idle(idle_period); 
  }
}
