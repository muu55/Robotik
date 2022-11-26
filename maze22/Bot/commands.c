val = 1000;
motor_on(val, MOTOR1_FWD|MOTOR2_FWD); // motor on for 1 second - forward

val = 1000;
motor_on(-val, MOTOR1_REV|MOTOR2_REV); // motor on for 1 second - reverse

val = 1000;
motor_on(val, MOTOR1_REV|MOTOR2_FWD); // turn right for 1 second

val = 1000;
motor_on(-val, MOTOR1_FWD|MOTOR2_REV); // turn left for 1 second

motor_off();                           // motor off

motor_pwm(80,80);                      // set pwm to 80 80


running = 0;                           // quit while loop