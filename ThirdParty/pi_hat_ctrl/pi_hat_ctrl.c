#include <stdlib.h>
#include <wiringPiI2C.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// ADRESSES
#define IS31FL3193_ADR 0x68
#define PCAL6416A_ADR 0x20
#define TLV320DAC3101_ADR 0x18

// PCAL6416A registers

#define PCAL6416A_input_port_reg_pair 0x00
#define PCAL6416A_output_port_reg2 0x03
#define PCAL6416A_config_reg 0x06
#define PCAL6416A_config_reg2 0x07
#define PCAL6416A_int_mask_port0_reg 0x4A

// TLV320DAC3101 registers

// Page 0
#define TLV320DAC3101_pg_ctrl_reg 0x00 // Register 0 - Page Control
#define TLV320DAC3101_sw_rst_reg 0x01 // Register 1 - Sofware Reset
#define TLV320DAC3101_clk_gen_mux_reg 0x04 // Register 4 - Clock-Gen Muxing
#define TLV320DAC3101_PPL_P_R_reg 0x05 // Register 5 - PLL P and R Values
#define TLV320DAC3101_PPL_J_reg 0x06 // Register 6 - PLL J Value
#define TLV320DAC3101_PPL_D_msb_reg 0x07 // Register 7 - PLL D Value (MSB)
#define TLV320DAC3101_PPL_D_lsb_reg 0x08 // Register 8 - PLL D Value (LSB)
#define TLV320DAC3101_ndac_val_reg 0x0B // Register 11 - NDAC Divider Value
#define TLV320DAC3101_mdac_val_reg 0x0C // Register 12 - MDAC Divider VALUE
#define TLV320DAC3101_dosr_val_lsb_reg 0x0E // Register 14 - DOSR Divider Value (LS Byte)
#define TLV320DAC3101_clkout_mux_reg 0x19 // Register 25 - CLKOUT MUX
#define TLV320DAC3101_clkout_m_val_reg 0x1A // Register 26 - CLKOUT M_VAL
#define TLV320DAC3101_codec_if_reg 0x1B // Register 27 - CODEC Interface Control
#define TLV320DAC3101_dac_dat_path_reg 0x3F // Register 63 - DAC Data Path Setup
#define TLV320DAC3101_dac_vol_reg 0x40 // Register 64 - DAC Vol Control
#define TLV320DAC3101_dacl_vol_d_reg 0x41 // Register 65 - DAC Left Digital Vol Control
#define TLV320DAC3101_dacr_vol_d_reg 0x42 // Register 66 - DAC Right Digital Vol Control
#define TLV320DAC3101_gpio1_io_reg 0x33 // Register 51 - GPIO1 In/Out Pin Control
// Page 1
#define TLV320DAC3101_hp_drvr_reg 0x1F // Register 31 - Headphone Drivers
#define TLV320DAC3101_spk_amp_reg 0x20 // Register 32 - Class-D Speaker Amp
#define TLV320DAC3101_hp_depop_reg 0x21 // Register 33 - Headphone Driver De-pop
#define TLV320DAC3101_dac_op_mix_reg 0x23 // Register 35 - DAC_L and DAC_R Output Mixer Routing
#define TLV320DAC3101_hpl_vol_a_reg 0x24 // Register 36 - Analog Volume to HPL
#define TLV320DAC3101_hpr_vol_a_reg 0x25 // Register 37 - Analog Volume to HPR
#define TLV320DAC3101_spkl_vol_a_reg 0x26 // Register 38 - Analog Volume to left speaker
#define TLV320DAC3101_spkr_vol_a_reg 0x27 // Register 39 - Analog Volume to right speaker
#define TLV320DAC3101_hpl_drvr_reg 0x28 // Register 40 - Headphone left driver
#define TLV320DAC3101_hpr_drvr_reg 0x29 // Register 41 - Headphone right driver
#define TLV320DAC3101_spkl_drvr_reg 0x2A // Register 42 - Left class-D speaker driver
#define TLV320DAC3101_spkr_drvr_reg 0x2B // Register 43 - Right class-D speaker driver


const char *command_SET_LED_RGB = "SET_LED_RGB";
const char *command_SET_LED_HSV = "SET_LED_HSV";
const char *command_SET_LED_HSL = "SET_LED_HSL";
const char *command_SET_MUTE_MIC = "SET_MUTE_MIC";
const char *command_SET_DAC_RESET = "SET_DAC_RESET";
const char *command_INIT_DAC = "INIT_DAC";
const char *command_GET_BUT_MUTE = "GET_BUT_MUTE";
const char *command_GET_BUT_VOL_UP = "GET_BUT_VOL_UP";
const char *command_GET_BUT_VOL_DN = "GET_BUT_VOL_DN";
const char *command_GET_BUT_ACTION = "GET_BUT_ACTION";
const char *command_SET_BOOT_SEL = "SET_BOOT_SEL";
const char *command_SET_INT_INPUT = "SET_INT_INPUT";
const char *command_GET_INT_N_IN = "GET_INT_N_IN";
const char *command_SET_LED_SPEAKING = "SET_LED_SPEAKING";


typedef struct {
	double hue;         // angle in degrees
	double saturation;  // a fraction between 0 and 1
	double value;       // a fraction between 0 and 1
} hsv;

typedef struct {
	double red;        // a fraction between 0 and 1
	double green;      // a fraction between 0 and 1
	double blue;       // a fraction between 0 and 1
} rgb;

typedef struct {
	double hue;         // angle in degrees
	double saturation;  // a fraction between 0 and 1
	double lightness;   // a fraction between 0 and 1
} hsl;


void sleep_ms(unsigned milliseconds);
void set_led_rgb (char **argv);
rgb hsv2rgb(hsv input);
void set_led_hsv (hsv input);
void parse_led_hsv (char **argv);
rgb hsl2rgb(hsl input);
void set_led_hsl (hsl input);
void parse_led_hsl (char **argv);
void set_mute_mic(char **argv);
void set_dac_reset(char **argv);
void init_dac();
int get_button_mute();
int get_button_vol_dwn();
int get_button_vol_up();
int get_button_action();
int boot_sel(char **argv);
int int_input(char **argv);
int get_int_n_in();
void set_led_speaking();


void sleep_ms(unsigned milliseconds){
	usleep(milliseconds*1000);
}


void set_led_rgb (char **argv){
	int fd = wiringPiI2CSetup(IS31FL3193_ADR);
    // Set Shutdown Register to normal operatiom // All channel enable
    wiringPiI2CWriteReg8(fd, 0x00, 0x20);
    // Set current Setting Register 0x03 to its minimum value (5 mA)
    wiringPiI2CWriteReg8(fd, 0x03, 0x10);
    // Set PWM register (OUT1-OUT3)
    wiringPiI2CWriteReg8(fd, 0x04, (int)round((float)atof(argv[2]))); // Red
    wiringPiI2CWriteReg8(fd, 0x05, (int)round((float)atof(argv[3]))); // Green
    wiringPiI2CWriteReg8(fd, 0x06, (int)round((float)atof(argv[4]))); // Blue

    wiringPiI2CWriteReg8(fd, 0x07, 0x00);
}


rgb hsv2rgb(hsv input){
	double m, C, X, h_div;
    rgb out;
    int h_div_int;

	h_div = input.hue/60.0;
	h_div_int = input.hue/60.0;
    int g = (int)h_div/2;

    m = input.value * (1.0 - input.saturation);
    C = input.value * input.saturation;
    X = C*(1.0 -fabs(h_div-2*g-1));
  	switch(h_div_int) {
		case 0:
			out.red = C; out.green = X; out.blue = 0.0;
			break;
		case 1:
	    	out.red = X; out.green = C; out.blue = 0.0;
	    	break;
		case 2:
	    	out.red = 0.0; out.green = C; out.blue = X;
	    	break;
		case 3:
	    	out.red = 0.0; out.green = X; out.blue = C;
	    	break;
		case 4:
	    	out.red = X; out.green = 0.0; out.blue = C;
	    	break;
		case 5:
	    	out.red = C; out.green = 0.0; out.blue = X;
	   		break;
	    case 6:
			out.red = C; out.green = 0.0; out.blue = X;
	    	break;
	}
   	out.red = out.red + m;
	out.green = out.green + m;
	out.blue = out.blue + m;

    return out;
}


void set_led_hsv (hsv input){
	rgb rgb_res = hsv2rgb(input);
	int fd = wiringPiI2CSetup(IS31FL3193_ADR);

	// Set Shutdown Register to normal operatiom // All channel enable
    wiringPiI2CWriteReg8(fd, 0x00, 0x20);

	// Set current Setting Register 0x03 to its minimum value (5 mA)
    wiringPiI2CWriteReg8(fd, 0x03, 0x10);

	// Conversion from HSV to RGB
	// Set PWM register (OUT1-OUT3)
    wiringPiI2CWriteReg8(fd, 0x04, (int)round((rgb_res.red)*255)); // Red
    wiringPiI2CWriteReg8(fd, 0x05, (int)round((rgb_res.green)*255)); // Green
    wiringPiI2CWriteReg8(fd, 0x06, (int)round((rgb_res.blue)*255)); // Blue
    printf("%d \n", (int)round((rgb_res.red)*255));
	printf("%d \n", (int)round((rgb_res.green)*255));
	printf("%d \n", (int)round((rgb_res.blue)*255));

    // Update the register
    wiringPiI2CWriteReg8(fd, 0x07, 0x00);
}


void parse_led_hsv (char **argv){
	// H (argv[2]) between 0 and 360, S (argv[3]) between 0 and 1, V (argv[4]) between 0 and 1
	hsv input;
	input.hue =  strtod(argv[2], NULL);
	input.saturation =  strtod(argv[3], NULL);
	input.value =  strtod(argv[4], NULL);
	set_led_hsv(input);
}


rgb hsl2rgb(hsl input){
	double m, C, X, h_div;
    rgb out;
    int h_div_int;

	h_div = input.hue/60.0;
    h_div_int = input.hue/60.0;
    int g = (int)h_div/2;
    C = (1-fabs(2*input.lightness-1))*input.saturation;
    m = input.lightness-C/2;
    X = C*(1.0 -fabs(h_div-2*g-1));
  	switch(h_div_int) {
		case 0:
        	out.red = C; out.green = X; out.blue = 0.0;
        	break;
    	case 1:
        	out.red = X; out.green = C; out.blue = 0.0;
        	break;
    	case 2:
        	out.red = 0.0; out.green = C; out.blue = X;
        	break;
    	case 3:
        	out.red = 0.0; out.green = X; out.blue = C;
        	break;
    	case 4:
        	out.red = X; out.green = 0.0; out.blue = C;
        	break;
    	case 5:
        	out.red = C; out.green = 0.0; out.blue = X;
        	break;
        case 6:
        	out.red = C; out.green = 0.0; out.blue = X;
        	break;
   	}

   	out.red = out.red + m;
	out.green = out.green + m;
	out.blue = out.blue + m;

    return out;
}


void set_led_hsl (hsl input){
	rgb rgb_res = hsl2rgb(input);
	int fd = wiringPiI2CSetup(IS31FL3193_ADR);

	// Set Shutdown Register to normal operatiom // All channel enable
    wiringPiI2CWriteReg8(fd, 0x00, 0x20);

	// Set current Setting Register 0x03 to its minimum value (5 mA)
    wiringPiI2CWriteReg8(fd, 0x03, 0x10);

	// Set PWM register (OUT1-OUT3)
    wiringPiI2CWriteReg8(fd, 0x04, (int)round((rgb_res.red)*255)); // Red
    wiringPiI2CWriteReg8(fd, 0x05, (int)round((rgb_res.green)*255)); // Green
    wiringPiI2CWriteReg8(fd, 0x06, (int)round((rgb_res.blue)*255)); // Blue
	printf("%d \n", (int)round((rgb_res.red)*255));
	printf("%d \n", (int)round((rgb_res.green)*255));
	printf("%d \n", (int)round((rgb_res.blue)*255));
    // Update the register
    wiringPiI2CWriteReg8(fd, 0x07, 0x00);
}


void parse_led_hsl (char **argv){
	// Conversion from HSV to RGB
	hsl input;
	input.hue =  strtod(argv[2], NULL);
	input.saturation =  strtod(argv[3], NULL);
	input.lightness =  strtod(argv[4], NULL);
	set_led_hsl(input);
}


// 1 - Mute the mic, 0 - Unmute the mic
void set_mute_mic(char **argv){
	// Setup I2C
	int fd = wiringPiI2CSetup(PCAL6416A_ADR);
    int read_config_reg = wiringPiI2CReadReg8(fd, PCAL6416A_config_reg);
    if (atoi(argv[2]) == 1) { // MUTE
		read_config_reg &= 0b11101111;
	}
	else if (atoi(argv[2]) == 0) {
		read_config_reg |= 0b00010000;
	}
	wiringPiI2CWriteReg8(fd, PCAL6416A_config_reg, read_config_reg);
}


// Reset the DAC with default software values + Power up the DAC
void set_dac_reset(char **argv){
	// Setup I2C
	int fd = wiringPiI2CSetup(PCAL6416A_ADR);
	int fd_TLV320DAC3101 = wiringPiI2CSetup(TLV320DAC3101_ADR);
    int read_config_reg = wiringPiI2CReadReg8(fd, PCAL6416A_config_reg);
    if (atoi(argv[2]) == 1) { // RESET DAC with default value
		//read_config_reg |= 0b10000000; // PIN DAC_RST_N defined as an input
		read_config_reg &= 0b01111111; // PIN DAC_RST_N defined as an output
		wiringPiI2CWriteReg8(fd, PCAL6416A_config_reg, read_config_reg);
		wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_pg_ctrl_reg, 0x00); // page 0 selected
		wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_sw_rst_reg, 0x01); // self-clearing software reset for control register
		// POWER UP DAC
		wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_dac_dat_path_reg, 0xD4); // powerup DAC left and right channels (soft step enabled)
		wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_dacl_vol_d_reg, 0x00); // DAC left gain
		wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_dacr_vol_d_reg, 0x00); // DAC right gain
		wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_dac_vol_reg, 0x00); // unmute DAC left and right channels
		printf("DAC turned on (self-clearing software reset for control register and power up)\n");
	}
	else if (atoi(argv[2]) == 0) { // TURN OFF the DAC
		read_config_reg |= 0b10000000; // DAC turned off
		printf("DAC turned off \n");
	}
}


// DAC initialisation + power up
void init_dac(){
	// Setup I2C
	int fd_PCAL6416A = wiringPiI2CSetup(PCAL6416A_ADR);
	int fd_TLV320DAC3101 = wiringPiI2CSetup(TLV320DAC3101_ADR);
    int read_config_reg = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_config_reg); // read config register of PCAL6416A
	read_config_reg |= 0b10000000; // PIN DAC_RST_N defined as an input
	read_config_reg &= 0b01111111; // PIN DAC_RST_N defined as an output
	// TIME SLEEP (0.1 sec)
	wiringPiI2CWriteReg8(fd_PCAL6416A, PCAL6416A_config_reg, read_config_reg); // DAC turned on
	// TIME SLEEP (0.1 sec)
	// TIME SLEEP (1 sec)
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_pg_ctrl_reg, 0x00); // page 0 selected
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_sw_rst_reg, 0x01); // Initiate SW reset (PLL is powered off as part of reset)

	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_PPL_J_reg, 0x08); // Set PLL J Value to 7
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_PPL_D_msb_reg, 0x00); // Set PLL D MSB Value to 0x00
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_PPL_D_lsb_reg, 0x00); // Set PLL D LSB Value to 0x00
	// TIME SLEEP (0.001 sec)
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_clk_gen_mux_reg, 0x07); // Set PLL_CLKIN = BCLK (device pin), CODEC_CLKIN = PLL_CLK (generated on chip)
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_PPL_P_R_reg, 0x94); // Set PLL P and R values and power up.
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_ndac_val_reg, 0x84); // Set NDAC clock divider to 4 and power up.
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_mdac_val_reg, 0x84); // Set MDAC clock divider to 4 and power up.
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_dosr_val_lsb_reg, 0x80); // Set OSR clock divider to 128.
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_clkout_mux_reg, 0x04); // Set CLKOUT Mux to DAC_CLK
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_clkout_m_val_reg, 0x81); // Set CLKOUT M divider to 1 and power up.
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_gpio1_io_reg, 0x10); // Set GPIO1 output to come from CLKOUT output.
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_codec_if_reg, 0x20); // Set CODEC interface mode: I2S, 24 bit, slave mode (BCLK, WCLK both inputs).
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_pg_ctrl_reg, 0x01); // Set register page to 1
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_hp_drvr_reg, 0x14); // Program common-mode voltage to mid scale 1.65V
	// Program headphone-specific depop settings.
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_hp_depop_reg, 0x4E); // De-pop, Power on = 800 ms, Step time = 4 ms
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_dac_op_mix_reg, 0x44); // LDAC routed to left channel mixer amp, RDAC routed to right channel mixer amp
	// Unmute and set gain of output driver
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_hpl_drvr_reg, 0x06); // Unmute HPL, set gain = 0 db
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_hpr_drvr_reg, 0x06); // Unmute HPR, set gain = 0 db
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_spkl_drvr_reg, 0x0C); // Unmute Left Class-D, set gain = 12 dB
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_spkr_drvr_reg, 0x0C); // Unmute Right Class-D, set gain = 12 dB
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_hp_drvr_reg, 0xD4); // HPL and HPR powered up
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_spk_amp_reg, 0xC6); // Power-up L and R Class-D drivers
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_hpl_vol_a_reg, 0x92); // Enable HPL output analog volume, set = -9 dB
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_hpr_vol_a_reg, 0x92); // Enable HPR output analog volume, set = -9 dB
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_spkl_vol_a_reg, 0x92); // Enable Left Class-D output analog volume, set = -9 dB
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_spkr_vol_a_reg, 0x92); // Enable Right Class-D output analog volume, set = -9 dB
	// TIME SLEEP (0.1 sec)
	// Power up DAC
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_pg_ctrl_reg, 0x00); // Set register page to 0
	// Power up DAC channels and set digital gain
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_dac_dat_path_reg, 0xD4); // Powerup DAC left and right channels (soft step enabled)
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_dacl_vol_d_reg, 0x00); // DAC Left gain = 0dB
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_dacr_vol_d_reg, 0x00); // DAC Right gain = 0dB
	wiringPiI2CWriteReg8(fd_TLV320DAC3101, TLV320DAC3101_dac_vol_reg, 0x00); // Unmute DAC left and right channels

	printf("DAC init. done \n");
}


// Get the BUT_MUTE value (0 = pushed, 1 = not pushed)
int get_button_mute(){
	// Setup I2C
    int fd_PCAL6416A = wiringPiI2CSetup(PCAL6416A_ADR);
    int read_input_port_reg_pair = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_input_port_reg_pair);
	if (read_input_port_reg_pair % 2 == 0) {
		printf("0 \n");
		return 0;
	}
	else if (read_input_port_reg_pair % 2 == 1) {
		printf("1 \n");
		return 1;
	}
}


// Get the BUT_VOL_DN value (0 = pushed, 1 = not pushed)
int get_button_vol_dwn(){
	// Setup I2C
    int fd_PCAL6416A = wiringPiI2CSetup(PCAL6416A_ADR);
    int read_input_port_reg_pair = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_input_port_reg_pair);
	read_input_port_reg_pair >>= 1;
	if (read_input_port_reg_pair % 2 == 0) {
		printf("0 \n");
		return 0;
	}
	else if (read_input_port_reg_pair % 2 == 1) {
		printf("1 \n");
		return 1;
	}
}


// Get the BUT_VOL_UP value (0 = pushed, 1 = not pushed)
int get_button_vol_up(){
	// Setup I2C
    int fd_PCAL6416A = wiringPiI2CSetup(PCAL6416A_ADR);
    int read_input_port_reg_pair = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_input_port_reg_pair);
	read_input_port_reg_pair >>= 3;
	if (read_input_port_reg_pair % 2 == 0) {
		printf("0 \n");
		return 0;
	}
	else if (read_input_port_reg_pair % 2 == 1) {
		printf("1 \n");
		return 1;
	}
}


// Get the BUT_ACTION value (0 = pushed, 1 = not pushed)
int get_button_action(){
	// Setup I2C
    int fd_PCAL6416A = wiringPiI2CSetup(PCAL6416A_ADR);
    int read_input_port_reg_pair = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_input_port_reg_pair);
	read_input_port_reg_pair >>= 2;
	if (read_input_port_reg_pair % 2 == 0) {
		printf("0 \n");
		return 0;
	}
	else if (read_input_port_reg_pair % 2 == 1) {
		printf("1 \n");
		return 1;
	}
}


// Set boot sel as an output, 1 - PIN up, 0 PIN ground
int boot_sel(char **argv){
	// Setup I2C
	int T;
	int read_output_port_reg;
    int fd_PCAL6416A = wiringPiI2CSetup(PCAL6416A_ADR);
    // PIN P1_0 as an output
    int read_config_reg2 = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_config_reg2); // read 2nd config register of PCAL6416A
	read_config_reg2 &= 0b11111110; // PIN DAC_RST_N defined as an output
	wiringPiI2CWriteReg8(fd_PCAL6416A, PCAL6416A_config_reg2, read_config_reg2); // PIN P1_0 as an output
	if (atoi(argv[2]) == 1) {
		read_output_port_reg = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_output_port_reg2); // read config register of PCAL6416A
		read_output_port_reg |= 0b00000001;
		T = wiringPiI2CWriteReg8(fd_PCAL6416A, PCAL6416A_output_port_reg2, read_output_port_reg); // PIN P1_0 up logic level
		printf("Set BOOT_Sel pin (level up) \n");
		printf("%d \n", read_output_port_reg);
		return 1;
	}
	else if (atoi(argv[2]) == 0) {
		read_output_port_reg = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_output_port_reg2); // read config register of PCAL6416A
		read_output_port_reg &= 0b11111110;
		T = wiringPiI2CWriteReg8(fd_PCAL6416A, PCAL6416A_output_port_reg2, read_output_port_reg); // PIN P1_0 up logic level
		printf("Set BOOT_Sel pin (level down) \n");
		printf("%d \n", read_output_port_reg);
		return 0;
	}
}


// 1 - Enable (set) interrupts for PIN P0_0, P0_1, P0_2, P0_3, P0_4, P0_6 defined as inputs, 0 - disable (set) interrupts for PIN P0_0, P0_1, P0_2, P0_3, P0_4, P0_6 defined as inputs
int int_input(char **argv){
	// Setup I2C
	int T;
	int read_int_mask_port0_reg;
    int fd_PCAL6416A = wiringPiI2CSetup(PCAL6416A_ADR);
    // PIN P1_0 as an output
    int read_config_reg = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_config_reg); // read first config register of PCAL6416A
	read_config_reg |= 0b01011111; // Calculation to put PIN P0_0, P0_1, P0_2, P0_3, P0_4, P0_6  as inputs
	wiringPiI2CWriteReg8(fd_PCAL6416A, PCAL6416A_config_reg, read_config_reg); // Write in config register to put PIN P0_0, P0_1, P0_2, P0_3, P0_4, P0_6 as inputs
	if (atoi(argv[2]) == 1) { // Enable (set) interrupts for PIN P0_0, P0_1, P0_2, P0_3, P0_4, P0_6 defined as inputs
		read_int_mask_port0_reg = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_int_mask_port0_reg); // read config register of PCAL6416A
		read_int_mask_port0_reg &= 0b10100000;
		T = wiringPiI2CWriteReg8(fd_PCAL6416A, PCAL6416A_int_mask_port0_reg, read_int_mask_port0_reg); // PIN P1_0 up logic level
		printf("Enable interrupts for PIN P0_0, P0_1, P0_2, P0_3, P0_4, P0_6 defined as inputs \n");
		printf("%d \n", read_int_mask_port0_reg);
		return 1;
	}
	else if (atoi(argv[2]) == 0) {
		read_int_mask_port0_reg = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_output_port_reg2); // read config register of PCAL6416A
		read_int_mask_port0_reg |= 0b01011111;
		T = wiringPiI2CWriteReg8(fd_PCAL6416A, PCAL6416A_int_mask_port0_reg, read_int_mask_port0_reg); // PIN P1_0 up logic level
		printf("Disable interrupts for PIN P0_0, P0_1, P0_2, P0_3, P0_4, P0_6 defined as inputs \n");
		printf("%d \n", read_int_mask_port0_reg);
		return 0;
	}
}


// Get the value of pin P0_6 (name : INT_N_IN) on I2C expander defined as an input.
int get_int_n_in(){
	// Setup I2C
    int fd_PCAL6416A = wiringPiI2CSetup(PCAL6416A_ADR);
    int read_input_port_reg_pair = wiringPiI2CReadReg8(fd_PCAL6416A, PCAL6416A_input_port_reg_pair);
	read_input_port_reg_pair >>= 6;
	if (read_input_port_reg_pair % 2 == 0) {
		printf("0 \n");
		return 0;
	}
	else if (read_input_port_reg_pair % 2 == 1) {
		printf("1 \n");
		return 1;
	}
}


void set_led_speaking(){
	#define LED_TRANSITIONS 10
	hsv input;
	rgb rgb_res;

	double int_coef = 10;
    	
    double hsv_XMOS_dark_blue__h = 199;
    double hsv_XMOS_dark_blue__s = 0.95;
    double hsv_XMOS_dark_blue__v = 0.2;
    double hsv_XMOS_light_blue__h = 198;
    double hsv_XMOS_light_blue__s = 0.6;
    double hsv_XMOS_light_blue__v = 0.945;
    double hsv_XMOS_light_green__h = 72;
    double hsv_XMOS_light_green__s = 0.892;
    double hsv_XMOS_light_green__v = 0.99;
    double hsv_XMOS_dark_green__h = 78;
    double hsv_XMOS_dark_green__s = 0.999;
    double hsv_XMOS_dark_green__v = 0.2;

    // SETUP I2C
    int fd = wiringPiI2CSetup(IS31FL3193_ADR);
    // Set Shutdown Register to normal operatiom // All channel enable
    wiringPiI2CWriteReg8(fd, 0x00, 0x20);
    // Set current Setting Register 0x03 to its minimum value (5 mA)
    wiringPiI2CWriteReg8(fd, 0x03, 0x10);
    // Set PWM register (OUT1-OUT3)

    for (int j = 0; j<=2; j++) {
		for (unsigned fraction = 0 ; fraction < LED_TRANSITIONS ; ++fraction ) {
			// FROM DARK_BLUE TO LIGHT_BLUE :
			// HSV VALUES DARK_BLUE TO LIGHT_BLUE
			input.hue = (hsv_XMOS_light_blue__h - hsv_XMOS_dark_blue__h) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_dark_blue__h;
			input.saturation = (hsv_XMOS_light_blue__s - hsv_XMOS_dark_blue__s) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_dark_blue__s;
			input.value = (hsv_XMOS_light_blue__v - hsv_XMOS_dark_blue__v) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_dark_blue__v;
			//
			// CONVERSION HSV TO RGB
			rgb_res = hsv2rgb(input);
			// WRITING VALUES TO REGISTERS
			wiringPiI2CWriteReg8(fd, 0x04, (int)round((rgb_res.red)*255/int_coef)); // Red
			wiringPiI2CWriteReg8(fd, 0x05, (int)round((rgb_res.green)*255/int_coef)); // Green
			wiringPiI2CWriteReg8(fd, 0x06, (int)round((rgb_res.blue)*255/int_coef)); // Blue
			// Update the register
			wiringPiI2CWriteReg8(fd, 0x07, 0x00);
			sleep_ms(10);
		}
		for (unsigned fraction = 0 ; fraction < LED_TRANSITIONS ; ++fraction ) {
			// FROM LIGHT_BLUE TO LIGHT_GREEN :
			// HSV VALUES LIGHT_BLUE TO LIGHT_GREEN
			input.hue = (hsv_XMOS_light_green__h - hsv_XMOS_light_blue__h) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_light_blue__h;
			input.saturation = (hsv_XMOS_light_green__s - hsv_XMOS_light_blue__s) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_light_blue__s;
			input.value = (hsv_XMOS_light_green__v - hsv_XMOS_light_blue__v) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_light_blue__v;
			//
			// CONVERSION HSV TO RGB
			rgb_res = hsv2rgb(input);
			// WRITING VALUES TO REGISTERS
			wiringPiI2CWriteReg8(fd, 0x04, (int)round((rgb_res.red)*255/int_coef)); // Red
			wiringPiI2CWriteReg8(fd, 0x05, (int)round((rgb_res.green)*255/int_coef)); // Green
			wiringPiI2CWriteReg8(fd, 0x06, (int)round((rgb_res.blue)*255/int_coef)); // Blue
			// Update the register
			wiringPiI2CWriteReg8(fd, 0x07, 0x00);
			sleep_ms(10);
	}
		for (unsigned fraction = 0 ; fraction < LED_TRANSITIONS ; ++fraction ) {
			// FROM LIGHT_GREEN TO DARK_GREEN :
			// HSV VALUES LIGHT_GREEN TO DARK_GREEN
			input.hue = (hsv_XMOS_dark_green__h - hsv_XMOS_light_green__h) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_light_green__h;
			input.saturation = (hsv_XMOS_dark_green__s - hsv_XMOS_light_green__s) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_light_green__s;
			input.value = (hsv_XMOS_dark_green__v - hsv_XMOS_light_green__v) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_light_green__v;
			//
			// CONVERSION HSV TO RGB
			rgb_res = hsv2rgb(input);
			// WRITING VALUES TO REGISTERS
			wiringPiI2CWriteReg8(fd, 0x04, (int)round((rgb_res.red)*255/int_coef)); // Red
			wiringPiI2CWriteReg8(fd, 0x05, (int)round((rgb_res.green)*255/int_coef)); // Green
			wiringPiI2CWriteReg8(fd, 0x06, (int)round((rgb_res.blue)*255/int_coef)); // Blue
			// Update the register
			wiringPiI2CWriteReg8(fd, 0x07, 0x00);
			sleep_ms(10);
		}
		for (unsigned fraction = 0 ; fraction < LED_TRANSITIONS ; ++fraction ) {
			// FROM DARK_GREEN TO DARK_BLUE :
			// HSV VALUES DARK_GREEN TO DARK_BLUE
			input.hue = (hsv_XMOS_dark_blue__h - hsv_XMOS_dark_green__h) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_dark_green__h;
			input.saturation = (hsv_XMOS_dark_blue__s - hsv_XMOS_dark_green__s) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_dark_green__s;
			input.value = (hsv_XMOS_dark_blue__v - hsv_XMOS_dark_green__v) * ((double)fraction)/(LED_TRANSITIONS-1) + hsv_XMOS_dark_green__v;

			// CONVERSION HSV TO RGB
			rgb_res = hsv2rgb(input);
			// WRITING VALUES TO REGISTERS
			wiringPiI2CWriteReg8(fd, 0x04, (int)round((rgb_res.red)*255/int_coef)); // Red
			wiringPiI2CWriteReg8(fd, 0x05, (int)round((rgb_res.green)*255/int_coef)); // Green
			wiringPiI2CWriteReg8(fd, 0x06, (int)round((rgb_res.blue)*255/int_coef)); // Blue
			// Update the register
			wiringPiI2CWriteReg8(fd, 0x07, 0x00);
			sleep_ms(10);
		}

	}
}


int main(int argc, char **argv) {

	if (strcmp(argv[1], command_SET_LED_RGB) == 0) {
		if ((argc != 5) || (atoi(argv[2]) < 0) || (atoi(argv[2]) > 255) || (atoi(argv[3]) < 0) || (atoi(argv[3]) > 255) || (atoi(argv[4]) < 0) || (atoi(argv[4]) > 255)) {
			printf("Command '%s' invalid \n", argv[1]);
			printf("This control has 3 arguments : arg 1 : [0-255] (red), arg 2 : [0-255] (green), arg  3 : [0-255] (blue): . Ex : ./test_i2c SET_LED_RGB 255 0 0 (red). \n");
          }
		else {
			set_led_rgb(argv);
		}
       }
	if (strcmp(argv[1], command_SET_LED_HSV) == 0) {
        if ((argc != 5) || (atoi(argv[2]) < 0) || (atoi(argv[2]) > 360) || (atoi(argv[3]) < 0) || (atoi(argv[3]) > 1) || (atoi(argv[4]) < 0) || (atoi(argv[4]) > 1)) {
			printf("Command '%s' invalid \n", argv[1]);
			printf("This control has 3 arguments : arg 1 : [0-360] (hue), arg 2 : [0-1] (saturation), arg  3 : [0-1] (value) : . Ex : ./test_i2c SET_LED_HSV 280 0.5 0.3 (red). \n");
           }

      else {
            parse_led_hsv(argv);
        }
	}

	if (strcmp(argv[1], command_SET_LED_HSL) == 0) {
		if ((argc != 5) || (atoi(argv[2]) < 0) || (atoi(argv[2]) > 360) || (atoi(argv[3]) < 0) || (atoi(argv[3]) > 1) || (atoi(argv[4]) < 0) || (atoi(argv[4]) > 1)) {
			printf("Command '%s' invalid \n", argv[1]);
			printf("This control has 3 arguments : arg 1 : [0-360] (hue), arg 2 : [0-1] (saturation), arg  3 : [0-1] (lightness) : . Ex : ./test_i2c SET_LED_HSL 264 0.8 0.2 (red). \n");
           }
        else {
             parse_led_hsl(argv);
        }
	}


	if (strcmp(argv[1], command_SET_MUTE_MIC) == 0) {
		if ((argc != 3) || ((atoi(argv[2]) != 0) && (atoi(argv[2]) != 1))) {
			printf("Command '%s' invalid \n", argv[1]);
			printf("This control has 1 argument : 1 (mute) or 0 (unmute). Ex : ./test_i2c SET_MUTE_MIC 0 \n");
          }
        else {
			set_mute_mic(argv);
		}
	}

	if (strcmp(argv[1], command_SET_DAC_RESET) == 0) {
		if (argc != 3) {
			printf("Command '%s' invalid \n", argv[1]);
			printf("This control has 1 argument : 1 (turning on the dac, software reset with default value) or 0 (turn off the DAC). Ex : ./test_i2c SET_DAC_RESET 0 \n");
          }
        else {
			set_dac_reset(argv);
		}
	}

	if (strcmp(argv[1], command_INIT_DAC) == 0) {
		init_dac();
	}

	if (strcmp(argv[1], command_GET_BUT_MUTE) == 0) {
		get_button_mute();
	}

	if (strcmp(argv[1], command_GET_BUT_VOL_DN) == 0) {
		get_button_vol_dwn();
	}

	if (strcmp(argv[1], command_GET_BUT_VOL_UP) == 0) {
		get_button_vol_up();
	}

	if (strcmp(argv[1], command_GET_BUT_ACTION) == 0) {
		get_button_action();
	}

	if (strcmp(argv[1], command_SET_BOOT_SEL) == 0) {
		if (argc != 3) {
			printf("Command '%s' invalid \n", argv[1]);
			printf("This control has 1 argument : 1 (set pin BOOT_SEL) or 0 (set pin BOOT_SEL). Ex : ./test_i2c SET_BOOT_SEL 1 \n");
          }
        else {
			boot_sel(argv);
		}
	}

	if (strcmp(argv[1], command_SET_INT_INPUT) == 0) {
		if (argc != 3) {
			printf("Command '%s' invalid \n", argv[1]);
			printf("This control has 1 argument : 1 (enable interrupts for P0_0, P0_1, P0_2, P0_3, P0_4, P0_6 defined as input) or 0 (disable interrupts for P0_0, P0_1, P0_2, P0_3, P0_4, P0_6 defined as input). Ex : ./test_i2c SET_INT_INPUT 1 \n");
          }
        else {
			int_input(argv);
		}
	}

	if (strcmp(argv[1], command_GET_INT_N_IN) == 0) {
		get_int_n_in();
	}

	if (strcmp(argv[1], command_SET_LED_SPEAKING) == 0) {
		set_led_speaking();
	}
	return(0);
}
