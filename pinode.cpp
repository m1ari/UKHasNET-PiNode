/*
 * Based on
 * https://raw.githubusercontent.com/raspberrypi/linux/rpi-3.10.y/Documentation/spi/spidev_test.c
 * This used the jansson json library, to install use:
 *   Rasbian: sudo apt-get install libjansson-dev
 *
 * Compile with: gcc -o pinode pinode.c -ljansson -Wall
 */

#include <stdint.h>
#include <unistd.h>
//#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <jansson.h>
#include "RFM69Registers.h"
#include "RFM69Config.h"
#include "spi.h"


/* Options to edit are here
 * DEBUG_SPI makes it print lots of debug stuff
 * JSON_CONFIG uses config.json (should be on by default
 */

//#define DEBUG_SPI	// Debug SPI stuffs
#define JSON_CONFIG	// Config is in config.json

/* If you disabled JSON_CONFIG then set values here
 */
#ifndef JSON_CONFIG
  #define SPI_DEV "/dev/spidev0.1"
  #define NODE_NAME "MAPI"
  #define LOCATION "0.1,0.1,100"
#endif


int loop=1;
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

void sig_handler(int sig){
	fprintf(stderr,"Got Signal %d\n",sig);
	switch (sig){
		case SIGHUP:
			// Sig HUP, Do a reload
			fprintf(stderr,"Sig: Got SIGHUP\n");
			loop=0;
		break;
		case SIGINT: // 2
			// Interupt (Ctrl c From command line) - Graceful shutdown
			fprintf(stderr,"Sig: Got SIGINT - Shutting down\n");
			loop=0;
		break;
		case 15:
			// TERM
			fprintf(stderr,"Sig: Got SIGTERM\n");
		break;
		case 16:
			// USR1
			fprintf(stderr,"Sig: Got SIGUSR1\n");
		break;
		case 17:
			// USR2
			fprintf(stderr,"Sig: Got SIGUSR2\n");
		break;
	}
}
/*

// Reads a single byte from a register
uint8_t spi_read(int fd, uint8_t reg){
	uint8_t buffer[2];
	buffer[0] = reg & ~RFM69_SPI_WRITE_MASK;	// Ensure the write mask is off
	buffer[1] = 0;

	spi_transfer(fd, buffer, ARRAY_SIZE(buffer)); // Send the address with the write mask off
	return buffer[1];
}
// Writes a single byte to a register
void spi_write(int fd, uint8_t reg, uint8_t val){
	uint8_t buffer[2];
	buffer[0] = reg | RFM69_SPI_WRITE_MASK;	// Set the write mask
	buffer[1] = val;

	spi_transfer(fd, buffer,ARRAY_SIZE(buffer)); // Send the address with the write mask on
}

*/

/*
void rfm69_setmode(int fd, uint8_t mode){
    spi_write(fd, RFM69_REG_01_OPMODE, (spi_read(fd,RFM69_REG_01_OPMODE) & 0xE3) | mode);
}
*/

int main(int argc, char *argv[]) {

#ifdef JSON_CONFIG
	/* Load the config.json file */
	FILE *fp_json=NULL;
	json_error_t err;
	fp_json = fopen("config.json", "r");
	if (fp_json == NULL){
		perror("Failed to open config file");
		exit(-1);
	}

	/* Read the json into a useful structure */
	json_t *config=NULL;
	config=json_loadf(fp_json,0,&err); // Newer version of jansoon lib
	fclose(fp_json);
	if (config == NULL){
		perror("Failed to parse the json config file");
		fprintf(stderr, "json error: %d %d %d\n%s\n", err.line, err.column, err.position, err.text);
		exit(-1);
	}

	/* get the SPI device from config.json */
	json_t *jdev;
	jdev = json_object_get(config, "device");
	const char *device;
	if (json_is_string(jdev)){
		device=json_string_value(jdev);;
	} else {
		perror("Can't read device name from config file\n");
		exit(-1);
	}

	/* Get the nodename from config.json */
	json_t *jnode;
	jnode = json_object_get(config, "nodename");
	const char *nodename;
	if (json_is_string(jnode)){
		nodename=json_string_value(jnode);;
	} else {
		perror("Can't read nodename name from config file\n");
		exit(-1);
	}

	/* get the location from config.json */
	char location[31];	// lat: -nnn.nnnnn, long: -nn.nnnnn, Alt: -nnnnn.nn (10 + 9 + 9 + commas)
	json_t *jloc;
	jloc = json_object_get(config, "location");
	if (!json_is_object(jloc)){
		perror("Can't read location from config file\n");
		exit(-1);
	}

	double latitude, longitude;
	json_t *jloc_lat, *jloc_lon, *jloc_alt;

	jloc_lat = json_object_get(jloc, "latitude");
	if (json_is_real(jloc_lat)){
		latitude=json_real_value(jloc_lat);
	} else {
		perror("Can't read latitude from config file\n");
		exit(-1);
	}

	jloc_lon = json_object_get(jloc, "longitude");
	if (json_is_real(jloc_lat)){
		longitude=json_real_value(jloc_lon);
	} else {
		perror("Can't read longitude from config file\n");
		exit(-1);
	}

	jloc_alt = json_object_get(jloc, "altitude");
	if (json_is_real(jloc_alt)){
		double altitude;
		altitude=json_real_value(jloc_alt);
		snprintf(location,31,"%.5f,%.5f,%.2f",latitude, longitude, altitude);
	} else if (json_is_integer(jloc_alt)){
		int altitude;
		altitude=json_integer_value(jloc_alt);
		snprintf(location,31,"%.5f,%.5f,%d",latitude, longitude, altitude);
	} else {
		snprintf(location,31,"%.5f,%.5f",latitude, longitude);
		perror("Can't read altitude from config file\n");
	}
	json_decref(jloc_lat);
	json_decref(jloc_lon);
	json_decref(jloc_alt);
	json_decref(jloc);

#else
	/* if we're not using config.json set the same variables to use the #define values */
	static const char *device = SPI_DEV;
	static const char *nodename = NODE_NAME;
	static const char *location = LOCATION;
#endif
	
	fprintf(stdout,"Opening SPI Device on %s\n", device);

	spi spi(device);
	spi.open();
	
	/*int fd;
	fd = open(device, O_RDWR);
	if (fd < 0){
		perror("can't open device\n");
		exit(-1);
	}
	*/
	/* If we're not going to use device anymore we should decref it
	 * json_decref(jdev) but only if it came from json;
	 */

	spi.getState();
	spi.setMode(SPI_MODE_0);
	spi.setBits(8);			// Bits per word
	//SPI.setBitOrder(MSBFIRST);
	spi.setSpeed(500000);	// Clock Speed (similar to SPI_CLOCK_DIV2 on Arduino
	spi.getState();

	// Set RFM69 Settings
	uint8_t i;
	for (i = 0; CONFIG[i][0] != 255; i++)
		spi.write(CONFIG[i][0], CONFIG[i][1]);

	uint8_t ver = spi.read(RFM69_REG_10_VERSION);
	printf("RFM69 Version: %02X\n", ver);
	if (ver != 0x24){
		fprintf(stderr,"Version doesn't match for RFM69\n");
		exit(-1);
	}


	signal(SIGHUP,sig_handler);

/* Start main loop around here */
	char seq='a';
	//rfm69_setmode(fd,RFM69_MODE_RX);
    	spi.write(RFM69_REG_01_OPMODE, (spi.read(RFM69_REG_01_OPMODE) & 0xE3) | RFM69_MODE_RX);
	char string[65];
	//uint8_t rfm_temp;
	uint8_t buffer[67];
	while (loop){
		memset (string, 0, sizeof(string)) ;


	
		/* TODO Spend some time waiting for a packet  timeout at ~60 seconds */


		/* If timed out create our own packet */

		/* TODO: with some casts we should be able to combine these */
		/* Create String */
		if (('a' == seq) || ('z' == seq))  {
			snprintf(string,65,"3%cL%sT19[%s]",seq,location,nodename);
		} else {
			snprintf(string,65,"3%cT19[%s]",seq,nodename);
		}

		if ('z' == seq++)
			seq='b';

		printf("tx: %s\n",string);

		/* Create spi buffer */
		buffer[0] = 0x80; // Fifo mode
		buffer[1] = strlen(string);
		memcpy(&buffer[2],string,65);

		/* Set to TX Mode */
		//rfm69_setmode(fd, RFM69_MODE_TX);
    		spi.write(RFM69_REG_01_OPMODE, (spi.read(RFM69_REG_01_OPMODE) & 0xE3) | RFM69_MODE_TX);
		while(!(spi.read(RFM69_REG_27_IRQ_FLAGS1) & RF_IRQFLAGS1_TXREADY)) { };

		spi.transfer(buffer,buffer[1]+2);
		while(!(spi.read(RFM69_REG_28_IRQ_FLAGS2) & RF_IRQFLAGS2_PACKETSENT)) { };

		//rfm69_setmode(fd,RFM69_MODE_RX);
    		spi.write(RFM69_REG_01_OPMODE, (spi.read(RFM69_REG_01_OPMODE) & 0xE3) | RFM69_MODE_RX);

		sleep(55);	/* Pause for whilst we don't do receiving */

	}
	//close(fd);
	spi.close();
#ifdef JSON_CONFIG
	json_decref(jnode);
	json_decref(config);
#endif
	return 0;
} 


