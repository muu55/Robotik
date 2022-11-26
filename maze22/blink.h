
static
int blinking = 0;

void *blinker(void *context) {
  while (0 < blinking--) {
    us_led(1); usleep(100000);
    us_led(0); usleep(100000);
  }

  return NULL;
}

static 
pthread_t blink_thread;

static
void blink(int num) {
  blinking = num;
  if (pthread_create(&blink_thread, NULL, blinker, NULL)) {
    fprintf(stderr, "Error creating blink thread\n");
  }
}

