
#include <linux/spi/spidev.h>

#include <iostream>
#include <string>
#include <string.h>
#include "spi.h"
#include "rfm69.h"
#include "RFM69Registers.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

rfm69::rfm69(std::string dev){
	s.setDevice(dev);
	s.open();
}

void rfm69::init(){
	s.getState();
	s.setMode(SPI_MODE_0);
	s.setBits(8);		// Bits per word
	//SPI.setBitOrder(MSBFIRST);
	s.setSpeed(500000);	// Clock Speed (similar to SPI_CLOCK_DIV2 on Arduino
	s.getState();
}

void rfm69::close(){
	s.close();
}

uint8_t rfm69::read(uint8_t reg){
	uint8_t buffer[2];
	buffer[0] = reg & ~RFM69_SPI_WRITE_MASK;	// Ensure the write mask is off
	buffer[1] = 0;

	s.transfer(buffer, ARRAY_SIZE(buffer)); // Send the address with the write mask off
	return buffer[1];
}

void rfm69::write(uint8_t reg, uint8_t val){
	uint8_t buffer[2];
	buffer[0] = reg | RFM69_SPI_WRITE_MASK; // Set the write mask
	buffer[1] = val;

	s.transfer(buffer,ARRAY_SIZE(buffer)); // Send the address with the write mask on
}

void rfm69::bulkWrite(std::string data){
	uint8_t buffer[300];

	buffer[0] = (RFM69_REG_00_FIFO | RFM69_SPI_WRITE_MASK);	// Set FIFO mode;
	buffer[1] = data.length();
	memcpy(&buffer[2],data.c_str(),255);

	/* Set to TX Mode */
	setMode(RFM69_MODE_TX);

	// Send the string
        s.transfer(buffer,buffer[1]+2);

	// Wait for the packet to be sent
	while(!(read(RFM69_REG_28_IRQ_FLAGS2) & RF_IRQFLAGS2_PACKETSENT)) { };

	setMode(RFM69_MODE_RX);
}

uint8_t rfm69::getVer(){
	uint8_t ver = read(RFM69_REG_10_VERSION);
        std::cout << "RFM69 Version: 0x" << std::hex << (int)ver << std::endl;
        if (ver != 0x24){
                std::cerr << "Version doesn't match for RFM69" << std::endl;
                exit(-1);
        }
	return ver;
}

void rfm69::setMode(uint8_t mode){
	write(RFM69_REG_01_OPMODE, (read(RFM69_REG_01_OPMODE) & 0xE3) | mode);
	_mode=mode;

	if (mode == RFM69_MODE_TX){
		while(!(read(RFM69_REG_27_IRQ_FLAGS1) & RF_IRQFLAGS1_TXREADY)) { };
	}

}

float rfm69::readTemp() {
	// Set mode into Standby (required for temperature measurement)
	uint8_t oldMode = _mode;
	setMode(RFM69_MODE_STDBY);

	// Trigger Temperature Measurement
	write(RFM69_REG_4E_TEMP1, RF_TEMP1_MEAS_START);

	// Wait for reading to be ready
	while (read(RFM69_REG_4E_TEMP1) & RF_TEMP1_MEAS_RUNNING);

	// Read raw ADC value
	uint8_t rawTemp = read(RFM69_REG_4F_TEMP2);
	
	// Set transceiver back to original mode
	setMode(oldMode);

	// Return processed temperature value
	//return 168.3-float(rawTemp);
	return float(rawTemp) - 90.0;
}

bool rfm69::checkrx(){
}


#if 0


void RFM69::spiBurstRead(uint8_t reg, uint8_t* dest, uint8_t len)
{
    digitalWrite(_slaveSelectPin, LOW);
    
    SPI.transfer(reg & ~RFM69_SPI_WRITE_MASK); // Send the start address with the write mask off
    while (len--)
        *dest++ = SPI.transfer(0);

    digitalWrite(_slaveSelectPin, HIGH);
}


boolean RFM69::checkRx()
{
    // Check IRQ register for payloadready flag (indicates RXed packet waiting in FIFO)
    if(spiRead(RFM69_REG_28_IRQ_FLAGS2) & RF_IRQFLAGS2_PAYLOADREADY) {
        // Get packet length from first byte of FIFO
        _bufLen = spiRead(RFM69_REG_00_FIFO)+1;
        // Read FIFO into our Buffer
        spiBurstRead(RFM69_REG_00_FIFO, _buf, RFM69_FIFO_SIZE);
        // Read RSSI register (should be of the packet? - TEST THIS)
        _lastRssi = -(spiRead(RFM69_REG_24_RSSI_VALUE)/2);
        // Clear the radio FIFO (found in HopeRF demo code)
        clearFifo();
        return true;
    }
    
    return false;
}

void RFM69::recv(uint8_t* buf, uint8_t* len)
{
    // Copy RX Buffer to byref Buffer
    memcpy(buf, _buf, _bufLen);
    *len = _bufLen;
    // Clear RX Buffer
    _bufLen = 0;
}

void RFM69::send(const uint8_t* data, uint8_t len, uint8_t power)
{
    // power is TX Power in dBmW (valid values are 2dBmW-20dBmW)
    if(power<2 or power >20) {
        // Could be dangerous, so let's check this
        return;
    }
    uint8_t oldMode = _mode;
    // Copy data into TX Buffer
    memcpy(_buf, data, len);
    // Update TX Buffer Size
    _bufLen = len;
    // Start Transmitter
    setMode(RFM69_MODE_TX);
    // Set up PA
    if(power<=17) {
        // Set PA Level
        uint8_t paLevel = power + 14;
	spiWrite(RFM69_REG_11_PA_LEVEL, RF_PALEVEL_PA0_OFF | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | paLevel);        
    } else {
        // Disable Over Current Protection
        spiWrite(RFM69_REG_13_OCP, RF_OCP_OFF);
        // Enable High Power Registers
        spiWrite(RFM69_REG_5A_TEST_PA1, 0x5D);
        spiWrite(RFM69_REG_5C_TEST_PA2, 0x7C);
        // Set PA Level
        uint8_t paLevel = power + 11;
	spiWrite(RFM69_REG_11_PA_LEVEL, RF_PALEVEL_PA0_OFF | RF_PALEVEL_PA1_ON | RF_PALEVEL_PA2_ON | paLevel);
    }
    // Wait for PA ramp-up
    while(!(spiRead(RFM69_REG_27_IRQ_FLAGS1) & RF_IRQFLAGS1_TXREADY)) { };
    // Throw Buffer into FIFO, packet transmission will start automatically
    spiFifoWrite(_buf, _bufLen);
    // Clear MCU TX Buffer
    _bufLen = 0;
    // Wait for packet to be sent
    while(!(spiRead(RFM69_REG_28_IRQ_FLAGS2) & RF_IRQFLAGS2_PACKETSENT)) { };
    // Return Transceiver to original mode
    setMode(oldMode);
    // If we were in high power, switch off High Power Registers
    if(power>17) {
        // Disable High Power Registers
        spiWrite(RFM69_REG_5A_TEST_PA1, 0x55);
        spiWrite(RFM69_REG_5C_TEST_PA2, 0x70);
        // Enable Over Current Protection
        spiWrite(RFM69_REG_13_OCP, RF_OCP_ON | RF_OCP_TRIM_95);
    }
}

void RFM69::SetLnaMode(uint8_t lnaMode) {
    // RF_TESTLNA_NORMAL (default)
    // RF_TESTLNA_SENSITIVE
    spiWrite(RFM69_REG_58_TEST_LNA, lnaMode);
}

void RFM69::clearFifo() {
    // Must only be called in RX Mode
    // Apparently this works... found in HopeRF demo code
    setMode(RFM69_MODE_STDBY);
    setMode(RFM69_MODE_RX);
}


int16_t RFM69::lastRssi() {
    return _lastRssi;
}

int16_t RFM69::sampleRssi() {
    // Must only be called in RX mode
    if(_mode!=RFM69_MODE_RX) {
        // Not sure what happens otherwise, so check this
        return 0;
    }
    // Trigger RSSI Measurement
    spiWrite(RFM69_REG_23_RSSI_CONFIG, RF_RSSI_START);
    // Wait for Measurement to complete
    while(!(RF_RSSI_DONE && spiRead(RFM69_REG_23_RSSI_CONFIG))) { };
    // Read, store in _lastRssi and return RSSI Value
    _lastRssi = -(spiRead(RFM69_REG_24_RSSI_VALUE)/2);
    return _lastRssi;
}
#endif
