
# nrf24 - RF24 Radios in the node way

[![GitHub issues](https://img.shields.io/github/issues/ludiazv/node-nrf24.svg)](https://github.com/ludiazv/node-nrf24/issues)

This module enable __nodejs__ (using javascript syntactic sugar) to manage __nRF24L01(+)__ radios on linux-based one board computers (RaspberryPi,OrangePi,BeagleBone,Edison,CHIP...). Wide support for SBCs is provided by the *RF24 library* supporting Generic Linux devices with SPIDEV, MRAA, RPi native via BCM* chips, WiringPi or using LittleWire.

This module is based on the outstanding C++ library optimized by @tmrh20. Please consult this project documentation **nRF24** [here](http://tmrh20.github.io/RF24/) for additional details.

This project has a sister project to enable **Node-RED** to use __nRF24L01(+)__ radios in a seamless way with visual flow programming. Check out the repository [here](https://github.com/ludiazv/node-red-contrib-nrf24).


## Overview

This nodejs add-on has the following features:

1. Basic __RF24__ radio support.
2. Create a __RF24Mesh__ network as master or Join to a *RF24Mesh* as node.
3. Provide a __RF24Gateway__ directly on nodeJs to provide TCP/IP connectivity for your radios. *[In development]*


## Installation

### Prerequisites:

#### SPI enabled
In order to communicate with the radio linux kernel need to have SPI enabled. In some distributions SPI interface is disabled by default so it's needed to be enabled.

In Rpi Raspbian it can be done using ``raspi-config`` or modifing `` /boot/config.txt ``. Check [here](https://www.raspberrypi.org/documentation/hardware/raspberrypi/spi/README.md) for more details.
For Rpi DietPi (recommended distribution for headless projects) the procedure is similar.

In Rpi alternatives based on armbian such OrangePi, NaoPi,... please consult armbian documentation [here](https://docs.armbian.com/Hardware_Allwinner_overlays/).

For other SBCs or distributions consult who to activate SPI.

If using Spidev driver (see below) the user executing the nodejs application need to have access to the /dev/spidevX.X files. This can be done in typical distribution adding the user to the ``spi`` group.

#### A working nRF24L01(+) radio

A *nRF24L01(+)* radio must be wired to your system and tested for connection. RF24* libs have several testing examples that can be use for for that purpose.

An example of wiring for an OrangePi Zero(using SPI 1 bus) and Rpi (using SPI 0 bus) is show in the following table:

| PIN NRF24L01 | OrangePi Zero Pin |
| -------------- | ----------------- |
| 1 GND          |  opi-gnd (25)
| 2 VCC          |  opi-3v3 (17)
| 3	CE	         |  opi-PA6	(7)
| 4	CSN	         |  opi-spi1cs0 (24)
| 5	SCK	         |  opi-spi1Sck (14)
| 6	MOSI	       |  opi-spi1Mosi (19)
| 7	MISO	       |  opi-spi1Miso (16)
| 8	IRQ		       |  NC              

| PIN	NRF24L01	| RPI	RPi -P1 Connector |
| ------------- | --------------------- |
| 1	GND	        | rpi-gnd	(25)
| 2	VCC	        | rpi-3v3	(17)
| 3	CE	        | rpi-gpio22	(15)
| 4	CSN	        | rpi-gpio8	(24)
| 5	SCK	        | rpi-sckl	(23)
| 6	MOSI	      | rpi-mosi	(19)
| 7	MISO	      | rpi-miso	(21)
| 8	IRQ		      |  NC              

Understand your wiring is critical for hardware initialization as there are not hardware discovery mechanism implemented in the library.

__note__: IRQ pins are not used in current version but planned.


#### RF24* libraries installed

RF24* libs must be installed in the system. Please check out  [this](http://tmrh20.github.io/RF24/Linux.html) for detailed installation procedure.

You can use the installation script in this repository:

```
# cd $HOME
# ./build_rf24libs.sh
```

it should work on a typical linux environments. nRF24 c++ support different drivers. SPIDEV driver is recommended as this this the standard portable SPI. But in principle all drived provides _should_ work in a similar way. This nodejs package has been tested using Rpi driver and Spidev.

### Install the nrf24 package

Use NPM as with other modules:
```
cd <your project>
npm install nrf24 --save
```
This will add nrf24 to your project and save the dependence in your __package.json__

__disclaimer:__ This package is a C++ native add-on that will require compilation on your linux system. Please assure that you have the proper "build-essentials" packages installed according to your linux distribution.

## Usage

### Basic RF24
Enable **nodejs** have basic communication over __nRF24L01(+)__ radios in  a simple javascript code:

```javascript
...
var nrf24=require("nRF24"); // Load de module
// Init the radio
var rf24= new nrf24.RF24(<CE gpio>,<CS gpio>);
rf24.begin();
// Configure the radio
rf24.config({
  PALevel: nrf24.RF24_PA_LOW,
  DataRate: nrf24.RF24_1MBPS,
  ...
});

// Register Reading pipes
var pipe=rf24.addReadPipe("0x65646f4e31",true) // listen in pipe "0x65646f4e31" with AutoACK enabled.
...
//up to 5 pipes can be configured

// Register callback for reading
rf24.read( function(data,pipe) {
  // when data arrive on any registered pipe this function is called
  // data -> node buffer with the data
  // pipe -> number of the pipe    
},function(isStopped,by_user,error_count) {
  // This will be if the listening process is stoped.
});

...

// Sending data over the radio
var data=Buffer.from(<your data>); // Create a node buffer for sending data
rf24.useWritePipe("0x72646f4e31"); // Select the pipe address to write
rf24.write(data); // send the data

...

// If you want to stop listening
rf24.stop_read();


```

### RF24Mesh

TODO documentation
....

### RF24RF24Gateway

TODO documentation
....

## API

### RF24 API

The module offer a simplified and basic functionality of the nRF24 library. Only basic operations are supported. Features such DynamicPayloads, AckPayloads, buffer manipulation, ... are not supported.

#### RF24(ce,cs) constructor
Create RF24 object that match the wiring. ce and cs values will vary depending on your
hardware and build o RF24 libs. Please refer to R24libs documentation [here](http://tmrh20.github.io/RF24/RPi.html)
for your specific case.

*Parameters:*

| Param | description |
| ----- | ----------- |
| ce    | GPIO # for CE pin |
| cs    | SPI Chip Select pin. For SPIDEV the mapping is (a * 10 + b)  for /dev/spideva.b

Both parameters are mandatory. Constructor must be called with __new__ keyword to create a new RF24 object, otherwise the call will produce an exception.

*Example:*
```javascript
// For RPI
var rf24=new nrf24.RF24(22,0); // Rpi SPI0 with CE in PIN 15/GPIO 22 and CS0

// for SPIDEV
var rf24=new nrf24.RF24(6,10); // OrangePi SPI1 CE=PA06 and CS=<a>*10+<b> for /dev/spidev<a>.<b>

```
*Returns*: RF24 Object
*Throws:*: TypeError Exception if the parameters CS or CE are invalid.

#### begin(print_debug=false)
After the creation of the object it is required to call to begin. If begin initializes the hardware for transmitting an receiving.

*Parameters:*

| Param | description |
| ----- | ----------- |
| print_debug (optional)    | true/false prints RF24 debug information in stdout (it will be usefull for debuging and wiring cheking purposes). default=false

*Example:*

```javascript
var rf24=new nrf24.RF24(22,0);
rf24.begin();

```

*returns:* true/false success
*throws:* nothing



#### config(options)
Configure the radio with some generally used configuration parameters. This function must be called after calling begin().

*Parameters:*
 A sole parameter options is passed for configuration. options must be javascript object with the following properties:

| Property | description (valid values) | Default Value |
| -----    | ----------- | ------------- |
| PALevel  | Transmission power (MIN, LOW, HIGH, MAX) | MIN
| DataRate | Speed of transmission bps (2MBPS,1MBPS,250KB)            | 1MBPS |
| Channel  | Radio Channel (1-127)  | 76 |
| CRCLength| Length of CRC checksum for data integrity checking (8bit,16bit, disabled)  | CRC 16 bit  |
| retriesCount  |  Number of transmission retries (5-15)   | 5 |
| retriesDelay   | Time (us) to wait between transmission retries  | 15 |
| PayloadSize  | Size of the radio frame (1-32)  | 32 |
| AddressWidth  | Radio Pipes address size (3,4,5)  | 5  |

All options Property are **optional**. Default values described above are used if not present in the options object.

node-rf24 exports some constants to help to define:

| Item | Values |
| ---- | ------ |
| Speed | RF24_1MBPS, RF24_2MBPS,RF24_250KBPS
| Power Level  |  RF24_PA_MIN,RF24_PA_LOW,RF24_PA_HIGH,RF24_PA_HIGH, RF24_PA_MAX |
| CRC Checking  | RF24_CRC_DISABLED,RF24_CRC_8,RF24_CRC_16  |

__note__: 250KBPS seed is only available in nRF24L01+ variant. If your radio is a nRF24L01 this speed option will not work.

*Example:*

```javascript
rf24.begin();
/* set Powe Level to maximum, set transfer speed to 250KBPS and on channel 101 */
var conf={
  PALevel:nrf24.RF24_PA_MAX,
  DataRate: nRF24.RF24_250KBPS,
  Channel: 101
};
rf24.config(conf); // Configuration will be applied.

```
*Returns:* nothing

*Throws:* Syntax Error exception if errors of on parameters are found.

### present()
Check if radio is connected. This function must be called after begin to work properly.

*parameters*: none.

*Example:*
```javascript
rf24.begin();
console.log("Radio connected:" + rf24.present()); // Prints true/false if radio is connected.

```
*Returns*: true/false (if radio is connected)

*Throws*: nothing.

### isP()
Check if the connected radio is nRF24L01+ variant.This function must be called after begin to work properly.

*parameters*: none.

*Example:*
```javascript
rf24.begin();
console.log("Is + Variant:" + rf24.isP()); // Prints true/false if radio is + Variant

```
*Returns*: true/false (if radio is + variant)

*Throws*: nothing.

### powerUp() / powerDown()
Disable/enable radio saving energy when not in use. By Default after ```begin()```the radio will be powered up. Note that no information can be received or transmitted but in exchange the power consumption of the radio will be less than 1uA(sleep mode).

Note that if you are listening for radio frames via ```read()``` API you must call ```stop_read()``` before put the radio to sleep.


*parameters*: none.

*Example:*
```javascript
rf24.begin();
// radio is UP
rf24.powerDown();
// Radio is asleep
...
rf24.read(...);
// After registeting reading callbacks radio will remain allwayson
...
rf24.stop_read();
rf24.powerDown();
// Radio is asleep

```
*Returns*: nothing.

*Throws*: nothing.

### addReadPipe(addr,auto_ack)
Before being able to receive any frame a **reading pipe** must be registered. A reading pipe is a one-way address-based radio tunnel that need to be used by the transmitter to communicate with the receiver. In a typical radio communication the receiver will 'listen' on a predefined pipe address and the transmitter will 'send' to the same predefined address.

For a two-way communication to be possible the transmitter reading pipe needs to be the same to the receiver writing pipe and vice versa. This two cross-pipe configuration enable seamless two-way transmissions.

nRF24 library provide 5 available reading pipes to listen numbered from 1 to 5.

>__caveat__:
> First Reading pipe registered can have any valid address (pipe 1). Pipes 2-5 are restricted to change only the last byte of the address.



*parameters*:

| parameter | description |
| --------- | ----------- |
| addr      | a **string** with the hexadecimal format "0x#######". If the radio is configured with AddressWidth=5 the string must have 12 char. if AddressWidth=4 10 chars and if AddressWidth=3 8 chars.
| auto_ack  | true/false nRF24 support to send ACK to the transmitter by the hardware.

both parameters are mandatory.

*Example*:
```javascript
rf24.begin();
var pipenr=rf24.addReadPipe("0xABCD11FF55");
if(pipenr==-1) console.log("Error!!!");
else console.log("Pipe opened" + pipenr);
...

```
*Returns*: (number) -1= No pipe is available. 1-5 number of pipe opened to read.

*Throws*: SyntaxError if parameters are invalid.


### removeReadPipe(pipenr)
Deregister a pipe to read from. This function can be used to reconfigure close reading pipes registered with addReadPipe.

*parameters*:

| parameter | description |
| --------- | ----------- |
| pipenr    | pipe number descriptor returned by addReadPipe   |

*returns:* nothing

*throws:* SyntaxError if parameters are invalid.


### read(rcv_callback,stop_callback)
Register async callback for receiving information over the radio. This will enable the radio to receive frames from other radios. Reading pipes registered via addReadPipe will be used.

__Note:__ Have in mind that read callback is unique for all pipes and is not possible to register several call-backs. If read is called multiple times only the first one will used. To register another callback ```stop_read()``` must be called and then another call to ```read()``` will be successful.

*parameters*:

| parameter | description |
| --------- | ----------- |
| rcv_callback  |  function(Buffer data, int pipenr)  that will called back on reception of any valid frame from the radio. data is Buffer , pipenr is the pipe descriptor|
| stop_callback   | function(bool stopped,bool by_user, int error_count) that will be called back when by some reason we have stopped listening.    |

both parameters are mandatory.

*example*:

```javascript
rf24.begin();
var pipe1=rf24.addReadPipe("0xABCD11FF55");
var pipe2=rf24.addReadPipe("0xABCD11FF56");

rf24.read( function (data,pipe) {
  if(pipe==pipe1) {
    // received from 0xABCD11FF55
    ...
  }else if (pipe == pipe2) {
    // rcv from 0xABCD11FF56
  }

},
function(stop,by_user,err_count) {
  ...
});

```

*returns*: nothing

*Throws:*: SyntaxError if parameters are invalid.

### stop_read()
Stop listening for radio frames from the radio. This function is intended halt any further reading, however, as read mechanism is async it is possible that any pending read is delivered to reading callback. After successful halt of reading operation a ``` stop_callback ``` will be called, at this point no further frames will be received in ```rcv_callback ```.

This function will take about 200ms to complete blocking your JS code. Therefore it should be used mainly to shut down communication not being designed for switching purposes.

This function is also called automatically upon object destruction.

__Note: To send frames is NOT needed to stop reading.__

*parameters:* none.

*example:*

```javascript
...
rf24.read( ... , ... );

// At this point rcv_callback is called once a frame is received.

...
// other code
...

// Stop reading
rf24.stop_read();

```

*returns:* nothing.

*throws:* nothing.


### useWritePipe(addrs)
Define the output pipe to write to. __nRF24L01__ have only one



### write(buffer)





## RF24Mesh API

TODO

## RF24Gateway API

TODO


# TODO

- ~~Test bindings~~
- ~~Migrate to NAN >2.8 to support queued async msg passing.~~
- Document Mesh
- Document Gateway.
- Get rid off try_abort hack (pending nRF24 lib merge)
- Implement IRQ management.
- Benchmark IRQ performance.

# Change log

- v0.0.0-alpha4
  - Documentation improvements.
  - Basic RF24 additional Testing
  - Migrated to NAN 2.10.0: Async workers now are queued so no data loss in nodejs.
  - Cleaned RF24Mesh for after testing.
  - Test/Example modification to arduino RF24 common pins.
  - Added License

- v0.0.0-alpha3
  - Include bindings package to resolve package name
  - Basic RF24 Working stable.
  - RF24Mesh in testing.
  - RF24Gateway in development.

- v0.0.0-alpha1 & 2 versions: Initial implementation of Basic Radio.
