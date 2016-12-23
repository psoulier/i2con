#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include "i2conclient.hpp"

I2conClient::I2conClient() : socket_fd(-1) {
}

I2conClient::~I2conClient() {
    disconnect();
}

/*
 * Connect to the i2c bus on the specified host.
 *
 * The i2c bus for the Raspberry Pi is either 0 or 1 depending on the hardware version of
 * the board.
 *
 * @param host: Host to connect to (IP address or host name)
 * @param i2c_bus: I2C bus to connect to.
 */
void I2conClient::connect(const std::string &ip, uint8_t bus) {
    struct addrinfo hints;
    struct addrinfo *server_info;

    if (socket_fd != -1) {
        throw I2conException("Already connected");
    }

    this->bus = bus;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip.c_str(), I2CON_PORT, &hints, &server_info) == -1) {
        throw I2conException(errno);
    }

    for (struct addrinfo *pai = server_info; pai != nullptr; pai = pai->ai_next) {
        socket_fd = socket(pai->ai_family, pai->ai_socktype, pai->ai_protocol);
        if (socket_fd == -1) {
            continue ;
        }

        if (::connect(socket_fd, pai->ai_addr, pai->ai_addrlen) < 0) {
            close(socket_fd);
            socket_fd = -1;
            continue ;
        }

        break ;
    }

    freeaddrinfo(server_info);
}

/*
 * Close connection to the Raspberry Pi.
 */
void I2conClient::disconnect() {
    if (socket_fd >= 0) {
        i2cmd_t         i2cmd;

        i2cmd.cmd = I2CON_CMD_DISCONN;
        i2cmd.len = sizeof(i2cmd);
        send(&i2cmd, sizeof(i2cmd));

        close(socket_fd);
        socket_fd = -1;
    }
}

/*
 * Reads byte from an i2c device.
 *
 * @param adr: address of i2c device.
 * @param reg: register of device to read from.
 */
uint8_t I2conClient::read8(uint16_t adr, uint8_t reg) {
    i2cmd_t         i2cmd;

    i2cmd.bus = bus;
    i2cmd.adr = adr;
    i2cmd.reg = reg;
    i2cmd.cmd = I2CON_CMD_RD8;
    i2cmd.len = sizeof(i2cmd);

    send(&i2cmd, sizeof(i2cmd_t));
    recv(&i2cmd, sizeof(i2cmd_t));

    return i2cmd.dat[0];
}

uint16_t I2conClient::read16(uint16_t adr, uint8_t reg) {
    i2cmd_t         i2cmd;

    i2cmd.bus = bus;
    i2cmd.adr = adr;
    i2cmd.reg = reg;
    i2cmd.cmd = I2CON_CMD_RD16;
    i2cmd.len = sizeof(i2cmd);

    send(&i2cmd, sizeof(i2cmd_t));
    recv(&i2cmd, sizeof(i2cmd_t));

    return ntohs(i2cmd.w);
}

/*
 * Writes a byte to an i2c device.
 *
 * @param adr: address of i2c device.
 * @param reg: register of i2c device to read.
 */
void I2conClient::write8(uint16_t adr, uint8_t reg, uint8_t b) {
    i2cmd_t         i2cmd;

    i2cmd.bus = bus;
    i2cmd.adr = adr;
    i2cmd.reg = reg;
    i2cmd.cmd = I2CON_CMD_WR8;
    i2cmd.len = sizeof(i2cmd);
    i2cmd.b = b;

    send(&i2cmd, sizeof(i2cmd_t));
    recv(&i2cmd, sizeof(i2cmd_t));
}

/*
 * Same as write8, except 16-bit word.
 */
void I2conClient::write16(uint16_t adr, uint8_t reg, uint16_t w) {
    i2cmd_t         i2cmd;

    i2cmd.bus = bus;
    i2cmd.adr = adr;
    i2cmd.reg = reg;
    i2cmd.cmd = I2CON_CMD_WR16;
    i2cmd.len = sizeof(i2cmd);
    i2cmd.w = htons(w);

    send(&i2cmd, sizeof(i2cmd_t));
    recv(&i2cmd, sizeof(i2cmd_t));
}

/*
 * Wrapper for the posix send function.
 */
void I2conClient::send(void *p, size_t len) {
    uint8_t *buf = (uint8_t*)p;
    ssize_t xfer;

    do {
        xfer = ::send(socket_fd, buf, len, 0);
        buf += xfer;
        len -= xfer;
    } while (xfer > 0 && len > 0);

    if (xfer == 0) {
        throw I2conException("Connection closed");
    }
    else if (xfer < 0) {
        throw I2conException(errno);
    }
}

/*
 * Wrapper for the posix recv function.
 */
void I2conClient::recv(void *p, size_t len) {
    uint8_t *buf = (uint8_t*)p;
    ssize_t xfer;

    do {
        xfer = ::recv(socket_fd, buf, len, 0);
        buf += xfer;
        len -= xfer;
    } while (xfer > 0 && len > 0);

    if (xfer == 0) {
        throw I2conException("Connection closed");
    }
    else if (xfer < 0) {
        throw I2conException(errno);
    }
}


