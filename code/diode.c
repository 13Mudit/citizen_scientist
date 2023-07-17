#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <pigpio.h>

#define ARR_LEN 100   //100 for zener 
#define SHOW_LEN (ARR_LEN*4)
/*
   gcc -Wall -pthread -o diode.exe diode.c -lpigpio -lrt
*/

int handler;

void INThandler(int);

int main(int argc, char *argv[])
{
   int status;
   int pwm_pin = 12, freq = 100000, duty_cycle = 5000;
   int i;
   double current = 0, voltage = 0;
   double start, diff;
   int speed = 1600000;
   char buf[3];

   FILE * temp = fopen("adc_data.temp", "w");
   FILE * gnuplotPipe = popen ("gnuplot -persistent", "w");

   float store_voltage[ARR_LEN];
   float store_current[ARR_LEN];
   for(int j=0; j<ARR_LEN; j++){
      store_voltage[j] = 0;
      store_current[j] = 0;
   }

   if (gpioInitialise() < 0) return 1;

   handler = spiOpen(0, speed, 0);

   if (handler < 0) return 2;
   start = time_time();

   for (i=0; i<=1000000; i+=10000)     //i<=1000000 for zener
   {
      status = gpioHardwarePWM(pwm_pin, freq, i);
      gpioDelay(20000);
      //SPI read starts after 11000 is transmitted
      //00000000 00000000 00011000 -> OUTPUT
      //     N-- -------- --       -> INPUT

      voltage = 0;
      for(int k = 0; k<128; k++){
         buf[0] = 6;
         buf[1] = 2 << 6;
         buf[2] = 0;
         spiXfer(handler, buf, buf, 3);
         voltage += ((buf[1]&15)<<8) + buf[2];
      }
      
      voltage /= 128;
//      voltage = (voltage*3.3*4.3)/4096;  //for zeners
      voltage = (voltage*3.3*4.3)/4096; 
//      store_voltage[i/10000] = -voltage;  //for zeners
      store_voltage[i/10000] = voltage;

      current = 0;
      for(int k = 0; k<128; k++){
      	buf[0] = 6;
      	buf[1] = 1 << 6;
      	buf[2] = 0;
      	spiXfer(handler,buf,buf,3);
      	current += ((buf[1]&15)<<8) + buf[2];	
      }

      current /= 128;     
      current = (current*33*2)/4096; 
//      store_current[i/10000]  = -current; //FOR ZENERS
      store_current[i/10000] = current;
   }

   status = gpioHardwarePWM(pwm_pin, 0,0);

   diff = time_time() - start;
   fprintf(stderr, "Reading done, plotting graph...\n");

   for(i=0; i<ARR_LEN; i++)
      fprintf(temp, "%f %f\n",store_voltage[i], store_current[i]);

   fprintf(gnuplotPipe, "set xlabel 'Voltage(V)'\n" "set ylabel 'Current(mA)'\n" );
   fprintf(gnuplotPipe, "set title 'LED(WHITE) Characteristic Curve'\n");
   fprintf(gnuplotPipe, "plot [0:4] [-5:65] 'adc_data.temp' pt 7\n");

   
   fprintf(stderr, "sps=%.1f @ %d bps (%d/%.1f)\n",
      ARR_LEN / diff, speed, ARR_LEN, diff);

   spiClose(handler);

   gpioTerminate();

   return 0;
}

void INThandler(int sig){
    signal(sig, SIG_IGN);
    printf("Closing Program...");

    spiClose(handler);
    gpioTerminate();
}