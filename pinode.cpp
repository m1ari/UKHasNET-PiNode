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
//#include <jansson.h>
#include <curl/curl.h>
#include "spi.h"
#include "rfm69.h"
#include "config.h"
#include "RFM69Registers.h"
#include "RFM69Config.h"

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

	Config conf;
	conf.setFile("config.json");
	conf.loadConfig();

	/* get the SPI device from config.json */
	const char *device = conf.getDevice();

	/* Get the nodename from config.json */
	const char *nodename;
	nodename = conf.getNodename();

	/* get the location from config.json */
	std::string location = conf.getLocation();
	
	fprintf(stdout,"Opening SPI Device on %s\n", device);

	rfm69 rfm69(device);

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
	//rfm69.setLnaMode(RF_TESTLNA_SENSITIVE);
	rfm69.setLnaMode(RF_TESTLNA_NORMAL);
	
	char string[65];
	float rfm_temp=0;
	//uint8_t buffer[67];

	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();

	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://ukhas.net/api/upload");
	}

	// Get time
	time_t start = 0 ;
	while (loop){
		memset (string, 0, sizeof(string)) ;

		if (conf.enableRX()) {
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
			
			}
		}

		// If timed out create our own packet

		if (conf.enableTX()){
			if (time(NULL) > (start + 62)){
				// Read the Temperature
				float temp=rfm69.readTemp();
				std::cout << "Read Temperature " << (float)temp << std::endl;
				if (255 != temp)
					rfm_temp = temp;
	
				// TODO: with some casts we should be able to combine these
				// Create String
				if (('a' == seq) || ('z' == seq))  {
					snprintf(string,65,"3%cL%sT%.1f[%s]",seq,location.c_str(),rfm_temp,nodename);
				} else {
					snprintf(string,65,"3%cT%.1f[%s]",seq,rfm_temp,nodename);
				}
	
				if ('z' == seq++)
					seq='b';
	
				printf("tx: %s\n",string);
	
				rfm69.bulkWrite(string);
	
				rfm69.setMode(RFM69_MODE_RX);
				//rfm69.setLnaMode(RF_TESTLNA_SENSITIVE);
				rfm69.setLnaMode(RF_TESTLNA_NORMAL);
				start = time(NULL);
			}
		}

		// Put a short delay in so it's not just polling all the time
		rfm69.delayMilli(100);
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
	return 0;
} 


