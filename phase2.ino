#include <cops_and_robbers.h>
#include <nRF24L01.h>
#include <packet.h>
#include <radio.h>
#include <sensor_struct.h>
#include <spi.h>

#include <scheduler.h>

#define HIGH_BYTE(x) x>>8
#define LOW_BYTE(x) x & 0x00FF 

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
radiopacket_t recv_packet;
radiopacket_t send_packet;

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
    fireGun('B');
  }
}

int16_t old_x = -1;
int16_t old_y = -1;

int16_t mapSpeedValue(int16_t val){
 return map(val,0,4,-200,200); 
}

int16_t mapRotValue(int16_t val){
 return map(val,0,4,-200,200);
}

// read in input. setup a move packet
void handle_servo(){ 
   uint16_t x_value = map(analogRead(joystick_x_pin), 0, 1023, 0, 5);            // reads the value of the potentiometer (value between 0 and 1023) 
   uint16_t y_value = map(analogRead(joystick_y_pin), 0, 1023, 0, 5);            // reads the value of the potentiometer (value between 0 and 1023) 

//  if(old_x == x_value && old_y == y_value){
//    return;
//  }
//  old_x = x_value;
//  old_y = y_value;

  // map the x and y values to the correct ranges 
   if( x_value == 2 && y_value == 2)
   {
    // stop  
      moveCommand(0,(int16_t)0x8000);
    } else if( x_value == 2){
    // move straight
      y_value = -mapSpeedValue(y_value);
      moveCommand(y_value,(int16_t)0x8000);
    } else if( y_value == 2){
    // turn in place
      uint16_t deg = -1;
      if( x_value < 2 ){
        deg = 1;
      }
      moveCommand(200,deg);
    }else{
      x_value = -mapRotValue(x_value); 
      y_value = -mapSpeedValue(y_value);      
      moveCommand(y_value,x_value);
    }

  
}


// recv_packet code
void recv_packet_task(){
  if (rxflag)
  {
          // remember always to read the packet out of the radio, even
         // if you don't use the data.
     if (Radio_Receive(&recv_packet) != RADIO_RX_MORE_PACKETS)
     {
            // if there are no more packets on the radio, clear the receive flag;
            // otherwise, we want to handle the next packet on the next loop iteration.
      rxflag = 0;
    }
    
    if( recv_packet.type == SENSOR_DATA){
      Serial.println("SENSOR_DATA");         
    }else if(recv_packet.type == IR_DATA){
      Serial.println("IR_DATA:");
      Serial.println(recv_packet.payload.ir_data.roomba_number);
      Serial.println(recv_packet.payload.ir_data.ir_data);
    }else{
      Serial.println("Unknown recv packet"); 
    }    
  }
}

// send packet code
void send_packet_task(){
  if( txflag == 0){return;}
  Serial.println("sending packet");
  Radio_Transmit(&send_packet, RADIO_WAIT_FOR_TX);
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
  Scheduler_StartTask(5,20,recv_packet_task);
  Scheduler_StartTask(1,200,send_packet_task);
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
  if( txflag == 1){
    if (send_packet.type = IR_COMMAND){
      return;
    }
  }
  send_packet.type = IR_COMMAND;
  
  pf_ir_command_t* cmd = &(send_packet.payload.ir_command);
  setSenderAddress(cmd->sender_address,my_addr,5);  
  cmd->ir_command = SEND_BYTE;
  cmd->ir_data = 0;
  cmd->servo_angle = 0;
  txflag = 1;
}

void moveCommand(int16_t spd, int16_t deg){
  if(txflag == 1){return;}    
  
  // clear_the_packet(&send_packet);
  
  send_packet.type = COMMAND;
  
  pf_command_t* cmd = &(send_packet.payload.command);
  setSenderAddress(cmd->sender_address,my_addr,5);
  
  cmd->command = 137; // DRIVE op code
  cmd->num_arg_bytes = 4;
  
  // clear the arguments array to zero;
//  memset(cmd->arguments,0,sizeof(uint8_t)*16);
  
  // set the  speed
  cmd->arguments[0] = HIGH_BYTE(spd);
  cmd->arguments[1] = LOW_BYTE(spd);
  
  // set the amount of rotation
  cmd->arguments[2] = HIGH_BYTE(deg);
  cmd->arguments[3] = LOW_BYTE(deg);
  
//  Serial.println(cmd->arguments[0]);
//  Serial.println(cmd->arguments[1]);
//  Serial.println(cmd->arguments[2]);
//  Serial.println(cmd->arguments[3]);
  txflag = 1;

  // if (check_if_same_packet(&prev_packet,&send_packet)){
  //   save_the_packet(&prev_packet,send_packet);
  //   txflag = 1;
  // }
}