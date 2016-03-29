#ifndef config_h_
#define config_h_

#include <jansson.h>
#include <string>

class Config {
	private:
	json_t *config;
	std::string filename;

	public:
	Config();
	~Config();
	void setFile(std::string file);
	void loadConfig();
	void saveConfig() const;

	const char* getDevice() const;
	const char* getNodename() const;
	std::string getLocation() const;
	unsigned int getTXFreq() const;
	bool enableRX() const;
	bool enableTX() const;
	bool enableUpload() const;
	bool enableRepeater() const;
	protected:
};

#endif
