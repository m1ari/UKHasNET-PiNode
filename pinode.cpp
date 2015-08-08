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
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
//#include <sys/ioctl.h>
//#include <linux/types.h>
//#include <linux/spi/spidev.h>
#include <jansson.h>
#include <curl/curl.h>
#include "spi.h"
#include "rfm69.h"
#include "RFM69Registers.h"
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


bool loop=true;
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

void sig_handler(int sig){
	fprintf(stderr,"Got Signal %d\n",sig);
	switch (sig){
		case SIGHUP:
			// Sig HUP, Do a reload
			fprintf(stderr,"Sig: Got SIGHUP\n");
			loop=false;
		break;
		case SIGINT: // 2
			// Interupt (Ctrl c From command line) - Graceful shutdown
			fprintf(stderr,"Sig: Got SIGINT - Shutting down\n");
			loop=false;
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

	rfm69 rfm69(device);
	// We may be able to json_decref(jdev) now.

	rfm69.init();	// Initialise the RFM69 (and SPI)

	// Set RFM69 Settings
	uint8_t i;
	for (i = 0; CONFIG[i][0] != 255; i++)
		rfm69.write(CONFIG[i][0], CONFIG[i][1]);

	rfm69.getVer();

	signal(SIGHUP,sig_handler);
	signal(SIGINT,sig_handler);

/* Start main loop around here */
	char seq='a';
	rfm69.setMode(RFM69_MODE_RX);
	rfm69.setLnaMode(RF_TESTLNA_SENSITIVE);
	
	char string[65];
	float rfm_temp=0;
	//uint8_t buffer[67];

	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://ukhas.net/api/upload");
	}

	while (loop){
		memset (string, 0, sizeof(string)) ;

		// TODO Spend some time waiting for a packet  timeout at ~60 seconds
		for (int t=0; t<=55; t++){
			for (int m=0; m<=20; m++){
				if (!loop)
					break;

				if (rfm69.checkrx()) {
					//std::string rxdata = rfm69.getRX();
					std::cout << "rx:" << rfm69.getRX() << ", RSSI: " << (int)rfm69.getRSSI() << std::endl;
					if (curl){
						char curlbuff[255];
						sprintf(curlbuff,"origin=%s&data=%s&rssi=%d",nodename, rfm69.getRX().c_str(),(int)rfm69.getRSSI());
						curl_easy_setopt(curl, CURLOPT_POSTFIELDS, curlbuff);
 
						/* Perform the request, res will get the return code */ 
						res = curl_easy_perform(curl);
    						/* Check for errors */ 
						if(res != CURLE_OK)
							fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
					}
				
				} else {
					rfm69.delayMilli(50);
				}
			}
		}


		// rf69.recv(buf, &len)

		// If timed out create our own packet

#ifdef TX
		// Read the Temperature
		float temp=rfm69.readTemp();
		std::cout << "Read Temperature " << (float)temp << std::endl;
		if (255 != temp)
			rfm_temp = temp;

		// TODO: with some casts we should be able to combine these
		// Create String
		if (('a' == seq) || ('z' == seq))  {
			snprintf(string,65,"3%cL%sT%.1f[%s]",seq,location,rfm_temp,nodename);
		} else {
			snprintf(string,65,"3%cT%.1f[%s]",seq,rfm_temp,nodename);
		}

		if ('z' == seq++)
			seq='b';

		printf("tx: %s\n",string);

		rfm69.bulkWrite(string);

		rfm69.setMode(RFM69_MODE_RX);
		rfm69.setLnaMode(RF_TESTLNA_SENSITIVE);
#endif
		// Pause a while - This may be redundant when we have RX code
		int timer=55;
		while (loop && timer) {
			timer--;
			sleep(1);
		}

	}

	// Ensure we don't stay TXing all the time.
	rfm69.setMode(RFM69_MODE_RX);

	if (curl){
		/* always cleanup */ 
		curl_easy_cleanup(curl);
		curl_global_cleanup();
	}
	//close(fd);
	rfm69.close();
#ifdef JSON_CONFIG
	json_decref(jdev);	// This could probably be done earlier ...
	json_decref(jnode);
	json_decref(config);
#endif
	return 0;
} 


