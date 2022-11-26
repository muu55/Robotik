if ((dist(1) > 150.0) && (dist(2) < 150.0) && (motor_count() < 1)){
  motor_on(1000, MOTOR1_FWD|MOTOR2_FWD);
}
if ((dist(2) > 150) && (motor_count() < 1)){
  motor_on(600, MOTOR1_FWD|MOTOR2_FWD);
  usleep(1000000);
  motor_on(2150, MOTOR1_REV|MOTOR2_FWD);
  usleep(2150000);
  motor_pwm(100, 100);
  motor_on(1000, MOTOR1_FWD|MOTOR2_FWD);
  usleep(1000000);
  motor_off();
  running = 0;
  motor_pwm(60, 60);
}






