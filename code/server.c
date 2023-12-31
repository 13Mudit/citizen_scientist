
#include<string.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

#include<signal.h>

#include<zmq.h>
#include<pigpio.h>
/*
   gcc -Wall -pthread -o server.exe server.c -lpigpio -lrt -lzmq
*/

#define SOS_ARR_LEN 10000
#define SOS_TRIGGER_PIN 26

#define MAX_RLC_ARR_LEN 4000
#define RLC_TRIGGER_PIN 24
#define RC_TRIGGER_PIN 25

#define PHOTOGATE_ARR_LEN 100
#define PHOTOGATE_PIN 27

#define DIODE_BUFFER_ARR_LEN 16
#define DIODE_ARR_LEN (100 * DIODE_BUFFER_ARR_LEN)
#define DIODE_PWM_PIN 12


int handler;

void INThandler(int);
void get_data(char buf[3], char adc_channel, unsigned short *store, int arr_len);
void send_data(void* responder, unsigned short *store, int arr_len);


void sos(void *responder);
void rlc(void *responder);
void pwm(void *responder);
void diode(void *responder);
void temp(void *responder);
void filter(void *responder);
void ph(void *responder);
void photo(void *responder);


int main(){
  printf("Started\n");

  void *context = zmq_ctx_new();
  void *responder = zmq_socket(context, ZMQ_REP);

  char recv_buf[10];
   
  printf("Context created!\n");
  int speed = 800000;

  if(gpioInitialise() < 0) return 1;

  
  int rc = zmq_bind(responder, "tcp://*:5555");
  assert(rc == 0);
  printf("Server bound\n"); 


  handler = spiOpen(0, speed, 0);
  if(handler < 0) return 2;

  while(1){

    zmq_recv(responder, recv_buf, 10, 0);
    sleep(1);
    printf("Received: %s ", recv_buf);
    

    // printf(" %d ", strcmp(recv_buf, "PHOTO"));


    if(!strcmp(recv_buf, "SOS"))
      sos(responder);
    else if(!strcmp(recv_buf, "RLC"))
      rlc(responder);
    else if(!strcmp(recv_buf, "PWM"))
      pwm(responder);
    else if(!strcmp(recv_buf, "DIODE"))
      diode(responder);
    else if(!strcmp(recv_buf, "TEMP"))
      temp(responder);
    else if(!strcmp(recv_buf, "FILTER"))
      filter(responder);
    else if(!strcmp(recv_buf, "PH"))
      ph(responder);
    else if(!strcmp(recv_buf, "PHO"))
      photo(responder);

    sleep(1);
  }  
  
  spiClose(handler);

  gpioTerminate();

  return 0;
}


void sos(void *responder){

  char adc_channel = 0;
  char buf[3];
  char recv_buf[5];

  unsigned short store[SOS_ARR_LEN];
  int break_outer = 0;

  zmq_send(responder, "SOS", 3, 0);

  gpioSetMode(SOS_TRIGGER_PIN, PI_INPUT);

  while(1){
    while(1){
      
      zmq_recv(responder, recv_buf, 1, 0);
      if(*recv_buf - '0' == 0){
        zmq_send(responder, "SOS Exited", 10, 0);
        break_outer = 1;
        break;
      }

      if(gpioRead(SOS_TRIGGER_PIN) == PI_LOW)
        break;
    }

    get_data(buf, adc_channel, store, SOS_ARR_LEN);

    if(break_outer)
      break;

    send_data(responder, store, SOS_ARR_LEN);
  }
}


void rlc(void *responder){

  char adc_channel = 3;
  char buf[3];

  char recv_buf[5];

  unsigned int rlc_arr_len = 600;
  unsigned short store[MAX_RLC_ARR_LEN];

  zmq_send(responder, "RLC", 3, 0);

  gpioSetMode(RLC_TRIGGER_PIN, PI_OUTPUT);
  gpioSetMode(RC_TRIGGER_PIN, PI_OUTPUT);

  while(1){

    zmq_recv(responder, recv_buf, 1, 0);
    if(*recv_buf - '0' == 0){
      zmq_send(responder, "RLC Exited", 10, 0);
      break;
    }

    gpioWrite(RLC_TRIGGER_PIN, 1);
    gpioWrite(RC_TRIGGER_PIN, 1);
    get_data(buf, adc_channel, store, MAX_RLC_ARR_LEN/2);
    gpioWrite(RLC_TRIGGER_PIN, 0);
    gpioWrite(RC_TRIGGER_PIN, 0);
    get_data(buf, adc_channel, &store[MAX_RLC_ARR_LEN/2], MAX_RLC_ARR_LEN/2);

    send_data(responder, store, MAX_RLC_ARR_LEN);
  }
}

void pwm(void *responder){

}

void diode(void *responder){
  char voltage_adc_channel = 2;
  char current_adc_channel = 1;

  int freq = 2000000;
  int status;

  char buf[3];

  char recv_buf[5];

  unsigned short voltage_current_store[DIODE_ARR_LEN*2];

  zmq_send(responder, "DIODE", 5, 0);


  // gpioSetMode(DIODE_PWM_PIN, PI_OUTPUT);


  while(1){

    zmq_recv(responder, recv_buf, 1, 0);
    if(*recv_buf - '0' == 0){
      zmq_send(responder, "RLC Exited", 10, 0);
      break;
    }


    // gpioWrite(DIODE_PWM_PIN, 1);
    // while(1);

    for(int i=0; i<(DIODE_ARR_LEN/DIODE_BUFFER_ARR_LEN); i++){

      status = gpioHardwarePWM(DIODE_PWM_PIN, freq, i*10000);
      if(status){
        printf("Error in PWM %d\n", status);
        break;
      }

      gpioDelay(1000);

      get_data(buf, voltage_adc_channel, (voltage_current_store + i*DIODE_BUFFER_ARR_LEN), DIODE_BUFFER_ARR_LEN);
      get_data(buf, current_adc_channel, (voltage_current_store + DIODE_ARR_LEN + i*DIODE_BUFFER_ARR_LEN), DIODE_BUFFER_ARR_LEN);

    }
    
    status = gpioHardwarePWM(DIODE_PWM_PIN, 0, 0);
    if(status){
      printf("Error in PWM %d\n", status);
      break;
    }

    send_data(responder, voltage_current_store, DIODE_ARR_LEN*2);

    gpioDelay(500000);
  }


}

void temp(void *responder){
  
}

void filter(void *responder){
  
}

void ph(void *responder){
  
}

void photo(void *responder){
  unsigned short store[PHOTOGATE_ARR_LEN];
  char recv_buf[5];

  zmq_send(responder, "PHOTO", 5, 0);

  if(gpioSetMode(RLC_TRIGGER_PIN, PI_INPUT) != 0) return;

  while(1){

    zmq_recv(responder, recv_buf, 1, 0);
    if(*recv_buf - '0' == 0){
      zmq_send(responder, "PHOTOGATE Exited", 16, 0);
      break;
    }

    for(int i=0; i<PHOTOGATE_ARR_LEN; i++)
      store[i] = gpioRead(PHOTOGATE_PIN);

    send_data(responder, store, PHOTOGATE_ARR_LEN);
  }

}

void get_data(char buf[3], char adc_channel, unsigned short *store, int arr_len){
  for(int i=0; i<arr_len; i++){
      /*
      SPI read starts after 11000 is transmitted
      1 -> start bit
      1 -> single/differential signal
      0 -> X for 4 bit ADC (MSB channel select for 8 bit ADC)
      0 -> Channel select
      0 -> channel select
      
      spi transfer between pi and MCP3204
      24 bit char buffer used
      00000110 00000000 00000000
      ^^^^^      ^^
      align      NA
      last 12 bits are received as data
      */

      buf[0] = 6; //0b00000110
      buf[1] = adc_channel << 6; //0bCC000000; CC <- ADC channel select
      buf[2] = 0; //0b00000000

      spiXfer(handler, buf, buf, 3);
      store[i] = ((buf[1]&15)<<8) + buf[2];
    }
}


void send_data(void* responder, unsigned short *store, int arr_len){
  if(zmq_send(responder, store, arr_len*2, 0) == -1)
      fprintf( stderr, "Error sending data\n");
}


void INThandler(int sig){
  signal(sig, SIG_IGN);
  printf("Closing Program...\n");

  spiClose(handler);
  gpioTerminate();
}
