// spi.h

#ifndef SPI_h
#define SPI_h

#include <string>
#include <cstdint>

class spi {
	public:
		spi();
		spi(std::string dev);
		void setDevice(std::string dev);
		void open();
		void close();
		void setMode(uint8_t mode);
		void setBits(uint8_t bits);
		void setSpeed(uint32_t speed);
		void getState();
		int transfer (uint8_t *data, int len);
		//int getFD();		// Bodge until we've sorted out other code

	private:
		int fd;
		std::string device;

};

#endif
