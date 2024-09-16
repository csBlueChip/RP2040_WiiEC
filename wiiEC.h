#ifndef  WIIEC_H_
#define  WIIEC_H_

//----------------------------------------------------------------------------- ---------------------------------------
// i2c IO [editable]
//
#define  EC_I2C_DEV     i2c1    // device name     {i2c0, i2c1}
#define  EC_I2C_SDA     (18)    // GPIO #          {see RP2040 pinout}
#define  EC_I2C_SCL     (19)    // ...             {see RP2040 pinout}
#define  EC_I2C_KHZ     (400)   // i2c clock speed {100, 400}

// https://forums.raspberrypi.com/viewtopic.php?t=305742
//   TL;DR: The pico pull-ups are weak. Suggest external pull-ups:
//   "4.7k is a good place to start, but anything {2k .. 10k} should work"
// If you do not want to use external pull-ups and you are getting bad readings
//   dropping the i2c bus speed from 400KHz down to 100KHz might help 
//   ...intermediate values (eg. 250KHz) are out of spec
#define  EC_I2C_EXTPU   0       // set to '1' if you are using External Pullups

//----------------------------------------------------------------------------- ---------------------------------------
// Extension Controller registers [do not edit]
//
// i2c address
#define  EC_I2C_ADDR    (0x52)

// We do not handle encrypted comms - we bypass it!
// https://github.com/csBlueChip/FlipperZero_WiiEC/blob/main/wii_i2c.c#L235
#define  EC_ENC_BASE    (0x40)
#define  EC_ENC_LEN     (2)

// Initialise with encryption bypass
#define  EC_INIT_REG1   (0xF0)
#define  EC_INIT_DAT1   (0x55)

#define  EC_INIT_REG2   (0xFB)
#define  EC_INIT_DAT2   (0x00)

// Perhipheral ID
#define  EC_PID_BASE    (0xFA)
#define  EC_PID_LEN     (6)

// Joystick/gamepad/controller readings
#define  EC_JOY_BASE    (0x00)
#define  EC_JOY_LEN     (6)

// Calibration data
#define  EC_CAL_BASE    (0x20)
#define  EC_CAL_LEN     (16)

#endif //WIIEC_H_
