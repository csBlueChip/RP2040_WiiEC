#ifndef  WIIEC_H_
#define  WIIEC_H_

//----------------------------------------------------------------------------- ---------------------------------------
// i2c IO [editable]
//
#define  EC_I2C_DEV    i2c1    // device name     {i2c0, i2c1}
#define  EC_I2C_SDA    (18)    // GPIO #          {see RP2040 pinout}
#define  EC_I2C_SCL    (19)    // ...             {see RP2040 pinout}
#define  EC_I2C_KHZ    (400)   // i2c clock speed {100, 400}

//----------------------------------------------------------------------------- ---------------------------------------
// Extension Controller details
//
// i2c address
#define  EC_I2C_ADDR   (0x52)

// We do not handle encrypted comms - we bypass it!
#define  EC_ENC_REG1   (0x40)
#define  EC_ENC_LEN    (2)

// Init with encryption DISabled
#define  EC_INIT_REG1  (0xF0)
#define  EC_INIT_DAT1  (0x55)

#define  EC_INIT_REG2  (0xFB)
#define  EC_INIT_DAT2  (0x00)

// Perhipheral ID
#define  EC_PID_REG1   (0xFA)
#define  EC_PID_LEN    (6)

// Joystick/gamepad readings
#define  EC_JOY_REG1   (0x00)
#define  EC_JOY_LEN    (6)

// Calibration data
#define  EC_CAL_REG1   (0x20)
#define  EC_CAL_LEN    (16)

#endif //WIIEC_H_
