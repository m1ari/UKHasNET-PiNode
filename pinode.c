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
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <jansson.h>
#include "RFM69.h"
#include "RFM69Config.h"


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


#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

void spi_set_mode(int fd, uint8_t mode){
	int ret=0;
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		perror("can't set spi mode(wr)");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		perror("can't get spi mode(rd)");

	printf("spi mode: %d\n", mode);

}

//SPI_IOC_RD_MODE32, SPI_IOC_WR_MODE32

//SPI_IOC_RD_LSB_FIRST, SPI_IOC_WR_LSB_FIRST

void spi_set_bits(int fd, uint8_t bits){
	int ret=0;
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		perror("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		perror("can't get bits per word");

	printf("bits per word: %d\n", bits);
}

void spi_set_speed(int fd, uint32_t speed){
	int ret=0;
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		perror("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		perror("can't get max speed hz");

	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
}
void spi_get_state(int fd){
	int ret=0;

	uint8_t mode;
	uint8_t bits;
	uint32_t speed;

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		perror("can't get spi mode(rd)");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		perror("can't get bits per word");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		perror("can't get max speed hz");

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
}

int spi_transfer (int fd, uint8_t *data, int len) {
	struct spi_ioc_transfer spi ;
	int ret;

#ifdef DEBUG_SPI
	int i;
	printf("SPI Sending:  ");
	for (i=0; i<len; i++)
		printf("%02X ", data[i]);
	printf("\n");
#endif

	memset (&spi, 0, sizeof (spi)) ;

	spi.tx_buf        = (unsigned long)data ;
	spi.rx_buf        = (unsigned long)data ;
	spi.len           = len ;
	//spi.delay_usecs   = 0 ;
	//spi.speed_hz      = 0 ;
	//spi.bits_per_word = 0 ;

	ret= ioctl (fd, SPI_IOC_MESSAGE(1), &spi) ;
	if (ret < 1){
		perror("Error sending SPI Message");
#ifdef DEBUG_SPI
	} else {
		printf("SPI Recieved: ");
		for (i=0; i<len; i++)
			printf("%02X ", data[i]);
		printf("\n");
#endif
	}

	return ret;
}

/* Reads a single byte from a register */
uint8_t spi_read(int fd, uint8_t reg){
	uint8_t buffer[2];
	buffer[0] = reg & ~RFM69_SPI_WRITE_MASK;	// Ensure the write mask is off
	buffer[1] = 0;

	spi_transfer(fd, buffer, ARRAY_SIZE(buffer)); // Send the address with the write mask off
	return buffer[1];
}
/* Writes a single byte to a register */
void spi_write(int fd, uint8_t reg, uint8_t val){
	uint8_t buffer[2];
	buffer[0] = reg | RFM69_SPI_WRITE_MASK;	// Set the write mask
	buffer[1] = val;

	spi_transfer(fd, buffer,ARRAY_SIZE(buffer)); // Send the address with the write mask on
}

void rfm69_setmode(int fd, uint8_t mode){
    spi_write(fd, RFM69_REG_01_OPMODE, (spi_read(fd,RFM69_REG_01_OPMODE) & 0xE3) | mode);
}

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

	int fd;
	fd = open(device, O_RDWR);
	if (fd < 0){
		perror("can't open device\n");
		exit(-1);
	}
	/* If we're not going to use device anymore we should decref it
	 * json_decref(jdev) but only if it came from json;
	 */

	spi_get_state(fd);
	spi_set_mode(fd,SPI_MODE_0);	/* SPI Mode */
	spi_set_bits(fd,8);		/* Bits per word */
	//SPI.setBitOrder(MSBFIRST);
	spi_set_speed(fd,500000);	/* Clock Speed (similar to SPI_CLOCK_DIV2 on Arduino */

	spi_get_state(fd);

	// Set RFM69 Settings
	uint8_t i;
	for (i = 0; CONFIG[i][0] != 255; i++)
		spi_write(fd,CONFIG[i][0], CONFIG[i][1]);


	printf("Version: %02X\n", spi_read(fd, 0x10));

	rfm69_setmode(fd,RFM69_MODE_RX);


	char string[65];
	snprintf(string,65,"3aL%sT19[%s]",location,nodename);

	uint8_t buffer[67];
	buffer[0] = 0x80; // Fifo mode
	buffer[1] = strlen(string);
	memcpy(&buffer[2],string,65);

	// Send Packet
	rfm69_setmode(fd, RFM69_MODE_TX);
	while(!(spi_read(fd, RFM69_REG_27_IRQ_FLAGS1) & RF_IRQFLAGS1_TXREADY)) { };

	spi_transfer(fd,buffer,buffer[1]+2);
	while(!(spi_read(fd,RFM69_REG_28_IRQ_FLAGS2) & RF_IRQFLAGS2_PACKETSENT)) { };

	rfm69_setmode(fd,RFM69_MODE_RX);

	close(fd);
#ifdef JSON_CONFIG
	json_decref(config);
#endif
	return 0;
} 


