
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
		void bulkWrite(std::string data);
		uint8_t getVer();
		void setMode(uint8_t mode);
		float readTemp();
		bool checkrx();

	private:
		spi s;
		uint8_t _mode;


};

#endif

