// spi.h

#ifndef SPI_h
#define SPI_h

#include <string>
#include <cstdint>

class spi {
	public:
		spi(std::string dev);
		void open();
		void close();
		void setMode(uint8_t mode);
		void setBits(uint8_t bits);
		void setSpeed(uint32_t speed);
		void getState();
		int transfer (uint8_t *data, int len);
		uint8_t read(uint8_t reg);
		void write(uint8_t reg, uint8_t val);
		int getFD();		// Bodge until we've sorted out other code

	private:
		int fd;
		std::string device;

};

#endif
