Basic Pi based node for the [UKHASnet](http://ukhas.net) network using a RFM69.

## Installation
You will probably need the following packages to compile the code

* To use JSON based config
```
sudo apt-get install libjansson libjansson-dev
```

* To upload to the UKHASnet server (Act as a gateway node)
```
sudo apt-get install libcurl4-openssl-dev
```

At present both are required but a later release may allow building without some options (most code is already in place to replace the JSON config with #defines)
