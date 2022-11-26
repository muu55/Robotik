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