# i2con
I2C Over Network (i2con) is a simple client/server that allows a remote client to send/receive I2C 
requests over the network to a device that has an I2C bus.  The server is a linux application 
intended for a Raspberry Pi platform.  This has only been tested on a Pi 2 B+.

Currently, only byte (8-bit) and word (16-bit) I2C read/writes are supported by the server.  The
protocol implemented by the server allows for multi-byte transfers, but this isn't implemented. 
I2con handles byte ordering for 2-byte read/writes; for larger transfers using a buffer (which is 
not implemented), the client must ensure proper byte ordering for the target I2C device.

**Disclaimer:** This code is really just for reference purposes and most likely has bugs.  

## Server
The server supports multiple active connections (the kernel drivers manages multiple requests to the
I2C bus).  

### Dependencies
The server uses the SMBus interface to access I2C.  Depending on the linux distro on the Pi, the 
`libi2c-dev` package may need to be installed.  If the server fails to compile due to missing smbus
functions, this is probably why -- try installing the package with `sudo apt-get install libi2c-dev`.

### Building the Server
Building the server uses CMake.  In the root directory where you have the i2con source files, the
following should get the server built:

```
> mkdir build
> cd build
> cmake ..
> make i2consrv
```

To run, just enter `./i2consrv` from the command line and `CTRL-C` to stop it.

## Client
The client is basic reference code for communicating with the i2con server.  Any platform implementing
Posix sockets should be able to communicate with the server.

### Sample Client
The `test_client.cpp` provides a _very_ basic example of a client application.  The
example just sends some commands to an I2C connected TCS34725 RGB color sensor to read
the curent RGB colors it sees.  Obviously if you don't have one of these sensors, the
example won't work.

### Building the Client
As with the server, CMake is used and the following should build a binary for testing the server.

```
> mkdir build
> cd build
> cmake ..
> make
> ./i2contest
```
