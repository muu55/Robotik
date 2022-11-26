//=====================================================================
// The I2C bus: This is for V2 pi's. For V1 Model B you need i2c-0
static const char *devName = "/dev/i2c-1";

int i2c_file = -1;

void i2c_connect(int address) {
  if (i2c_file < 0) {
     printf("I2C: Open %s\n", devName);

     if ((i2c_file = open(devName, O_RDWR)) < 0) {
       fprintf(stderr, "I2C: Failed to access %d\n", devName);
       return;
     }
  }

  printf("I2C: connect to 0x%x\n", address);

  if (ioctl(i2c_file, I2C_SLAVE, address) < 0) {
    fprintf(stderr, "I2C: Failed to acquire bus access/talk to slave 0x%02x\n", address);
  }
}

int i2c_read(int addr, char data[], int len) {

  struct i2c_msg rdwr_msgs[1] = {
    { // Read buffer
      .addr = 0x04,
      .flags = I2C_M_RD, // read
      .len = 2,
      .buf = data
    }
  };

  struct i2c_rdwr_ioctl_data rdwr_data = {
    .msgs = rdwr_msgs,
    .nmsgs = 1
  };

  rdwr_msgs[0].addr = addr;
  rdwr_msgs[0].buf  = data;
  rdwr_msgs[0].len  = len;

  return ioctl(i2c_file, I2C_RDWR, &rdwr_data );
}

int i2c_write(int addr, char data[], int len) {

  struct i2c_msg rdwr_msgs[1] = {
    { // Read buffer
      .addr = 0x04,
      .flags = 0, // write
      .len = 1,
      .buf = data
    }
  };

  struct i2c_rdwr_ioctl_data rdwr_data = {
    .msgs = rdwr_msgs,
    .nmsgs = 1
  };

  rdwr_msgs[0].addr = addr;
  rdwr_msgs[0].buf  = data;
  rdwr_msgs[0].len  = len;

  return ioctl(i2c_file, I2C_RDWR, &rdwr_data );
}

//=====================================================================

#define BUTTON_ADDR 0x38

#define ledA 0x80
#define ledB 0x40
#define ledC 0x20
#define ledD 0x10
#define ledE 0x08
#define ledF 0x04
#define ledG 0x02
#define ledH 0x01

byte buttonA = 0;
byte buttonB = 0;
byte buttonC = 0;
byte buttonD = 0;
byte buttonE = 0;
byte buttonF = 0;
byte buttonG = 0;
byte buttonH = 0;

byte button_leds  = 0;
byte button_state = 0xFF;

void button_update() {
  byte data = 0xFF;

  i2c_write(BUTTON_ADDR, &data, 1); usleep(1000);
  i2c_read(BUTTON_ADDR, &data, 1); 
  
  if (data != button_state) {
    // printf("led: %02x button: %02x <-- %02x\n", button_leds, button_state, data);
    
    // Button A Release Event
    if ((0x80&data) > (0x80&button_state)) { buttonA = 1-buttonA; button_leds = (button_leds & ~ledA)|(~button_leds & ledA); }  

    // Button B Press Event
    if ((0x40&data) < (0x40&button_state)) { buttonB = 1-buttonB; button_leds = (button_leds & ~ledB)|(~button_leds & ledB); }

    // Button C Release Event
    if ((0x20&data) > (0x20&button_state)) { buttonC = 1-buttonC; button_leds = (button_leds & ~ledC)|(~button_leds & ledC); }

    // Button D Press Event
    if ((0x10&data) < (0x10&button_state)) { buttonD = 1-buttonD; button_leds = (button_leds & ~ledD)|(~button_leds & ledD); }

    // Button E..G Press Event
    if ((0x08&data) < (0x08&button_state)) { buttonE = 1; }  
    if ((0x04&data) < (0x04&button_state)) { buttonF = 1; }  
    if ((0x02&data) < (0x02&button_state)) { buttonG = 1; }  
    if ((0x01&data) < (0x01&button_state)) { buttonH = 1; }  
  }
  
  button_state = data; data = ~(button_leds);
  i2c_write(BUTTON_ADDR, &data, 1);
}

void led_update(byte led) {
  byte data = ~(button_leds = led);

  i2c_write(BUTTON_ADDR, &data, 1);
}

//=====================================================================

#define CMPS12_ADDR 0x60
#define CMPS12_TIME 20000

struct {
  clock_t last;
  unsigned int kompass;
  int bearing;
  int pitch;
  int roll;
} cmps12 = { 0, 0, 0, 0 };

void cmps12_update() {
#if 1
  clock_t now = clock_now();

  // skip if too early
  if (now < cmps12.last + CMPS12_TIME) return;
  cmps12.last = now;
#endif

  byte data[8];

  data[0] = 0;
  i2c_write(CMPS12_ADDR, data, 1); usleep(1000);
  i2c_read(CMPS12_ADDR, data, 6); 

  cmps12.bearing = data[1];

  { unsigned int h = data[2], l = data[3];
    cmps12.kompass = (h << 8) + l;
  }

  cmps12.pitch = (data[4] > 128) ? data[4]-255: data[4];
  cmps12.roll  = (data[5] > 128) ? data[5]-255: data[5];
}

//=====================================================================

#define PCA9685_ADDR 0x40

#define PCA9685_MODE1         0x00
#define PCA9685_MODE2         0x01
#define PCA9685_SUBADR1       0x02
#define PCA9685_SUBADR2       0x03
#define PCA9685_SUBADR3       0x04
#define PCA9685_ALLCALLADR    0x05
#define PCA9685_LED0_ON_L     0x06
#define PCA9685_LED0_ON_H     0x07
#define PCA9685_LED0_OFF_L    0x08
#define PCA9685_LED0_OFF_H    0x09
#define PCA9685_ALL_LED_ON_L  0xFA
#define PCA9685_ALL_LED_ON_H  0xFB
#define PCA9685_ALL_LED_OFF_L 0xFC
#define PCA9685_ALL_LED_OFF_H 0xFD
#define PCA9685_PRE_SCALE     0xFE

int pca9685_read(int addr, char reg) {
  byte data = reg;

  i2c_write(addr, &data, 1); usleep(1000);
  i2c_read(addr, &data, 1); 

  return data;
}

int pca9685_write(int addr, char reg, char value) {
  byte data[2];

  data[0] = reg;
  data[1] = value;

  return i2c_write(addr, data, 2);
}

void pca9685_init(int addr, double freq) {
  double scaleval = 25000000.0; // 25MHz
  scaleval /= 4096.0; // 12-bit
  scaleval /= freq;
  scaleval -= 1.0;
  char prescale = (char) floor(scaleval + 0.5);
  char oldmode = pca9685_read(addr, PCA9685_MODE1);
  char newmode = (oldmode & 0x7F) | 0x10;

  // init device
  pca9685_write(addr, PCA9685_MODE1, 0); usleep(5000);

  // set frequency
  pca9685_write(addr, PCA9685_MODE1, newmode);
  pca9685_write(addr, PCA9685_PRE_SCALE, prescale);
  pca9685_write(addr, PCA9685_MODE1, oldmode);
  //usleep(5000); // sleep 5 milliseconds
  pca9685_write(addr, PCA9685_MODE1, oldmode | 0x80);
}

void pca9685_pwm(int addr, int channel, int on, int off) {
  pca9685_write(addr, PCA9685_LED0_ON_L  + 4 * channel, on & 0xFF);
  pca9685_write(addr, PCA9685_LED0_ON_H  + 4 * channel, on >> 8);
  pca9685_write(addr, PCA9685_LED0_OFF_L + 4 * channel, off & 0xFF);
  pca9685_write(addr, PCA9685_LED0_OFF_H + 4 * channel, off >> 8);
}


//=====================================================================

#define APDS9960_ADDR 0x39

void apds9960_init() {
  byte data[2];

  data[0] = 0x80; data[1] = 0x00;
  i2c_write(APDS9960_ADDR, data, 2); usleep(1000);

  data[0] = 0x8f; data[1] = 0x01;
  i2c_write(APDS9960_ADDR, data, 2); usleep(1000);

  data[0] = 0x80; data[1] = 0x03;
  i2c_write(APDS9960_ADDR, data, 2); usleep(1000);

  data[0] = 0x92; data[1] = 0;
  i2c_write(APDS9960_ADDR, data, 1); usleep(1000);
  i2c_read(APDS9960_ADDR, data, 1); 
  if (data[0] != 0xAB) {
    printf("apds9960 with unknown id: %02X\n", data[0]);
    return;
  }

  // wait for APDS9960 to get ready
  for (int count = 0; count < 100; count++) {
    data[0] = 0x93; data[1] = 0;
    i2c_write(APDS9960_ADDR, data, 1); usleep(1000);
    i2c_read(APDS9960_ADDR, data, 1); 

    if ((data[0] & 0x01) == 0x01) {
      printf("apds9960 ready; status: %02X\n", data[0]);
      return;
    }
  }

  printf("apds9960 status: %02X\n", data[0]);
}
                                                                                   
void apds9960_read(int *value) {
  byte data[8];

  data[0] = 0x80; data[1] = 0; 
  i2c_write(APDS9960_ADDR, data, 1); usleep(1000);
  i2c_read(APDS9960_ADDR, data, 1); 

  if (data[0] != 0x03) {
    printf("initialize apds9960\n");

    data[0] = 0x8f; data[1] = 0x01;
    i2c_write(APDS9960_ADDR, data, 2); usleep(1000);

    data[0] = 0x80; data[1] = 0x03;
    i2c_write(APDS9960_ADDR, data, 2); usleep(1000);
  }

  for (int count = 0; count < 100; count++) {
    data[0] = 0x93; data[1] = 0;
    i2c_write(APDS9960_ADDR, data, 1); usleep(1000);
    i2c_read(APDS9960_ADDR, data, 1); 

    if ((data[0] & 0x80) == 0x80) {
      printf("apds9960 reset; status: %02X\n", data[0]);

      data[0] = 0x80; data[1] = 0x00;
      i2c_write(APDS9960_ADDR, data, 2); usleep(1000);

      data[0] = 0x8f; data[1] = 0x01;
      i2c_write(APDS9960_ADDR, data, 2); usleep(1000);

      data[0] = 0x80; data[1] = 0x03;
      i2c_write(APDS9960_ADDR, data, 2); usleep(1000);
    }

    if ((data[0] & 0x01) == 0x01) break;
  }

  data[0] = 0x94;
  i2c_write(APDS9960_ADDR, data, 1); usleep(1000);
  i2c_read(APDS9960_ADDR, data, 8); 
  value[0] = 256 * data[1] + data[0];
  value[1] = 256 * data[3] + data[2];
  value[2] = 256 * data[5] + data[4];
  value[3] = 256 * data[7] + data[6];

}

//=====================================================================
