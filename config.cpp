#include <jansson.h>
#include "config.h"

Config::Config() {
	config=NULL;
	filename="";
}

Config::~Config() {
	json_decref(config);
}

void Config::setFile(std::string f){
	filename=f;
}

void Config::loadConfig(){
	FILE *fp=NULL;
	json_error_t err;

	fp = fopen(filename.c_str(), "r");
	if (fp == NULL){
		fprintf(stderr, "Failed to open %s\n",filename.c_str());
		perror("Failed to open config file");
		exit(-1);
	}
	config=json_loadf(fp,0,&err);	// Newer version of jansoon lib
	fclose(fp);

	if (config == NULL){
		perror("Failed to parse the json config file");
		fprintf(stderr, "json error: %d %d %d\n%s\n", err.line, err.column, err.position, err.text);
		exit(-1);
	}
}

void Config::saveConfig() const{
	// rename old config file
	// open filename to save
	// Dump json
	// close file
}

const char* Config::getDevice() const{
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
	return device;
}

const char *Config::getNodename() const{
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
	return nodename;
}

std::string Config::getLocation() const{
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
	return std::string(location);
}

bool Config::enableRX() const{
	json_t *jval;
	jval = json_object_get(config, "enable_rx");
	bool ret;
	if (json_is_boolean(jval)){
		ret = json_is_true(jval);
	} else {
		perror("Can't read enable_rx from config file\n");
		exit(-1);
	}
	return ret;
}
bool Config::enableTX() const{
	json_t *jval;
	jval = json_object_get(config, "enable_tx");
	bool ret;
	if (json_is_boolean(jval)){
		ret = json_is_true(jval);
	} else {
		perror("Can't read enable_tx from config file\n");
		exit(-1);
	}
	return ret;
}

bool Config::enableUpload() const{
	json_t *jval;
	jval = json_object_get(config, "enable_upload");
	bool ret;
	if (json_is_boolean(jval)){
		ret = json_is_true(jval);
	} else {
		perror("Can't read enable_upload from config file\n");
		exit(-1);
	}
	return ret;
}
bool Config::enableRepeater() const{
	json_t *jval;
	jval = json_object_get(config, "enable_repeater");
	bool ret;
	if (json_is_boolean(jval)){
		ret = json_is_true(jval);
	} else {
		perror("Can't read enable_repeater from config file\n");
		exit(-1);
	}
	return ret;
}

/* if we're not using config.json set the same variables to use the #define values */
/*
#ifndef JSON_CONFIG
	#define SPI_DEV "/dev/spidev0.1"
	#define NODE_NAME "MAPI"
	#define LOCATION "0.1,0.1,100"
#endif
static const char *device = SPI_DEV;
static const char *nodename = NODE_NAME;
static const char *location = LOCATION;

*/
