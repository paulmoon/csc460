#include <cops_and_robbers.h>
#include <nRF24L01.h>
#include <packet.h>
#include <radio.h>
#include <sensor_struct.h>
#include <spi.h>

#include <scheduler.h>

#define HIGH_BYTE(x) x>>8 
#define LOW_BYTE(x) x&0xFF 

// constants won't change. They're used here to 
// set pin numbers:
const int joystick_x_pin = 0;
const int joystick_y_pin = 1;
const int button_pin = 2;

volatile uint8_t rxflag = 0; 
uint8_t txflag = 0;

// packets are transmitted to this address roomba
uint8_t roomba_num = 0;
//uint8_t station_addr[5] = { 0xAB, 0xAB, 0xAB, 0xAB, 0xAB };

// this is our radio's address
uint8_t my_addr[5] = { 0x18,0x2d,0x44,0x54,0xfb }; 

// struct to hold packets
radiopacket_t recvPacket;
radiopacket_t sendPacket;

int x_value;
int y_value;

// util function to set the sender addresss
void setSenderAddress(uint8_t* dest, uint8_t src);

// setup a move roomba command packet
// speed = -500 --> 500 mm/sec
// deg = -2000 --> 2000 mm/sec
void moveCommand(int spd,int deg);

// setup a roomba ir_command packet
// sending the specified uint8_t byte
void fireGun(uint8_t b);

// read in input, setup a firePacket
void check_button(){
  // read the state of the switch into a local variable:
  int val = analogRead(button_pin);
  if( val < 50 ){
    fireGun(50);
  }
}

// read in input. setup a move packet
void handle_servo(){ 
   x_value = analogRead(joystick_x_pin);            // reads the value of the potentiometer (value between 0 and 1023) 
   y_value = analogRead(joystick_y_pin);            // reads the value of the potentiometer (value between 0 and 1023) 

   // map the x and y values to the correct ranges          
   Serial.println(x_value);
   x_value = map(x_value,0,1023,0,10);
   if( x_value >= 3 && x_value <= 5){
      x_value = 5; 
   }
   
   y_value = map(y_value,0,1023,0,10);
   if( y_value >= 3 && y_value <= 5){
      y_value = 5; 
   }   

   int16_t x;
   int16_t y = 0;
   if( x_value < 5 ){
     x = -200;
   }else if( x == 5){
     x = 0;
   }else{
     x = 200;
   }
   
   Serial.println(x);   
//   moveCommand(x,y);   
}


// recv_packet code
void recv_packet(){
  if (rxflag)
  {
        // remember always to read the packet out of the radio, even
       // if you don't use the data.
       if (Radio_Receive(&recvPacket) != RADIO_RX_MORE_PACKETS)
       {
          // if there are no more packets on the radio, clear the receive flag;
          // otherwise, we want to handle the next packet on the next loop iteration.
          rxflag = 0;
       }
       
       if( recvPacket.type == SENSOR_DATA){
          Serial.println("SENSOR_DATA");         
       }else if(recvPacket.type == IR_DATA){
          Serial.println("IR_DATA:");
          Serial.println(recvPacket.payload.ir_data.roomba_number);
          Serial.println(recvPacket.payload.ir_data.ir_data);
       }else{
          Serial.println("Unknown recv packet"); 
       }    
    }  
}

// send packet code
void send_packet(){
  if( txflag == 0){return;}
  Serial.println("sending packet");
  Radio_Transmit(&sendPacket, RADIO_WAIT_FOR_TX);
  txflag = 0;
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
   
   Radio_Set_Tx_Addr(ROOMBA_ADDRESSES[roomba_num]);
    
  // Thanks Neil!  
  Scheduler_Init();
  Scheduler_StartTask(0,20,check_button);
  Scheduler_StartTask(8,20,handle_servo);
  Scheduler_StartTask(5,20,recv_packet);
  Scheduler_StartTask(1,200,send_packet);
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

void setSenderAddress(uint8_t* dest, uint8_t *src,int len){
  for(int i = 0;i < len ;++i){
    dest[i] = src[i];
  }
}

void fireGun(uint8_t b){
  if( txflag == 1){return;}
  sendPacket.type = IR_COMMAND;
  
  pf_ir_command_t* cmd = &(sendPacket.payload.ir_command);
  setSenderAddress(cmd->sender_address,my_addr,5);  
  cmd->ir_command = SEND_BYTE;
  cmd->ir_data = 0;
  cmd->servo_angle = 0;
  txflag = 1;
}

void moveCommand(int16_t spd, int16_t deg){
  if(txflag == 1){return;}
  sendPacket.type = COMMAND;
  
  pf_command_t* cmd = &(sendPacket.payload.command);
  setSenderAddress(cmd->sender_address,my_addr,5);
  
  cmd->command = 137; // DRIVE op code
  cmd->num_arg_bytes = 4;
  
  // set the  speed
  cmd->arguments[0] = HIGH_BYTE(spd);
  cmd->arguments[1] = LOW_BYTE(spd);
  
  // set the amount of rotation
  cmd->arguments[2] = HIGH_BYTE(deg);
  cmd->arguments[3] = LOW_BYTE(deg);
  txflag = 1;
}
