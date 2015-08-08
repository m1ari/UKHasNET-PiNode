#include "spi.h"
#include <cstdint>
#include <string>
#include <cstring>
#include <iostream>
#include <fcntl.h>
//#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cstdio>

spi::spi(){
}

spi::spi(std::string dev){
	device=dev;
}


void spi::setDevice(std::string dev){
	device=dev;
}

void spi::open() {
	fd = ::open(device.c_str(), O_RDWR);
	if (fd < 0) {
		perror("can't open device\n");
		exit(-1);
	}
}

void spi::close() {
	::close(fd);
}

void spi::setMode(uint8_t mode){
	int ret=0;
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		perror("can't set spi mode(wr)");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		perror("can't get spi mode(rd)");

	std::cout << "spi mode: " << (int)mode << std::endl;

}

void spi::setBits(uint8_t bits){
	int ret=0;
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		perror("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		perror("can't get bits per word");

	std::cout << "bits per word: "<< (int)bits << std::endl;
}

void spi::setSpeed(uint32_t speed){
	int ret=0;
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		perror("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		perror("can't get max speed hz");

	std::cout << "max speed: " << (int)speed << " Hz (" << (int)(speed/1000) << "KHz)" << std::endl;
}

void spi::getState(){
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

	std::cout << "Current State" << std::endl;
	std::cout << "\tspi mode: " << (int)mode << std::endl;
	std::cout << "\tbits per word: " << (int)bits << std::endl;
	std::cout << "\tmax speed: "<< (int)speed << " Hz (" << (int)(speed/1000) << " KHz)" << std::endl;
}

int spi::transfer (uint8_t *data, int len) {
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

	spi.tx_buf	= (unsigned long)data ;
	spi.rx_buf	= (unsigned long)data ;
	spi.len	= len ;
	//spi.delay_usecs	= 0 ;
	//spi.speed_hz	= 0 ;
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

/*
int spi::getFD(){
	return fd;
}
*/
