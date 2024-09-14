#include  <stdio.h>
#include  <stdbool.h>
#include  <string.h>

#include  "pico/stdlib.h"
#include  "hardware/i2c.h"

#include  "get_bootsel_button.h"

#include  "wiiEC.h"

//+============================================================================
/*
void  waitFor (uint8_t seek)
{
	bool gotEC = false;

	while (!gotEC) {

		printf("\n    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

		for (int addr = 0;  addr < (1 << 7);  addr++) {
			int      ret;
			uint8_t  rxdata;

			if ((addr % 16) == 0)  printf("%02x  ", addr) ;

			if ((addr & 0x78) == 0 || (addr & 0x78) == 0x78)
				printf("x");  // reserved
			else if (i2c_read_blocking(i2c0, addr, &rxdata, 1, false) >= 0)
				printf("@");  // address was ACKed
			else
				printf(".");  // no device found

			printf("%s", ((addr % 16) == 15) ? "\n" : "  ");

			// did we find what we were looking for?
			if ((addr == seek) && (ret >= 0)) gotEC = true ;
		}

		if (!gotCC)  sleep_ms(1000) ;
	}
}
*/

//+============================================================================
// read 'len' bytes starting at register 'base' to memory 'dst'
//
int  i2c_read (i2c_inst_t *i2c,  uint8_t addr,
                uint8_t reg1,  size_t len,  uint8_t *dst)
{
	// write out the base register number
	i2c_write_blocking(i2c, addr, &reg1, 1, false/*nostop*/);

	// wait for the EC chip to update its register pointer
	// 150 is not quite long enough - 200 has been reliable for me
	// ...my flipperzero code actually uses 300!
	//    https://github.com/csBlueChip/FlipperZero_WiiEC/blob/main/wii_i2c.h#L32
	sleep_us(200);

	// read 'len' (8-bit) register values (starting with register 'base')
	// and return the number of bytes read (or negative values for errors)
	return i2c_read_blocking(EC_I2C_DEV, EC_I2C_ADDR, dst, len, false/*nostop*/);
}

//-----------------------------------------------------------------------------
// We build the memory map in to this block
//
#define  I2C_MEM_MAX  (256)
uint8_t  map[I2C_MEM_MAX] = {0};

//+============================================================================
uint8_t*  dumpBlock (uint8_t base,  int len,  bool lf)
{
	assert(base+len <= I2C_MEM_MAX);

	int  ret = i2c_read(EC_I2C_DEV, EC_I2C_ADDR, base, len, map+base);

	// read failed
	if (ret < 0)  return NULL ;

	// dump results
	printf("@ %02X = { ", base);
	for (int i = 0;  i < len;  i++)  printf("%02X ", map[base+i]) ;
	printf("}[%d]%s", ret, lf ? "\n" : "\r");

	return map+base;
}

//++===========================================================================
int  main (void)
{
	// --- start the serial port ---
	stdio_init_all();
	sleep_ms(3000);  // wait for serial monitor
	printf( "\n\n# WiiEC demo\n"
	        "# SDA=%d, SCL=%d, ADDR=0x%02X @ %dKHz\n",
	        EC_I2C_SDA, EC_I2C_SCL, EC_I2C_ADDR, EC_I2C_KHZ );

	// --- initialise the LED ---
	int  led = 0;  // the current state of the LED (1=on)
	gpio_init(PICO_DEFAULT_LED_PIN);
	gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

	// --- initialise the i2c hardware ---
	i2c_init(EC_I2C_DEV, EC_I2C_KHZ * 1000);

	gpio_set_function(EC_I2C_SDA, GPIO_FUNC_I2C);
	gpio_set_function(EC_I2C_SCL, GPIO_FUNC_I2C);

	// https://forums.raspberrypi.com/viewtopic.php?t=305742
	//   The pico pull-ups are weak. Suggest external pull-ups:
	//   "4.7k is a good place to start, but anything {2k .. 10k} should work"
	// If you do not want to use external pull-ups and you are getting bad readings
	//   dropping the i2c bus speed from 400KHz down to 100KHz might help 
	//   ...intermediate values (eg. 250KHz) are out of spec
#	define USING_EXTERNAL_PULLUPS  0  // this should be in a config file
#	if (USING_EXTERNAL_PULLUPS == 0)
		// use internal pull-ups
		gpio_pull_up(EC_I2C_SDA);
		gpio_pull_up(EC_I2C_SCL);
#	endif

	while (true) {
		// --- wait for our device to appear ---
		printf("* Waiting for device...");
		while (true) {
			uint8_t  rx;  // pico does not have an API for address scanning

			gpio_put(PICO_DEFAULT_LED_PIN, (led = !led));  // toggle LED

			// try to read a byte, to make sure the Address was ACKed
			if (i2c_read_blocking(EC_I2C_DEV, EC_I2C_ADDR, &rx, 1, false) >= 0)
				break ;
			(void)rx;  // avoid compiler warning "variable set and not used"

			sleep_ms(1000);  // wait 1 second
		}
		printf("\n# Found device @ 0x%02X\n", EC_I2C_ADDR);

		// --- EC init (no crypto) ---
		uint8_t  init1[] = {EC_INIT_REG1, EC_INIT_DAT1};
		uint8_t  init2[] = {EC_INIT_REG2, EC_INIT_DAT2};

		printf( "# EC init-1 : {%02X=%02X}[%d]\n", init1[0], init1[1],
			i2c_write_blocking(EC_I2C_DEV, EC_I2C_ADDR, init1, sizeof(init1), false) );

		printf( "# EC init-2 : {%02X=%02X}[%d]\n", init2[0], init2[1],
			i2c_write_blocking(EC_I2C_DEV, EC_I2C_ADDR, init2, sizeof(init2), false) );

		// --- get pid ---
		printf("# Perhipheral ID   : ");
		dumpBlock(EC_PID_REG1, EC_PID_LEN, true);

		// --- get calibration data ---
		printf("# Calibration Data : ");
		dumpBlock(EC_CAL_REG1, EC_CAL_LEN, true);

		// Do NOT attempt to access the crypto registers {0x40..0x41}
		// else the device will reset!

		// --- read EC controls ---
		printf("# Poll EC readings\n");
		while (dumpBlock(EC_JOY_REG1, EC_JOY_LEN, false)) {
			// decode algorithms
			// https://github.com/csBlueChip/FlipperZero_WiiEC/blob/main/wii_ec_classic.c#L39
			// https://github.com/csBlueChip/FlipperZero_WiiEC/blob/main/wii_ec_nunchuck.c#L19
			gpio_put(PICO_DEFAULT_LED_PIN, (led = !led)); // toggle LED
			if (get_bootsel_button())  break ;  // check button (for reset)
			sleep_ms(100);  // wait 1/10th second
		}

		printf("\n! Connection Lost\n");
		while (get_bootsel_button()) ;  // wait for button release

	}//while(true)
}
