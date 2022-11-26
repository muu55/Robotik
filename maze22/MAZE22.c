//compile: gcc -lm -lpthread -lgpiod -o maze20 maze20.c

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/times.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <errno.h>
#include <math.h>

typedef unsigned char byte;

//=====================================================================

int running = 1;
int autonom = 0;


//=====================================================================

static
clock_t clock_now() {
  struct timespec now;

  clock_gettime(CLOCK_MONOTONIC, &now);

  return 1000000 * now.tv_sec + now.tv_nsec/1000;
}

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include "i2c-devices.h"
#include "color.h"
#include "blink.h"

//=====================================================================

int  len = 0;
char line[80];

void *read_cmdline(void *context) {
  while (running && (len = read(0, line, sizeof(line)-1))) {
    if (line[len-1] == '\n') line[--len] = 0;
  }

  return NULL;
}

//=====================================================================

#include "dist.h"

pthread_spinlock_t dist_spinlock;
double distance[3]; // protected by spinlock
long   dist_count = 0;

// Startknopf
int pressed = 0;


double dist(int id) { 
  pthread_spin_lock(&dist_spinlock);
  double value = distance[id];
  pthread_spin_unlock(&dist_spinlock);

  return value;
}

void dist_set(int id, double value) { 
  pthread_spin_lock(&dist_spinlock);
  distance[id] = value;
  pthread_spin_unlock(&dist_spinlock);
}

void *read_distance(void *context) {
  if (us_open()) {
    printf("us_open() failed.\n");
    return NULL;
  }

  pressed = 0;

  distance[0] = 0.0;
  distance[1] = 0.0;
  distance[2] = 0.0;

  while (running) {
    if (us_button()) pressed++;

    dist_set(0, us_dist(0)); // links
    dist_set(1, us_dist(1)); // mitte
    dist_set(2, us_dist(2)); // rechts
    dist_count++;
  }

  us_close();

  return NULL;
}

//=====================================================================

#define ADC 0
#define ADC00 "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define ADC01 "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define ADC02 "/sys/bus/iio/devices/iio:device0/in_voltage2_raw"
#define ADC03 "/sys/bus/iio/devices/iio:device0/in_voltage3_raw"
#define ADC04 "/sys/bus/iio/devices/iio:device0/in_voltage4_raw"
#define ADC05 "/sys/bus/iio/devices/iio:device0/in_voltage5_raw"
#define ADC06 "/sys/bus/iio/devices/iio:device0/in_voltage6_raw"
#define ADC07 "/sys/bus/iio/devices/iio:device0/in_voltage7_raw"

#define ADC10 "/sys/bus/iio/devices/iio:device1/in_voltage0_raw"
#define ADC11 "/sys/bus/iio/devices/iio:device1/in_voltage1_raw"
#define ADC12 "/sys/bus/iio/devices/iio:device1/in_voltage2_raw"
#define ADC13 "/sys/bus/iio/devices/iio:device1/in_voltage3_raw"
#define ADC14 "/sys/bus/iio/devices/iio:device1/in_voltage4_raw"
#define ADC15 "/sys/bus/iio/devices/iio:device1/in_voltage5_raw"
#define ADC16 "/sys/bus/iio/devices/iio:device1/in_voltage6_raw"
#define ADC17 "/sys/bus/iio/devices/iio:device1/in_voltage7_raw"

#define MOTOR1_ENABLE "/sys/class/gpio/gpio22/value"
#define MOTOR2_ENABLE "/sys/class/gpio/gpio23/value"
#define MOTOR1_DIR    "/sys/class/gpio/gpio24/value"
#define MOTOR2_DIR    "/sys/class/gpio/gpio25/value"

#define MOTOR1_PWM "/sys/class/pwm/pwmchip0/pwm0/duty_cycle"   
#define MOTOR2_PWM "/sys/class/pwm/pwmchip0/pwm1/duty_cycle"   

void setPin(const char *pin, int value) {
  char buff[8];
  int fd = open(pin, O_WRONLY);

  if (fd < 0) {
    printf("setPin(%s,%d) failed.\n", pin, value);
    return;
  }

  sprintf(buff, "%d", value);
  if (write(fd, buff, 1+strlen(buff)) < 0) { perror("write()"); }

  close(fd);
}

int getPin(const char *pin) {
  int value = 0, fd = open(pin, O_RDONLY);

  if (fd < 0) {
    printf("getPin(%s) failed.\n", pin);
    return value;
  }

  { char buff[8];
    if (read(fd, buff, 8) > 0) value = atoi(buff); 
  }

  close(fd);

  return value;
}

//=====================================================================

#define MOTOR1_FWD 0x01
#define MOTOR1_REV 0x02
#define MOTOR2_FWD 0x04
#define MOTOR2_REV 0x08

pthread_spinlock_t motor_spinlock;
unsigned int _motor_count = 0; // protected by spinlock

unsigned int motor_count() {
  unsigned int count;

  pthread_spin_lock(&motor_spinlock);
  count = _motor_count;
  pthread_spin_unlock(&motor_spinlock);

  return count;
}

void motor_set_count(unsigned int count) {
  pthread_spin_lock(&motor_spinlock);
  _motor_count = count;
  pthread_spin_unlock(&motor_spinlock);
}

void motor_off() {
  motor_set_count(0);

  setPin(MOTOR1_ENABLE, 0);
  setPin(MOTOR2_ENABLE, 0);

  //printf("motor off\n");
}

void *motor_countdown(void *context) {
  while (running) {
    int state = 0;

    pthread_spin_lock(&motor_spinlock);
    if (_motor_count) state = (_motor_count-- != 1) ? 1 : 2;
    pthread_spin_unlock(&motor_spinlock);

    if (state == 0) usleep(10000); // idle wait
    if (state == 1) usleep(1000);  // motor running
    if (state == 2) motor_off();   
  }

  return NULL;
}

void motor_on(unsigned int count, char mode) {
  int en1, en2, dir1, dir2;

  //printf("motor %02X %d\n", mode, count);

  if (mode & MOTOR1_FWD) {
    en1 = 1; dir1 = 0;
  } else if (mode & MOTOR1_REV) {
    en1 = 1; dir1 = 1;
  } else {
    en1 = 0; dir1 = 0;
  }

  if (mode & MOTOR2_FWD) {
    en2 = 1; dir2 = 0;
  } else if (mode & MOTOR2_REV) {
    en2 = 1; dir2 = 1;
  } else {
    en2 = 0; dir2 = 0;
  }

  //printf("en1=%d, dir1=%d, en2=%d, dir2=%d\n", en1, dir1, en2, dir2);

  setPin(MOTOR1_DIR, dir1); 
  setPin(MOTOR2_DIR, dir2); 

  setPin(MOTOR1_ENABLE, en1); 
  setPin(MOTOR2_ENABLE, en2); 

  motor_set_count(count);
}

void motor_wait() {
  while (motor_count()){
	usleep(10000);
	if (pressed > 0) {
        pressed = 0;
        running = 0;
        motor_set_count(0);
        motor_off();}
}
}

void motor_pwm(int pwm1, int pwm2) {
  //printf("pwm %4d %4d\n", pwm1, pwm2);

  if (pwm1 < 0)   pwm1 = 0;
  if (pwm1 > 100) pwm1 = 100;
  if (pwm2 < 0)   pwm2 = 0;
  if (pwm2 > 100) pwm2 = 100;

  setPin(MOTOR1_PWM, 500*pwm1);
  setPin(MOTOR2_PWM, 500*pwm2);
}

//=====================================================================

void show() { 
  int value[16], pwm1, pwm2;

#if ADC
  value[0]  = getPin(ADC00);
  value[1]  = getPin(ADC01);
  value[2]  = getPin(ADC02);
  value[3]  = getPin(ADC03);
  value[4]  = getPin(ADC04);
  value[5]  = getPin(ADC05);
  value[6]  = getPin(ADC06);
  value[7]  = getPin(ADC07);

  value[8]  = getPin(ADC10);
  value[9]  = getPin(ADC11);
  value[10] = getPin(ADC12);
  value[11] = getPin(ADC13);
  value[12] = getPin(ADC14);
  value[13] = getPin(ADC15);
  value[14] = getPin(ADC16);
  value[15] = getPin(ADC17);
#endif

  pwm1 = getPin(MOTOR1_PWM)/500;
  pwm2 = getPin(MOTOR2_PWM)/500;

  printf("pwm: %3d %3d, count: %d\n", pwm1, pwm2, motor_count());

  printf("dist: %8.1f %8.1f %8.1f mm\n", dist(0), dist(1), dist(2));

  apds9960_read(value);
  printf("light: %5d, rgb: %5d %5d %5d\n", 
          value[0], value[1], value[2], value[3]); 

#if ADC
  printf("adc: %4d %4d %4d %4d  %4d %4d %4d %4d  %4d %4d %4d %4d  %4d %4d %4d %4d\n", 
         value[0],  value[1],  value[2],  value[3],  
         value[4],  value[5],  value[6],  value[7], 
         value[8],  value[9],  value[10], value[11], 
         value[12], value[13], value[14], value[15]); 
#endif
}

void beep() { 
  printf("beep!\n"); 
}
void stop() {
  printf("stopped!\n");
}

int turninit(int start, int val){ // start equals current compass value // val equals angle to turn for
  int startwert = start;
  printf(" Startwert: %d \n", startwert);
  int wanted = val;
  int total = startwert + wanted;

  if (total >= 3600){ // testing if val is valid with current start value
    int rest = total - 3600;
    wanted = rest;
    return wanted; // if start + val too big, return rest value as new wanted angle
  }
  else{
    wanted = startwert + wanted;
    return wanted; // else just return the shit right away
  }
}


void turn(int val){
  int wanted = val;
  cmps12_update();

if((cmps12.kompass - 20 < wanted) && (cmps12.kompass + 20 < wanted) && (motor_count() <= 10)){ // do we have to turn left? (bad compass values taken into account)
  motor_pwm(40, 40);
  motor_on(10, MOTOR1_REV|MOTOR2_FWD); // THEN DO IT!
  //startwert = wanted;
}
else if((cmps12.kompass - 20 > wanted) && (cmps12.kompass + 20 > wanted) && (motor_count() <= 10)){ // do we have to turn right? (bad compass values taken into account)
  motor_pwm(40, 40);
  motor_on(10, MOTOR1_FWD|MOTOR2_REV); // THEN DO IT
  //startwert = wanted;
}
}

void holdpath(int val){
  int wanted = val;
  cmps12_update();

if((cmps12.kompass - 20 < wanted) && (cmps12.kompass + 20 < wanted) && (motor_count() <= 10)){ // do we have to turn left? (bad compass values taken into account)
  motor_pwm(50, 50);
  motor_on(10, MOTOR1_REV|MOTOR2_FWD); // THEN DO IT!
  //startwert = wanted;
}
else if((cmps12.kompass - 20 > wanted) && (cmps12.kompass + 20 > wanted) && (motor_count() <= 10)){ // do we have to turn right? (bad compass values taken into account)
  motor_pwm(50, 50);
  motor_on(10, MOTOR1_FWD|MOTOR2_REV); // THEN DO IT
  //startwert = wanted;
}
}


//=====================================================================


int main(int argc, char *argv[]) {
  struct tms tms_data;
  pthread_t dist_thread, motor_thread, cmdline_thread;
  int count = 0;

  i2c_connect(APDS9960_ADDR);
  if (i2c_file < 0) {
    fprintf(stderr, "Error opening I2C device\n");
    return 1;
  }
  apds9960_init();

  pthread_spin_init(&motor_spinlock, 0);
  if (pthread_create(&motor_thread, NULL, motor_countdown, NULL)) {
    fprintf(stderr, "Error creating motor thread\n");
    return 1;
  }
  motor_off(); motor_pwm(60,60);

  if (pthread_create(&cmdline_thread, NULL, read_cmdline, NULL)) {
    fprintf(stderr, "Error creating cmdline thread\n");
    return 1;
  }

  pthread_spin_init(&dist_spinlock, 0);
  if (pthread_create(&dist_thread, NULL, read_distance, NULL)) {
    fprintf(stderr, "Error creating distance thread\n");
    return 1;
  }

  long clk_tck = sysconf(_SC_CLK_TCK);
  clock_t start = times(&tms_data);
  //printf("clock: start=%ld, %d Hz\n", start, clk_tck);

  printf("# Waiting for commands:\n");

usleep(100000);

//===============================================================================
cmps12_update();
int startwert = cmps12.kompass;
int wanted;
int onetime = 1;
char status = 'n'; // n = nothing; r = right; l = left; b = back; d = dead end;
int compassval;
//===============================================================================

  while (running) {

    if (len) {
      clock_t now = times(&tms_data);

      printf("%8.2f> %s\n", (double)(now-start)/(double)clk_tck, line);

      if (strncmp(line, "auto", 4) == 0) {
        int val;

        if (sscanf(line+5, "%d", &val) == 1) {
          if (val > 0) autonom = val;
        }
      } else if (strncmp(line, "move", 4) == 0) {
        int val;

        if (sscanf(line+5, "%d", &val) == 1) {
          if (val > 0) {
            motor_on(val, MOTOR1_FWD|MOTOR2_FWD);
          } else if (val < 0) {
            motor_on(-val, MOTOR1_REV|MOTOR2_REV);
          } else motor_off();
        } else {
          printf("%8d ms\n", motor_count());
        }
        } else if (strncmp(line, "led", 3) == 0) {
          int val;
  
         if (sscanf(line+4, "%d", &val) == 1) {
            us_led(val != 0);
        }
      } else if (strncmp(line, "turn", 4) == 0) {
        int val;

        if (sscanf(line+5, "%d", &val) == 1) {
          if (val > 0) {
            motor_on(val, MOTOR1_REV|MOTOR2_FWD);
          } else if (val < 0) {
            motor_on(-val, MOTOR1_FWD|MOTOR2_REV);
          } else motor_off();
        } else {
          printf("%8d ms\n", motor_count());
        }
      } else if (strncmp(line, "pwm", 3) == 0) {
        int val1, val2;

        if (sscanf(line+4, "%d %d", &val1, &val2) == 2) {
          motor_pwm(val1, val2);
        } else {
          val1 = getPin(MOTOR1_PWM)/500;
          val2 = getPin(MOTOR1_PWM)/500;
          printf("pwm: %3d %3d\n", val1, val2);
        }
      } else if (strncmp(line, "beep", 4) == 0) {
        beep();
      } else if (strncmp(line, "show", 4) == 0) {
        show();
      } else if (strncmp(line, "stop", 4) == 0) {
        autonom = 0;
      } else if (strncmp(line, "quit", 4) == 0) {
        running = 0;
      } else { printf("unknown cmd: %s\n", line); }

      len = 0;
      memset(line, 0, sizeof(line));
    } else { // no command
    }

#if 0
    if (autonom) {
      if (pressed > 0) {
        printf("stop\n"); pressed = 0; autonom = 0;
        running = 0;
        motor_off();
      }
    } 
    /*else {
      if (pressed > 0) {
        pressed = 0; autonom = 1;
        running = 0;
        motor_off();
      }
    }*/
#endif

#if 0
    if (dist(1) <= 50.0) { 
      //printf("dist: %8.1f %8.1f %8.1f mm\n", dist(0), dist(1), dist(2));
    }
#endif

//==================================
int value[4];
apds9960_read(value); 
char farbe = color(value[0], value[1], value[2], value[3]); // get color status out of the sensor values
motor_pwm(60, 60);
//==================================


cmps12_update();

// if movement block
if ( (dist(1) >= 150.0) && (status == 'n') ) { // if front free
  motor_on(10, MOTOR1_FWD|MOTOR2_FWD); // move forward
  if (count%10 == 0) { // do not turn every time, instead every 10th time
    holdpath(startwert); // correct to hold path
  }
}
else if ( (dist(1) <= 150) && (dist(2) >= 150)  && (status == 'n') ) { // if front blocked and right clear
  //=============
  cmps12_update();
  startwert = cmps12.kompass;
  //=============
  wanted = turninit(startwert, 900); // set wanted to 90° right from current angle
  status = 'r'; // set status to turn right
  continue; // get out to avoid LOP
}
else if ( (dist(1) <= 150) && (dist(0) >= 150) && (status == 'n') ) { // if front blocked and left clear
  //=============
  cmps12_update();
  startwert = cmps12.kompass;
  //=============
  wanted = turninit(startwert, -900); // set wanted to 90° left from current angle
  status = 'l'; // set status to turn left
  //========
if ( (status == 'l') || (status == 'r') ) {
  cmps12_update();
  compassval = cmps12.kompass;
  //========
  turn(wanted);
  // ↓ tolerance for turning ↓
  if ( (compassval - 10 < wanted) && (compassval + 10 < wanted) || // if compass -10 < wanted & compass +20 > wanted
  (compassval - 10 > wanted) && (compassval + 10 > wanted) ) { // if compass -10 > wanted & compass +10 > wanted
  status = 'n'; // turning complete, set status to nothing
  startwert = compassval; // set startwert to correct value for good orientation
  continue; // get out to avoid LOP - turn successful
  }
  else {
    continue; // get out to avoid LOP
  }
  }
}

// if colors block
if (farbe == 'b') { // black detected?
  motor_on(1000, MOTOR1_REV|MOTOR2_REV); // reverse!
  usleep(1000); // wait for reverse to end
  if ( (dist(0) >= 150) ) { // if left free
    status = 'l'; // set status to turn left
    }
}
else if ( (farbe == 'r') && (status == 'n') ){ // red detected and nothing important to do?
  blink(1); // blink to signal that red was tected
  usleep(5000); // wait 5 sec at red point to meet requirements
}


if (count%200 == 0) {
  //printf("\n Wand: %8.1f    %8.1f    %8.1f", dist(0 ), dist(1), dist(2));
  //printf("Farbe: %c \n", farbe);
  //printf("Light: %d \n", value[0]);
  //printf("Status: %c \n", status);
  //printf("kompass: %4d, bearing: %4d, pitch: %d, roll: %d\n", cmps12.kompass, cmps12.bearing, cmps12.pitch, cmps12.roll);
}



if (pressed > 0) {
  blink(1);
  motor_set_count(0);
  motor_off();
  //usleep(1000); // give the user some time
  pressed = 0;
    while (true) {
      usleep(20);
      if (pressed > 0) {
        pressed = 0;
        break;
        }
    }
}




#if 0
    switch (autonom) { 
      case 1: {
        if (dist(1) <= 50.0) { motor_off(); autonom = 0; }
      } break;
      case 2: {

      } break;
      case 3: {

      } break;
      default: { 
        if (dist(1) <= 50.0) {
          motor_off();
          printf("==> %8.1f mm\n", dist(1));
        }

        if (count%100 == 0) {
          printf("%2d %4d %6d %8.1f %8.1f %8.1f mm\n", autonom, pressed, dist_count, dist(0), dist(1), dist(2));
          dist_count = 0;
          pressed = 0;
        }
      }
    }
#endif 

    usleep(10000); count++;
  }

  pthread_join(dist_thread, NULL);
  pthread_join(motor_thread, NULL);

  pthread_spin_destroy(&dist_spinlock);
  pthread_spin_destroy(&motor_spinlock);

  //pthread_join(cmdline_thread, NULL);

  //printf("# Goodbye, and thanks for all the fish.\n");
  return EXIT_SUCCESS;
  us_led(0);
}

//=====================================================================
