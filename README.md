
# nrf24 - RF24 Radios in the node way

[![GitHub issues](https://img.shields.io/github/issues/ludiazv/node-nrf24.svg)](https://github.com/ludiazv/node-nrf24/issues)
[![Build Status](https://travis-ci.org/ludiazv/node-nrf24.svg?branch=master)](https://travis-ci.org/ludiazv/node-nrf24)
![npm](https://img.shields.io/npm/v/nrf24)
![GitHub tag (latest by date)](https://img.shields.io/github/v/tag/ludiazv/node-nrf24)



This module enable __nodejs__ (using javascript syntactic sugar) to manage __nRF24L01(+)__ radios on linux-based one board computers (RaspberryPi,OrangePi,BeagleBone,Edison,CHIP...). Wide support for SBCs is provided by the *RF24 library* supporting Generic Linux devices with SPIDEV, MRAA, RPi native via BCM* chips, WiringPi or using LittleWire.

This module is based on the outstanding C++ library optimized by @tmrh20. Please consult the project documentation **nRF24** [here](http://tmrh20.github.io/RF24/) for additional details.

This project has a sister project to enable __Node-RED__ to use __nRF24L01(+)__ radios in a seamless way with visual flow programming. Check out the repository [here](https://github.com/ludiazv/node-red-contrib-nrf24).

__nRF24L01__ present several problems when connected using long wires and poorly regulated power sources. To avoid this kind of problems is recommended to use break-out boards. For standard Pi-like
computers [this Hat/pHat](https://www.tindie.com/products/11790/) enable a simple hardware integration with minimal configuration. Full documentation of this openHw project can be consulted [here](https://github.com/ludiazv/borosRF2).

**BorosRF2 hat:**

![image](https://github.com/ludiazv/borosRF2/blob/master/media/all.jpg?raw=true)




If you **like** this project and want to support **the development and maintenance** please consider a donation.

<p align="center">
  <a href="https://www.buymeacoffee.com/boros" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-white.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>
</p>


## Overview

This nodejs add-on has the following features:

1. Basic & streaming __RF24__ radio support.
2. Create a __RF24Mesh__ network as master or Join to a *RF24Mesh* as node.
3. Provide a __RF24Gateway__ directly on nodeJs to provide TCP/IP connectivity for your radios. *[In development]*

## Installation

### Prerequisites:

#### SPI enabled & Gpio Access
In order to communicate with the radio linux kernel need to have SPI enabled and direct access to board GPIOs. In some distributions SPI interface is disabled by default so it's needed to be enabled.

In Rpi Raspbian it can be done using ``raspi-config`` or modifying `` /boot/config.txt ``. Check [here](https://www.raspberrypi.org/documentation/hardware/raspberrypi/spi/README.md) for more details.
For Rpi DietPi (recommended distribution for headless projects) the procedure is similar but using the ``/DietPi/config.txt``.

Gpio access require access to ``sysfs`` under the path ``/sys/class/gpio/``. In common linux distributions simply adding the user executing the nodejs application to the ``gpio`` group
check ``usermod``to do it.

In Rpi alternatives based on armbian such OrangePi, NaoPi,... please consult armbian documentation [here](https://docs.armbian.com/Hardware_Allwinner_overlays/).


For other SBCs or distributions consult who to activate SPI and Gpios.

If using Spidev driver (see below) the user executing the nodejs application need to have access to the /dev/spidevX.X files. This can be done in typical distribution adding the user to the ``spi`` group.

#### A working nRF24L01(+) radio

A *nRF24L01(+)* radio must be wired to your system and tested for connection. RF24* libs have several testing examples that can be use for for that purpose.

An example of wiring for an OrangePi Zero(using SPI 1 bus) and Rpi (using SPI 0 bus) is show in the following table:

| PIN NRF24L01 | OrangePi Zero Pin    |
| -------------- | ------------------ |
| 1 GND          |  opi-gnd (25)      |
| 2 VCC          |  opi-3v3 (17)      |
| 3 CE           |  opi-PA6 (7) (selectable)  |
| 4 CSN	         |  opi-spi1cs0 (24)  |
| 5 SCK	         |  opi-spi1Sck (14)  |
| 6 MOSI	       |  opi-spi1Mosi (19) |
| 7 MISO       |  opi-spi1Miso (16) |
| 8 IRQ          |  not connected / optional  |



| PIN	NRF24L01	| RPI	RPi -P1 Connector |
| ------------- | --------------------- |
| 1 GND	       | rpi-gnd	(25) |
| 2 VCC	      | rpi-3v3	(17) |
| 3 CE	       | rpi-gpio22	(15) |
| 4 CSN	       | rpi-gpio8	(24) |
| 5 SCK	       | rpi-sckl	(23) |
| 6 MOSI	     | rpi-mosi	(19) |
| 7 MISO	     | rpi-miso	(21) |
| 8 IRQ		     |  not connected  / optional |

Understand your wiring is critical for hardware initialization as there are not hardware discovery mechanism implemented in the library.

For better performance and lower CPU usage is recommended to connect also the __PIN 8 (IRQ line)__ an additional GPIO line. Check-out the ``config`` API addtional information about IRQ management. 

### Install the nrf24 package

Use NPM as with other modules:
```bash
cd <your project>
npm install nrf24 --save
```
This will add nrf24 to your project and save the dependence in your __package.json__

__disclaimer:__ This package is a C++ native add-on that will require compilation on your linux system. Please assure that you have the proper "build-essential","base-devel" packages installed according to your linux distribution.

By default the packages install will build the node module with **SPIDEV** driver. If other driver is preferred the install command must provide the ``env`` variable ``DRIVER=<driver>`` before the npm install command. For example if the driver to use is ``RPi`` install command is:

```bash
DRIVER=RPi npm install nrf24 --save
```

## Usage

### Basic RF24
Enable **nodejs** have basic communication over __nRF24L01(+)__ radios in a simple javascript code:

```javascript
...
const nrf24=require("nrf24"); // Load de module
// or
import * as nrf24 from 'nrf24'; // Load with import notation notation.

// Init the radio
var rf24= new nrf24.nRF24(<CE gpio>,<CS gpio>);
rf24.begin();
// Configure the radio
rf24.config({
  PALevel: nrf24.RF24_PA_LOW,
  DataRate: nrf24.RF24_1MBPS,
  ... // Other config
});

// Register Reading pipes
var pipe=rf24.addReadPipe("0x65646f4e31",true) // listen in pipe "0x65646f4e31" with AutoACK enabled.
...
//up to 5 pipes can be configured

// Register callback for reading
rf24.read( function(data,n) {
  // when data arrive on any registered pipe this function is called
  // data -> JS array of objects with the follwing props:
  //     [{pipe: pipe id, data: nodejs with the data },{ ... }, ...]
  // n -> number elements of data array    
},function(isStopped,by_user,error_count) {
  // This will be if the listening process is stopped.
});

...

// Sending data over the radio
var data=Buffer.from(<your data>); // Create a node buffer for sending data
rf24.useWritePipe("0x72646f4e31",true); // Select the pipe address to write with Autock
rf24.write(data); // send the data in sync mode

...
// Async write with callback
rf24.write(data,function(success,...){
  // Callback after sending data
});

...
// If you want to stop listening
rf24.stopRead();
...
// If you want to abort pending async writes
rf24.stopWrite();


// Finally to assure that object is destroyed
// and memory freed destroy must be called.
rf24.destroy();


```

### Streaming data
The maximun payload of NRF24 radios is 32 bytes. In order to transmit or recive streams bigger buffers of data the module offers some helpers for this operations. 

Streaming data can be acomplished by using stream function:
```javascript
rf24.changeWritePipe(true,2048); // Set max stream size to 2Kb

var big_buffer=Buffer.from(<data_up_to_2K>);
rf24.stream(big_buffer,function(success,tx_ok,tx_bytes,tx_req,frame_size,aborted) {
  if(success) console.log("Stream ok");
  else {
    console.log("Transfered:"+tx_bytes+ " total:" + tx_req + " OK->" + tx_ok);
  }
});
```

Receiving streaming data:

Receiving streaming data do not require special function. ``read()``function will receive the incoming frames. To improve perfomance incoming frames from the transmitter can be merged into a single buffer. The library will try to merge incoming frames and send then to the ```read() callback``` but is not guarteed that frames will be merged. User can configure maximun frames that are possible to merge by calling ```changeReadPipe()```

```javascript
rf24.changeReadPipe(pipenr,false,10); // Wil merge up to 10 frames with out 

```

### Peformance

Starting form version 1.0-beta the improvement of perfomance is significant. To archive better maximun performance **IRQ** line must be configured.

Check this [this](https://github.com/ludiazv/node-nrf24/blob/master/PERFORMANCE.md) page to check a baseline performed that can be archived with the library.

### Failure handling
Due to wrong wiring / weak signal quality or faulty hardware the module can detect when the radio is in a inconsistent status and is not able to transmit. User can check via ```hasFailure()``` API if this condition has been detected. Recovery of this situation could require to change wiring or improve power/decoupling for the radio module or simply change the radio module. 

__experimental__: Recovery of faulty hardware is supported in a experimental way by reseting the radio. If a radio is failing randomly but it perform well the user can call ```restart()``` API to reset the radio. All configuration will be restarted acording to user. Faliures can be only detected after writing.

### RF24Mesh

TODO documentation
....

### RF24RF24Gateway

TODO documentation
....


## API

### RF24 API

The module offer a simplified and basic functionality of the nRF24 library. Only basic operations are supported. Features such DynamicPayloads, AckPayloads, ... are not supported.

#### RF24(ce,cs,spi_speed=10000000) constructor
Create RF24 object that match the wiring. ce and cs values will vary depending on your
hardware and build o RF24 libs. Please refer to R24libs documentation [here](http://tmrh20.github.io/RF24/RPi.html)
for your specific case.

*Parameters:*

| Param | description |
| ----- | ----------- |
| ce    | GPIO # for CE pin |
| cs    | SPI Chip Select pin. For SPIDEV the mapping is (a * 10 + b)  for /dev/spideva.b |
| spi_speed | Speed of the SPI interface in Hz. Defaults to 10000000 (10Mhz) |

Both parameters are mandatory. Constructor must be called with __new__ keyword to create a new RF24 object, otherwise thiscall will produce an exception.

CE could be any available GPIO , CS pins on the contrary need to be pins that are configured in the kernel as part of the SPI interface.

*Example:*
```javascript
// For RPI
var rf24=new nrf24.nRF24(22,0); // Rpi SPI0 with CE in PIN 15/GPIO 22 and CS0

// for SPIDEV
var rf24=new nrf24.nRF24(6,10); // OrangePi SPI1 CE=PA06 and CS=<a>*10+<b> for /dev/spidev<a>.<b>

```
*Returns*: RF24 Object

*Throws:*: TypeError Exception if the parameters CS or CE are invalid.

#### destroy() Explicit destructor
Destroy the object __This method must be called__ if its needed to reclaim the memory and resources. If there is any pending reads on the reading queue or pending async writes will be cancelled.

*Example:*

```javascript
let rf24=new nrf24.nRF24(22,0);
...
// Do radio work

rf24.destroy(); // After this call the GC will reclaim the object if needed.
// The object will become unusable at this point.
```

*Returns*: none.
*Throws:*: none.

#### begin(print_debug=false)
After the creation of the object it is required to call to begin. If begin initializes the hardware for transmitting an receiving.

*Parameters:*

| Param | description |
| ----- | ----------- |
| print_debug (optional)    | true/false prints RF24 debug information in stdout (it will be useful for debugging  purposes). default=false |

*Example:*

```javascript
var rf24=new nrf24.nRF24(22,0);
rf24.begin();

```

*returns:* true/false success

*throws:* nothing


#### config(options,print_details=false)
Configure the radio with some generally used configuration parameters. This function must be called after calling begin().

*Parameters:*
 A sole parameter options is passed for configuration. options must be javascript object with the following properties:

| Property | description (valid values) | Default Value |
| -----    | ----------- | ------------- |
| PALevel  | Transmission power (MIN, LOW, HIGH, MAX) | MIN |
| EnableLna| Enable LNA amplifier if the radio module has it | true |
| DataRate | Speed of transmission bps (2MBPS,1MBPS,250KB)            | 1MBPS |
| Channel  | Radio Channel (1-127)  | 76 |
| CRCLength| Length of CRC checksum for data integrity checking (8bit,16bit, disabled)  | CRC 16 bit  |
| retriesCount  |  Number of transmission retries (0-15)   | 5 |
| retriesDelay   | Time (n+1)*250us to wait between transmission retries (0-15) | 15 |
| PayloadSize  | Size of the radio frame (1-32)  | 32 |
| AddressWidth  | Radio Pipes address size (3,4,5)  | 5  |
| AutoAck  | Use Built-in ack true/false | true |
| Irq | Gpio number of IRQ line (if negative IRQ line is not ussed) | -1 |
| PollBaseTime | If IRQ is not used this the reference time in us for polling reads (>4000) | 35000 |
| TxDelay | Grace time peridod when switching from listening to writing mode in us (>4) | 250 |

All options Property are **optional**. Default values described above are used if not present in the options object. If any parameter is not valid default values for the parameters are used instead.

node-rf24 exports some constants to help to define:

| Item | Values |
| ---- | ------ |
| Speed | RF24_1MBPS, RF24_2MBPS,RF24_250KBPS |
| Power Level  |  RF24_PA_MIN,RF24_PA_LOW,RF24_PA_HIGH,RF24_PA_HIGH, RF24_PA_MAX ,RF24_PA_ULTRA |
| CRC Checking  | RF24_CRC_DISABLED,RF24_CRC_8,RF24_CRC_16  |

__note__: 250KBPS seed is only available in nRF24L01+ variant. If your radio is a nRF24L01 this speed option will not work.

__note__: PA_ULTRA is the same as RF24_PA_MAX starting form version 0.1.6 for comaptibility reasons with previous versions.

The power levels correspond to the following output levels respectively:

NRF24L01: -18dBm (MIN), -12dBm(LOW),-6dBM(HIGH), and 0dBm(MAX), EnableLna affects modules with LNA

SI24R1:
- 6dBm (MIN), 0dBm (LOW), 3dBm(HIGH)  and 7dBm(MAX) with EnableLna = true
- 12dBm (MIN),-4dBm (LOW), 1dBm(HIGH) and 4dBm(MAX) with EnableLna = false



The second parameter ``print_details`` is optional boolean to show in stdout the configuration of the radio after config is executed. This is useful for debugging.

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

#### present()
Check if radio is connected. This function must be called after begin to work properly.

*parameters*: none.

*Example:*
```javascript
rf24.begin();
console.log("Radio connected:" + rf24.present()); // Prints true/false if radio is connected.

```
*Returns*: true/false (if radio is connected)

*Throws*: nothing.

#### isP()
Check if the connected radio is nRF24L01+ variant.This function must be called after begin to work properly.

*parameters*: none.

*Example:*
```javascript
rf24.begin();
console.log("Is + Variant:" + rf24.isP()); // Prints true/false if radio is + Variant

```
*Returns*: true/false (if radio is + variant)

*Throws*: nothing.

#### powerUp() / powerDown()
Disable/enable radio saving energy when not in use. By Default after ```begin()```the radio will be powered up. Note that no information can be received or transmitted but in exchange the power consumption of the radio will be less than 1uA(sleep mode).

Note that if you are listening for radio frames via ```read()``` API you must call ```stopRead()``` before put the radio to sleep. Conversely if there are pending async writes the API ``stopWrite()`` must be called before the radio can be powered down.


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
rf24.stopRead();
rf24.stopWrite();
rf24.powerDown();
// Radio is asleep

```
*Returns*: nothing.

*Throws*: nothing.

#### addReadPipe(addr,auto_ack=default value)
Before being able to receive any frame a **reading pipe** must be registered. A reading pipe is a one-way address-based radio tunnel that need to be used by the transmitter to communicate with the receiver. In a typical radio communication the receiver will 'listen' on a predefined pipe address and the transmitter will 'send' to the same predefined address.

For a two-way communication to be possible the transmitter reading pipe needs to be the same to the receiver writing pipe and vice versa. This two cross-pipe configuration enable seamless two-way transmissions.

nRF24 library provide 5 available reading pipes to listen numbered from 1 to 5.

>__caveat__:
> First Reading pipe registered can have any valid address (pipe 1). Pipes 2-5 addresses __must__ have the same first bytes equal to the 1st reading pipe and can only change the last byte of the address.



*parameters*:

| parameter | description |
| --------- | ----------- |
| addr      | a **string** with the hexadecimal format "0x#######". If the radio is configured with AddressWidth=5 the string must have 12 char. if AddressWidth=4 10 chars and if AddressWidth=3 8 chars. |
| auto_ack (optional) | true/false nRF24 support to send ACK to the transmitter by the hardware. |

if auto_ack is not provided the default value provided in *config* will be used.

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


#### removeReadPipe(pipenr)
Deregister a pipe to read from. This function can be used to reconfigure close reading pipes registered with addReadPipe.

*parameters*:

| parameter | description |
| --------- | ----------- |
| pipenr    | pipe number descriptor returned by addReadPipe   |

*returns:* nothing

*throws:* SyntaxError if parameters are invalid.

#### changeReadPipe(pipenr,auto_ack,maxmerge)
Change the configuration of the a reading pipe. It should be called after ``addReadPipe`` **only**.

*parameters*:

| parameter | description |
| --------- | ----------- |
| pipenr    | pipe number descriptor returned by addReadPipe   |
| auto_ack  | true/false nRF24 support to send ACK to the transmitter by the hardware. |
| maxmerge  | Maximum number of frames to merge in a single read event on the pipe. |

 maxmerge is special parameter that can be used in streaming use cases. This parameter inform the reading loop that the user will accept *merging* consecutive frames on the pipe (up to the maximum number of frames specified). __Note__ that this value is maximum value and do not guaranteed that the incoming frames will be merged.

*example:*

```javascript

var pipenr=rf24.addReadPipe("0xABCD11FF55");
if(pipenr>0) {
   rf24.changeReadPipe(pipenr,false,10);
   // Disble Ack and allow the libray to merge up to 10 consecutive frames on the pipe.
}


```

 *returns:* nothing.

 *throws:* SyntaxError if parameters are not valid.

#### read(rcv_callback,stop_callback)
Register async callback for receiving information over the radio. This will enable the radio to receive frames from other radios. Reading pipes registered via addReadPipe will be used.

__Note:__ Have in mind that read callback is unique for all pipes and is not possible to register several call-backs. If read is called multiple times only the first one will used. To register another callback ```stop_read()``` must be called and then another call to ```read()``` will be successful.

*parameters*:

| parameter | description |
| --------- | ----------- |
| rcv_callback  |  function(array data, int items)  that will called back on reception of any valid frame from the radio.|
| stop_callback   | function(bool stopped,bool by_user, int error_count) that will be called back when by some reason we have stopped listening.    |

both parameters are mandatory.

> __caveat__: This API has change from previous verions. Now it allow to perform only one callback for all pending packets at once. This will increase the performance of the library reducing the number of callbacks to the js userland.

*example*:

```javascript
rf24.begin();
var pipe1=rf24.addReadPipe("0xABCD11FF55");
var pipe2=rf24.addReadPipe("0xABCD11FF56");

rf24.read( function (data,items) {
  for(var i=0;i<items;i++) {
    if(data[i].pipe==pipe1) {
      // received from 0xABCD11FF55
      // data[i].data will contain a buffer with the data
    ...
  }else if (data[i].pipe == pipe2) {
    // rcv from 0xABCD11FF56
    }
  }
},
function(stop,by_user,err_count) {
  ...
});

```

*returns*: nothing

*Throws:*: SyntaxError if parameters are invalid.

#### stopRead()
Stop listening for radio frames from the radio. This function is intended halt any further reading, however, as read mechanism is async it is possible that any pending read is delivered to reading callback. After successful halt of reading operation a ``` stop_callback ``` will be called, at this point no further frames will be received in ```rcv_callback ```.

This function will take more than 200ms to complete blocking your JS code. Therefore it should be used mainly to shut down communication not being designed for switching purposes.

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
rf24.stopRead();

```

*returns:* nothing.

*throws:* nothing.

#### useWritePipe(addrs,auto_ack=default_value)
Define the output pipe to write to. __nRF24L01__ have only one output pipe with
pipe number *0*. This call is the counter part of *addReadPipe* in the transmitter side.
All comments about pipes explained there applies also to this pipe.

*parameters*:

| parameter | description |
| --------- | ----------- |
| addr      | a **string** with the hexadecimal format "0x#######". If the radio is configured with AddressWidth=5 the string must have 12 char. if AddressWidth=4 10 chars and if AddressWidth=3 8 chars. |
| auto_ack (optional) | true/false nRF24 support to wait to ACK from the transmitter by the hardware. |

if auto_ack is used the default value provided in *config* will be used.

*Example*:

```javascript
rf24.begin();
rf24.useWritePipe("0xABCD11FF55");
...
rf24.write(...) // This will be written to '0xABCD11FF55'

```

*Returns*: nothing

*Throws*: SyntaxError if parameters are invalid.

#### changeWritePipe(auto_ack,stream_maxsize)
Changes write pipe configuration. This API **must** be called after ``useWritePipe``.

*parameters*:

| parameter | descripton |
| --------- | ---------- |
| auto_ack  | true/false nRF24 support to wait to ACK from the transmitter by the hardware |
| stream_maxsize | Maximum size in bytes of buffer in stream mode.

The maximum buffer is reset to a default value of ``1204`` bytes after calling to ``useWritePipe`` so to change it this function must be called every time the writting pipe is changed. Note also that the ``write()`` function ignores the parameter ``stream_maxsize`` and it will be only applicable using the ``stream()`` API (see below).

*returns*: Nothing

*throws:* SyntaxError if parameters are invalid.

#### write(buffer,wr_callback *optional*)
Send data over the radio in sync or async mode. If sync mode used ``wr_callback`` function must not be provided but the execution of JS code will block until transfer is finished. This could take up to 4ms for a 32 byte frame that could be acceptable or not depending on the use-case.

*parameters*:

| parameter | description |
| ----------| ----------- |
| buffer    | node js buffer with the data to transmit. |
| wr_callback| callback function for async write |

if buffer size is > Payload size the call will not fail but only the bytes in the buffer that fit in one frame will be transmitted.

wr_callback is a regular js callaback with the following parameters:
- success: true/false
- tx_ok: number of frames transmitted ok.
- tx_bytes: number of bytes transmitted ok.
- tx_req: number of frames requested to transmit.
- frame_size: size of the frame in bytes
- abort: true/false if the transmission was aborted by user.

*example*:
sync mode:

```javascript
if(rf24.write(Buffer.from(<data>)))
    console.log("WR OK!");
else
    console.log("Failed!");

```

asyc mode:

```javascript
rf24.write(Buffer.from(<data>),function(success,...){
  if(success) console.log("WR OK!"); else console.log("Failed!");
});
```

*returns*: true/false success for sync write.
           true/false async job successfully created.

*throws*: SyntaxError if parameters are invalid.

#### stream(buffer,wr_callback)
Stream has the same semantics of write but it has its differences:

- Stream can send bigger buffers. Up to the size defined in ``stream_maxsize`` parameter of the ``changeWritePipe`` API call.
- Stream is async **only**.
- If auto ACK is enabled if any frame in the stream failed(after predefined retries) the whole buffer will be fail. This mean that the receiver will receive frames until the first fail.

*parameters:*

| parameters | description |
| ---------- | ----------- |
| buffer     | nodejs buffer with the data. If buffer is bigger than ``stream_maxsize`` the call will not fail but only the ``stream_maxsize`` will be transmitted. |
| wr_callback | callback function for async write |

``wr_callback`` has the same specification of write function.

*example*:
```javascript
rf24.changeWritePipe(true,2048); // Set max stream size to 2Kb
var big_buffer=Buffer.from(<data_up_to_2K>);
rf24.stream(big_buffer,function(success,tx_ok,tx_bytes,tx_req,frame_size,aborted) {
  if(success) console.log("Stream ok");
  else {
    console.log("Transfered:"+tx_bytes+ " total:" + tx_req + " OK->" + tx_ok);
  }
});
```

*returns*: true/false async job successfully created.

*throws*: SyntaxError if parameters are invalid.

#### stopWrite()
Abort any pending async write. If the transmission has been already started the write will not be aborted, only pending writes.

If write is aborted by the parameter ``abort`` in the write/stream callback functions will be ``true``.

*returns*: Nothing.
*throws*: Nothing.

#### hasFailure()
Get a boolenan indicator if failure has been detected in the radio. (**This is experimental feautre**)

*returns*: true/false 
*throws*: Nothing

### restart()
Reinit the radio maintaining the configuration and open pipes. Calling this funcion after detecting a failure may reset the de radio
and enable graceful recovery of comunications. (**This is experimental feature**)

*returns*: nothing
*throws*: nothing

#### getStats(*optional* pipe)
Get internal transfer stats. *#* represents the number of frames received or transmitted.

If pipe is not provided the follwing object is returned:
```javascript
rf24.getStats();
// => { TotalRx: # , TotalTx_Ok: # , TotalTx_Err: # , PipesRx:[#,#,...] , Failure: # }

```

if pipe=0 the stats retuned are those of the writing pipe with the follwing format:
```javascript
rf24.getStats(0);
//=>{  TotalTx_Ok: # ,TotalTx_Err: # }
```
if pipe >0 && <5 a number with the number of frames received is returned.
```javascript
rf24.getStats(pipe)
// => #
```

*returns*: number of frames recevied if pipe>1, if pipe=0 or not provided object with the stats.

*throws*: Nothing.

#### resetStats(*optional* pipe)
Reset to 0 internal transfer stats. if pipe is not selected all stats will be
cleared.

| parameter | description |
| ----------| ----------- |
|  pipe | pipe nr to reset if not set all pipes will be cleared. pipe=0 write pipe / pipe=RF24_FAILURE_STAT reset failure counter  |

*returns*: nothing.
*throws*: nothing.

## RF24Mesh API

TODO

## RF24Gateway API

TODO


# TODO

- ~~Assure RF24Lib is built with FAILURE_HANDLING~~
- ~~Implement FAILURE_HANDLING auto-recovery.~~
- ~~Implement FAILURE stats.~~
- ~~Test bindings~~
- ~~Migrate to NAN >2.8 to support queued async msg passing.~~
- ~~Implement setWriteAck for write pipe.~~
- ~~Change build script to not install globally the libraries (.so) in the system and link them locally inside the package (rpath)~~
- Implement MeshStats and non blocking behavior for begin,nodeID and Send.
- Remove try_abort hack on mesh and gateway.
- Refactor Mesh for async only API
- Document Mesh
- Document Gateway.
- ~~Get rid off try_abort hack (pending nRF24 lib release)~~
- ~~Benchmark transfer speeds.~~
- Implement CPU-friendly streaming (pending nRF24 lib release)
- ~~Implement IRQ management.~~
- ~~Benchmark IRQ performance.~~ -> Will respond on 300-400us basis. (1-2 retries)
- ~~Implement traffic stats~~
- ~~Update base RF24 library to new verions (1.3.3 & 1.3.4 do not work)
- Distribute arm32 & arm64 prebuilds with prebuidify.

# Change log

- v0.1.6-beta
  - Bump of base libraries to version 1.3.9 (RF24), 1.0.13 (RF24N) , 1.1.3(RF24M) #20
  - Speed of SPI interface is now configurable in the constructor. #15
  - Now the module is tested to compile against node versions 10, 12, 14 on arm32 and arm64 environments.
  - Fix compilation on node version 8 
  - Added EnableLna for better support radio boards with amplifiers and Si24R1 clones.
  - Some doc improvements

- v0.1.5-beta
  - Now RFLibs are linked statically avoiding to install library system-wide.
  - Improved documentation + added experimental support for autoRecovery.
  - Now the library is compatible and tested with node lts versions: 8 , 10 , 12
  - Added travis automatic build to test compilation in all supported node versions.
  - Fixes in v8 data conversion using isolates and Nan helpers.
  - As nan-marshal is not updated the module do not depend on this module anymore: Nan-marshal code is include with the project as a header file and has been adpated to have compativility with node 10 and 12.
- v0.1.4-beta
  - Fix build pull request #14
- v0.1.2-beta
  - Bump to version of mesh library to 1.0.7 (#12)
- v0.1.1-beta
  - Documentation improvements
  - Compilation of RFLibs to specific releases to assure stability
  - Force compilitation of RF24Lib with FAILURE_HANDLING activated.
  - Refactor IRQ management.
  - Added new Api ```hasFailure```
  - ```getStats``` & ```resetStats``` report failure detection stats.
- v0.1.0-beta
  - Documentation improvements.
  - Make nrf24 objects persistent with Ref(). Added destroy() method to Unref() and free object.
  - IRQ Line support.
  - Implement basic RF24 statistics.
  - Polling time can be configured via ``config``
  - Configure TxDelay via ``config``
  - Implement async write and steam write.
  - __This Release include breaking changes in read API__


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
