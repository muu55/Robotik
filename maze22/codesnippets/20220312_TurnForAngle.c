// before while loop

cmps12_update();
int startwert = cmps12.kompass;
printf(" Startwert: %d \n", startwert);
int wanted = 1800;
int total = startwert + wanted;

if (total >= 3600){
  int rest = total - 3600;
  wanted = rest;
}
else{
  wanted = startwert + wanted;
}

// in while loop

cmps12_update();

if((cmps12.kompass - 20 < wanted) && (cmps12.kompass + 20 < wanted) && (motor_count() <= 10)){
  motor_pwm(30, 30);
  motor_on(10, MOTOR1_REV|MOTOR2_FWD);
  //startwert = wanted;
}
else if((cmps12.kompass - 20 > wanted) && (cmps12.kompass + 20 > wanted) && (motor_count() <= 10)){
  motor_pwm(30, 30);
  motor_on(10, MOTOR1_FWD|MOTOR2_REV);
  //startwert = wanted;
}


// functions

int turninit(int start, int val){
  int startwert = start;
  printf(" Startwert: %d \n", startwert);
  int wanted = val;
  int total = startwert + wanted;

  if (total >= 3600){
    int rest = total - 3600;
    wanted = rest;
    return wanted;
  }
  else{
    wanted = startwert + wanted;
    return wanted;
  }
}

int wanted = turninit(900); // in main

void turn(int val){
  int wanted = val;
  cmps12_update();

if((cmps12.kompass - 20 < wanted) && (cmps12.kompass + 20 < wanted) && (motor_count() <= 10)){
  motor_pwm(30, 30);
  motor_on(10, MOTOR1_REV|MOTOR2_FWD);
  //startwert = wanted;
}
else if((cmps12.kompass - 20 > wanted) && (cmps12.kompass + 20 > wanted) && (motor_count() <= 10)){
  motor_pwm(30, 30);
  motor_on(10, MOTOR1_FWD|MOTOR2_REV);
  //startwert = wanted;
}
}
