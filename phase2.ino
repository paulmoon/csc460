#include <cops_and_robbers.h>
#include <nRF24L01.h>
#include <packet.h>
#include <radio.h>
#include <sensor_struct.h>
#include <spi.h>

#include <scheduler.h>

// constants won't change. They're used here to 
// set pin numbers:
const int joystick_x_pin = 0;
const int joystick_y_pin = 1;
const int button_pin = 2;

volatile uint8_t rxflag = 0; 
// packets are transmitted to this address
uint8_t station_addr[5] = { 0xAB, 0xAB, 0xAB, 0xAB, 0xAB };
// this is this radio's address
uint8_t my_addr[5] = { 0x18,0x2d,0x44,0x54,0xfb }; 
radiopacket_t packet;


void check_button(){
  // read the state of the switch into a local variable:
  int val = analogRead(button_pin);
  if( val < 50 ){
   // send the data   
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
          rxflag = 0;
       }
       
       if (packet.type == MESSAGE)
       {
         
       }
    }  
}

void handle_servo(){
   int val_x = analogRead(joystick_x_pin);            // reads the value of the potentiometer (value between 0 and 1023) 
   int val_y = analogRead(joystick_y_pin);            // reads the value of the potentiometer (value between 0 and 1023) 
   Serial.println(val_x);
   Serial.println(val_y);
}
  
void setup() {
  pinMode(button_pin, INPUT);
  pinMode(joystick_x_pin, INPUT);
  pinMode(joystick_y_pin, INPUT);
  
  pinMode(10, OUTPUT);    
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
   for(int i = 0;i < 5; ++i){
    packet.payload.message.address[i] = my_addr[i];
   }

   for (int i = 0; i < 24; ++i) {
     packet.payload.message.messagecontent[i] = 'A' + i;
   }
   Radio_Set_Tx_Addr(station_addr);
   
  // Thanks Neil!  
  Scheduler_Init();
  Scheduler_StartTask(0,20,check_button);
  Scheduler_StartTask(5,20,recv_packet);
  Scheduler_StartTask(8,20,handle_servo);
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
