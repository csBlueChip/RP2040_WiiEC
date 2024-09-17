#include  <stdio.h>
#include  <stdbool.h>
#include  <string.h>

#include  "pico/stdlib.h"
#include  "hardware/i2c.h"

#include  "get_bootsel_button.h"

#include  "wiiEC.h"


//+============================================================================
// debug: poll the entire i2c bus, 
// return when the specified address starts responding
//
#if 0
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
				printf(".");  // address NOT ACKed

			printf("%s", ((addr % 16) == 15) ? "\n" : "  ");

			// did we find what we were looking for?
			if ((addr == seek) && (ret >= 0)) gotEC = true ;
		}

		if (!gotCC)  sleep_ms(1000) ;
	}
}
#endif

//-----------------------------------------------------------------------------
// Das blinken lights - for debugging without a console
//
#define   LED_PIN  PICO_DEFAULT_LED_PIN

int  led = 0;  // initial state:  {0=Off, 1=On}

//+====================================
static inline
void  ledInit (void)
{
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
	gpio_put(LED_PIN, led);
}

//+====================================
static inline
void  ledToggle (void)
{
	gpio_put(LED_PIN, (led = !led));
}

//-----------------------------------------------------------------------------
// Convert PID to controller name
//
struct {
	uint8_t      pid[6];
	const char*  name;
} known[] = {
	{ {0x00, 0x00, 0xA4, 0x20, 0x00, 0x00}, "Nunchuck"                     },
	{ {0xFF, 0x00, 0xA4, 0x20, 0x00, 0x00}, "Nunchuck (rev2)"              },
	{ {0x00, 0x00, 0xA4, 0x20, 0x01, 0x01}, "Classic Controller"           },
	{ {0x01, 0x00, 0xA4, 0x20, 0x01, 0x01}, "Classic Controller Pro"       },
	{ {0x00, 0x00, 0xA4, 0x20, 0x04, 0x02}, "Balance Board"                },
	{ {0x00, 0x00, 0xA4, 0x20, 0x01, 0x03}, "Guitar Hero Guitar"           },
	{ {0x01, 0x00, 0xA4, 0x20, 0x01, 0x03}, "Guitar Hero World Tour Drums" },
	{ {0x03, 0x00, 0xA4, 0x20, 0x01, 0x03}, "DJ Hero Turntable"            },
	{ {0x00, 0x00, 0xA4, 0x20, 0x01, 0x11}, "Taiko Drum Controller)"       },
	{ {0xFF, 0x00, 0xA4, 0x20, 0x00, 0x13}, "uDraw Tablet"                 },
	{ {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, "-unknown-"                    }  // LAST!
};

//+====================================
const char*  identify (uint8_t* pid)
{
	const char*  this;
	for (int  i = 0;  i < sizeof(known) / sizeof(*known);  i++) {
		this = known[i].name;
		if (memcmp(known[i].pid, pid, sizeof(known[i].pid)) == 0)  break ;
	}
	return this;
}

//+============================================================================
void  dump (int base,  int max,  uint8_t* data,  int len)
{

	printf("@ %02X = { ", base);
	for (int i = 0;  i < max;  i++) {
		if (i < len)  printf("%02X ", data[i]) ;
		else          printf("-- ") ;
	}
	printf("}[%d]", len);
}

//+============================================================================
// read 'len' bytes starting at register 'base' to memory 'dst'
//
#define  I2C_MEM_MAX  (256)

int  i2c_readBytes ( i2c_inst_t *i2c,  uint8_t addr,
                     uint8_t base,  size_t len,  uint8_t *dst )
{
	assert(base+len <= I2C_MEM_MAX);

	int result;

	// write out the base register number
	result = i2c_write_blocking(i2c, addr, &base, 1, false/*nostop*/);  // !nostop==stop
	if (result != 1)  return PICO_ERROR_GENERIC ;

	// wait for the EC chip to update its register pointer
	// 150uS is not quite long enough - 200uS has been reliable for me
	// ...my flipperzero code actually uses 300uS !
	//  https://github.com/csBlueChip/FlipperZero_WiiEC/blob/main/wii_i2c.h#L32
	sleep_us(200);

	// read 'len' (8-bit) register values (starting with register 'base')
	// and return the number of bytes read (negative values are errors)
	return i2c_read_blocking(EC_I2C_DEV, EC_I2C_ADDR, dst, len, false/*nostop*/);
}

//++===========================================================================
int  main (void)
{
	// This are all the (known) EC user fields
	uint8_t  enc[EC_ENC_LEN] = {0};  //  w : Encryption keys [not used]
	uint8_t  pid[EC_PID_LEN] = {0};  // r  : Perhipheral ID
	uint8_t  cal[EC_CAL_LEN] = {0};  // r  : Calibration data
	uint8_t  joy[EC_JOY_LEN] = {0};  // r  : Joystick/Controlle data

	int      result          = 0;    // general usage


	// --- Start the serial port ---
	// ...only used here to send debug readings back to the console
	stdio_init_all();  // https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
	sleep_ms(3000);    // give the serial monitor time to attach
	printf( "\n\n# WiiEC demo\n"
	        "# SDA=%d, SCL=%d, ADDR=0x%02X @ %dKHz\n",
	        EC_I2C_SDA, EC_I2C_SCL, EC_I2C_ADDR, EC_I2C_KHZ );


	// --- Setup Das Blinken Lights ---
	// ...only used here for visual debug (sans console)
	// ...Slow (1s) flash: seeking;  Fast (0.1s) flash: reading
	ledInit();


	// --- Initialise the i2c hardware ---
	i2c_init(EC_I2C_DEV, EC_I2C_KHZ * 1000);       // NOT *1024, this is radio!

	gpio_set_function(EC_I2C_SDA, GPIO_FUNC_I2C);
	gpio_set_function(EC_I2C_SCL, GPIO_FUNC_I2C);

#	if (EC_I2C_EXTPU != 1)                         // qv. wiiEC.h
		// use internal pull-ups
		gpio_pull_up(EC_I2C_SDA);
		gpio_pull_up(EC_I2C_SCL);
#	endif


	// --- Hardware initialised - game on ---
	while (true) {

		// --- Poll the i2c bus until a device responds on address EC_I2C_ADDR ---
		printf("* Waiting for device...");
		result = -999;
		while (result < 0) {
			// pico does not have an API for address scanning
			// so we will issue a read for one byte,
			// just to check if the address was ACKed
			uint8_t  rx;
			(void)rx;  // avoid compiler warning "variable set and not used"

			ledToggle();                           // das blinken lights
			if (result != -999)  sleep_ms(1000) ;  // do not cook the RP2040

			// result = number of bytes read, or < 0 for an error
			result = i2c_read_blocking(EC_I2C_DEV, EC_I2C_ADDR, &rx, 1, false/*nostop*/);
		}
		printf("\n# Found device @ 0x%02X\n", EC_I2C_ADDR);


		// --- Initialise in crypto bypass mode ---
		// More details on the crypto here:
		// https://github.com/csBlueChip/FlipperZero_WiiEC/blob/main/wii_i2c.c#L235
		// I don't normally declare variables inline, but it seems helpful here
		uint8_t  init1[] = {EC_INIT_REG1, EC_INIT_DAT1};
		uint8_t  init2[] = {EC_INIT_REG2, EC_INIT_DAT2};

		// The sleep instructions are NOT actually required in this example code
		//   because the printf() causes a sufficient deley
		// They are included here to highlight that a (short) delay IS required after
		//   each of the two init commands
		result = i2c_write_blocking(EC_I2C_DEV, EC_I2C_ADDR, init1, sizeof(init1), false);
		sleep_us(20);  // give the EC time to configure itself
		printf( "# EC init-1 : {%02X=%02X}[%d]\n", init1[0], init1[1], result);
		if (result != sizeof(init1))  goto lost ;  // Issue #1: goto's scare me ;)

		result = i2c_write_blocking(EC_I2C_DEV, EC_I2C_ADDR, init2, sizeof(init2), false);
		sleep_us(20);  // give the EC time to configure itself
		printf( "# EC init-2 : {%02X=%02X}[%d]\n", init2[0], init2[1], result);
		if (result != sizeof(init2))  goto lost ;


		// Do NOT attempt to access the (standard) crypto registers {0x40..0x41}
		//   else the device will reset!
		(void)enc;  // avoid compiler warning "variable defined and not used"


		// --- Get pid ---
		printf("# Perhipheral ID   : ");
		result = i2c_readBytes(EC_I2C_DEV, EC_I2C_ADDR, EC_PID_BASE, EC_PID_LEN, pid);
		dump(EC_PID_BASE, EC_PID_LEN, pid, result);
		if (result != EC_PID_LEN)  goto lost ;
		printf(" - %s\n", identify(pid));


		// --- Get calibration data ---
		printf("# Calibration Data : ");
		result = i2c_readBytes(EC_I2C_DEV, EC_I2C_ADDR, EC_CAL_BASE, EC_CAL_LEN, cal);
		dump(EC_CAL_BASE, EC_CAL_LEN, cal, result);
		if (result != EC_CAL_LEN)  goto lost ;
		printf("\n");


		// --- Read EC controls ---
		printf("# Poll EC readings\n");
		while (true) {
			result = i2c_readBytes(EC_I2C_DEV, EC_I2C_ADDR, EC_JOY_BASE, EC_JOY_LEN, joy);
			dump(EC_JOY_BASE, EC_JOY_LEN, joy, result);
			if (result != EC_JOY_LEN)  goto lost ;
			printf("\r");

			// decode(joy, controllerType);
			// https://github.com/csBlueChip/FlipperZero_WiiEC/blob/main/wii_ec_classic.c#L39
			// https://github.com/csBlueChip/FlipperZero_WiiEC/blob/main/wii_ec_nunchuck.c#L19

			if (get_bootsel_button())  break ;  // check button (for reset)
			ledToggle();                        // das blinken lights
			sleep_ms(100);                      // don't cook the RP2040
		}

lost:
		printf("\n! Connection Lost\n");
		while (get_bootsel_button()) ;  // wait for button release

	}//while(true)
}
