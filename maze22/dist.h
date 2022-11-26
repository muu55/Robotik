#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

//-------------------------------------------------------------------
// HC-SR04  : Pi
// Vcc      : +5V 
// Trigger0 : GPIO19
// Echo0    : ---->|--+
// Trigger1 : GPIO20  |
// Echo1    : ---->|--+
// Trigger2 : GPIO21  |
// Echo2    : ---->|--+--[330]-- GPIO26 --[470]-- GND 
// Gnd      : GND 

const char *chipname = "gpiochip0";

struct gpiod_chip *chip;
struct gpiod_line *button, *echo, *trigger[3], *led[1]; 

int us_open(void) {
  int ret; 

  // Open GPIO chip
  chip = gpiod_chip_open_by_name(chipname);
  if (!chip) {
    perror("Open chip failed\n");
    return 1;
  }

  // Open GPIO lines
  trigger[0] = gpiod_chip_get_line(chip, 19);
  if (!trigger[0]) {
    perror("Get line 19 failed\n");
    return 2;
  }

  trigger[1] = gpiod_chip_get_line(chip, 20);
  if (!trigger[1]) {
    perror("Get line 20 failed\n");
    return 2;
  }

  trigger[2] = gpiod_chip_get_line(chip, 21);
  if (!trigger[2]) {
    perror("Get line 21 failed\n");
    return 2;
  }

  led[0] = gpiod_chip_get_line(chip, 16);
  if (!led[0]) {
    perror("Get line 16 failed\n");
    return 2;
  }

  echo = gpiod_chip_get_line(chip, 26);
  if (!echo) {
    perror("Get line 26 failed\n");
    return 2;
  }

  button = gpiod_chip_get_line(chip, 4);
  if (!button) {
    perror("Get line 4 failed\n");
    return 2;
  }

  ret = gpiod_line_request_output(trigger[0], "trigger0", 0);
  if (ret < 0) {
    perror("Request line 19 as output failed\n");
    return 3;
  }

  ret = gpiod_line_request_output(trigger[1], "trigger1", 0);
  if (ret < 0) {
    perror("Request line 20 as output failed\n");
    return 3;
  }

  ret = gpiod_line_request_output(trigger[2], "trigger2", 0);
  if (ret < 0) {
    perror("Request line 21 as output failed\n");
    return 3;
  }

  ret = gpiod_line_request_output(led[0], "led0", 0);
  if (ret < 0) {
    perror("Request line 16 as output failed\n");
    return 3;
  }

  ret = gpiod_line_request_input(echo, "echo");
  if (ret < 0) {
    perror("Request line 26 as input failed\n");
    return 3;
  }

  ret = gpiod_line_request_input(button, "button");
  if (ret < 0) {
    perror("Request line 4 as input failed\n");
    return 3;
  }

  return 0;
}

void us_close(void) {
  gpiod_line_release(button);
  gpiod_line_release(echo);
  gpiod_line_release(trigger[2]);
  gpiod_line_release(trigger[1]);
  gpiod_line_release(trigger[0]);
  gpiod_line_release(led[0]);
  gpiod_chip_close(chip);
}

bool us_button() {
  return (gpiod_line_get_value(button) == 0);
}

void us_led(bool value) {
  gpiod_line_set_value(led[0], value);
}

double us_dist(int id) {
  if ((id < 0) || (id > 2)) return 0.0;

  struct timeval before, after;
  int loop, count, value;

  for (loop = 0; loop < 10; loop++) {
  
    // hc-sr04 senden
    gpiod_line_set_value(trigger[id], 1);
    usleep(1500);
    gpiod_line_set_value(trigger[id], 0);
  
    // echo empfangen
    count = 0; value = 0; 
    while ((count < 6000) && (value < 3)) {
      if (gpiod_line_get_value(echo) == 1) value++; else value = 0;
      count++;  
    }

    // Kein Echo, nochmal versuchen
    if (value < 3) continue;

    gettimeofday(&before, NULL);

    count = 0; value = 0;
    while ((count < 10000) && (value < 3)) {
      if (gpiod_line_get_value(echo) == 0) value++; else value = 0;
      count++;  
    }
    gettimeofday(&after , NULL);

    if (count < 10000) break;
  }
  
  // Laufzeit [ms]
  double delta = 1000.0 * (after.tv_sec  - before.tv_sec) + 
                  0.001 * (after.tv_usec - before.tv_usec);

  if (value < 3) delta = 0;

  // Entfernung [mm]
  double dist = delta * 343 / 2;

  //printf("dist[%d]: %3d %5d %.3f ms ==> %8.1f mm\n", id, loop, count, delta, dist);

  return dist;   
}
