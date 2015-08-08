
#ifndef RFM69_H
#define RFM69_H

#include <string>

class rfm69 {
	public:
		rfm69(std::string dev);
		void init();
		void close();
		uint8_t read(uint8_t reg);
		void write(uint8_t reg, uint8_t val);
		void bulkRead(uint8_t reg, uint8_t *buf, uint8_t len);
		void bulkWrite(std::string data);
		uint8_t getVer();
		void setMode(uint8_t mode);
		float readTemp();
		bool checkrx();
		std::string getRX();
		int16_t getRSSI();
		void delayMilli(uint16_t howLong);
		void setLnaMode(uint8_t lnaMode);

	private:
		spi s;
		void clearFifo();

		uint8_t _mode;

		// Storage to receieved strings
		uint8_t buflen;
		uint8_t rxbuf[255];
		int16_t rxrssi;

};

#endif

