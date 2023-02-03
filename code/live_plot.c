
#include<string.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>

#include<signal.h>

#include<zmq.h>
#include<pigpio.h>
/*
   gcc -Wall -pthread -o test.exe live_plot.c -lpigpio -lrt -lzmq
*/

#define ARR_LEN 10000
#define TRIGGER_PIN 26


int handler;

void INThandler(int);

int main(){
  printf("Started\n");

  void *context = zmq_ctx_new();
  void *responder = zmq_socket(context, ZMQ_REP);

  char recv_buf[10];
   
  printf("Context created!\n");
  int i;
  int speed = 800000;
  char buf[3];

  short store[ARR_LEN];
  for(int j=0; j<ARR_LEN; j++){
    store[j] = 0;
  }

  if(gpioInitialise() < 0) return 1;
  
  
  int rc = zmq_bind(responder, "tcp://*:5555");
  assert(rc == 0);
  printf("Server bound\n"); 


  handler = spiOpen(0, speed, 0);
  if(handler < 0) return 2;

  while(1){
    printf("Waiting Trigger\n");
    zmq_recv(responder, recv_buf, 10, 0);
    printf("Received %s", recv_buf);
    
    while(1)
      if(gpioRead(TRIGGER_PIN) == PI_LOW)
        break;
    

    for(i=0; i<ARR_LEN; i++){
      /*
      SPI read starts after 11000 is transmitted
      1 -> start bit
      1 -> single/differential signal
      0 -> X for 4 bit ADC, MSB for 8 bit ADC
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
      buf[1] = 0; //0b00000000
      buf[2] = 0; //0b00000000

      spiXfer(handler, buf, buf, 3);
      store[i] = ((buf[1]&15)<<8) + buf[2];
    }

    if(zmq_send(responder, &store, sizeof(store), 0) != -1)
      printf("%d bytes sent successfully\n", sizeof(store));
    else
      printf("Error sending data\n"); 

    //for(int i=0; i<ARR_LEN; i++){
    //  printf("%d ", store[i]);
    //}

    sleep(1);
  }  
  
  spiClose(handler);

  gpioTerminate();

  return 0;
}

void INThandler(int sig){
  signal(sig, SIG_IGN);
  printf("Closing Program...\n");

  spiClose(handler);
  gpioTerminate();
}
