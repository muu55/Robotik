usleep(15000);
if ((dist(1) >= 100.0) && (motor_count() < 1) && (status == 0)){
  motor_on(1000, MOTOR1_FWD|MOTOR2_FWD);
  usleep(1000000);
}
if ((dist(0) <= 100.0) && (dist(1) <= 100.0) && (dist(2) <= 100.0)){
  printf("\n Wand: %8.1f    %8.1f    %8.1f", dist(0 ), dist(1), dist(2));
  status = 1;
  motor_on(3000, MOTOR1_REV|MOTOR2_REV);
  usleep(3000000);
  if(dist(2) >= 150.0){
    motor_on(2150, MOTOR1_REV|MOTOR2_FWD);
    usleep(2150000);
    motor_on(2000, MOTOR1_FWD|MOTOR2_FWD);
    usleep(2000000);
    running = 0;
    motor_off();
    status = 0;
  }
  else if (dist(0) >= 150.0){
    motor_on(2150, MOTOR1_FWD|MOTOR2_REV);
    usleep(2150000);
    motor_on(2000, MOTOR1_FWD|MOTOR2_FWD);
    usleep(2000000);
    running = 0;
    motor_off();
    status = 0;
  }
}