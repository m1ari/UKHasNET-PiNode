Basic Pi based node for the [UKHASnet](http://ukhas.net) network using a RFM69.

## Installation
You will probably need the following packages to compile the code

* To use JSON based config
  * for wheezy based distributions use `sudo apt-get install libjansson libjansson-dev`
  * on jessie you need to use `sudo apt-get install libjansson4 libjansson-dev` instead

* To upload to the UKHASnet server (Act as a gateway node)
```
sudo apt-get install libcurl4-openssl-dev
```

At present both are required but a later release may allow building without some options (most code is already in place to replace the JSON config with #defines)

On Raspbian you will need to enable the SPI kernel driver in raspi-config for this to work
