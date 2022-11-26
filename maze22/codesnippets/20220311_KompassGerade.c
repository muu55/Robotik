cmps12_update();

if((cmps12.bearing < 0) || (cmps12.bearing > 150) && (motor_count() <= 10)){
  motor_pwm(30, 30);
  motor_on(10, MOTOR1_REV|MOTOR2_FWD);
}
else if((cmps12.bearing > 0) && (motor_count() <= 10)){
  motor_pwm(30, 30);
  motor_on(10, MOTOR1_FWD|MOTOR2_REV);
}