#ifndef __I2CON_HPP__
#define __I2CON_HPP__

#include <stdexcept>
#include <string>
#include <string.h>

static const uint8_t I2CON_CMD_DISCONN  = 0x02;
static const uint8_t I2CON_CMD_RD8      = 0x03;
static const uint8_t I2CON_CMD_RD16     = 0x04;
static const uint8_t I2CON_CMD_WR8      = 0x05;
static const uint8_t I2CON_CMD_WR16     = 0x06;

static const int8_t I2CON_STS_OK        = 0;
static const int8_t I2CON_STS_BADBUS    = -1;
static const int8_t I2CON_STS_BADADR    = -2;
static const int8_t I2CON_STS_IOERR     = -3;

static const char *I2CON_PORT = "3490";

struct i2cmd_t {
    uint16_t        adr;        // Address of i2c device (u16 to support 10-bit 12c address)
    uint8_t         reg;        // Register to access
    uint8_t         cmd;        // Command to issues to i2c devie
    int8_t          sts;        // Status of command
    uint8_t         rsv[3];
    uint32_t        len;        // Length of data transfer
    union {
        uint8_t     dat[4];     // 4 bytes of data (use malloc w/ extra space for larger xfers)
        uint8_t     b;
        uint16_t    w;
        int8_t      bus;
    };
};

class I2conException {
    int         err;
    std::string msg;

    public:
        I2conException(int err) : err(err) {
        }

        I2conException(const std::string &msg) : err(0), msg(msg) {
        }

        virtual const char* what() const throw() {
            if (msg.length() > 0) {
                return msg.c_str();
            }
            else if (err < 0) {
                return strerror(err);
            }
            else {
                return "Unknown i2con exception occurred.";
            }
        }
};

#endif

