
#include<string.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

#include<signal.h>
#include<time.h>

#include<zmq.h>
#include<pigpio.h>
/*
   gcc -Wall -pthread -o test_time.exe live_timing.c -lpigpio -lrt -lzmq
*/

#define MAX_ARR_LEN 100000
#define TRIGGER_PIN 26


int handler;

void INThandler(int);
void get_data(char buf[3], char adc_channel, short store[MAX_ARR_LEN], int arr_len);
void send_data(void* responder, short store[MAX_ARR_LEN], int arr_len);

int main(){
  printf("Started\n");

  long int start_time;
  long int time_difference;
  struct timespec gettime_now;

  void *context = zmq_ctx_new();
  void *responder = zmq_socket(context, ZMQ_REP);

  char recv_buf[10];
   
  printf("Context created!\n");
  int speed = 800000;
  char adc_channel = 0;
  char buf[3];

  short store[MAX_ARR_LEN];
  for(int j=0; j<MAX_ARR_LEN; j++){
    store[j] = 0;
  }

  if(gpioInitialise() < 0) return 1;
  
  //0 or 1, depending upon recv_buf
  //0 -> stream data
  //1 -> single shot data
  int mode;

  int arr_len;
  
  int rc = zmq_bind(responder, "tcp://*:5555");
  assert(rc == 0);
  printf("Server bound\n"); 


  handler = spiOpen(0, speed, 0);
  if(handler < 0) return 2;

  while(1){
    printf("Waiting for message!!\n");
    zmq_recv(responder, recv_buf, 10, 0);
    printf("Received: %s", recv_buf);
    
    mode = *recv_buf - '0';
    arr_len = atoi(recv_buf+2);


    //different operations depending on received instruction 
    if(mode){
      //Single shot data send
      zmq_send(responder, "Single", 6, 0);
      printf("Waiting Trigger\n");
      
      while(1)
        if(gpioRead(TRIGGER_PIN) == PI_LOW){
          clock_gettime(CLOCK_REALTIME, &gettime_now);
          start_time = gettime_now.tv_nsec;
          break;
        }
      

      get_data(buf, adc_channel, store, arr_len);
      
      clock_gettime(CLOCK_REALTIME, &gettime_now);
      time_difference = gettime_now.tv_nsec - start_time;
      if(time_difference<0)
        time_difference += 1000000000;

      send_data(responder, store, arr_len);

      printf("\n%ld ns time taken\n", time_difference);
    }
    else{
      zmq_send(responder, "Stream", 6, 0);
      
      while(1){
        zmq_recv(responder, recv_buf, 1, 0);
        if(*recv_buf - '0' == 0)
          break;

        get_data(buf, adc_channel, store, arr_len);
        send_data(responder, store, arr_len);
      }
    }

    sleep(1);
  }  
  
  spiClose(handler);

  gpioTerminate();

  return 0;
}


void get_data(char buf[3], char adc_channel, short store[MAX_ARR_LEN], int arr_len){
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


void send_data(void* responder, short store[MAX_ARR_LEN], int arr_len){
  if(zmq_send(responder, &store, arr_len, 0) != -1)
      printf("%d bytes sent successfully\n", arr_len);
    else
      printf("Error sending data\n"); 
}


void INThandler(int sig){
  signal(sig, SIG_IGN);
  printf("Closing Program...\n");

  spiClose(handler);
  gpioTerminate();
}
