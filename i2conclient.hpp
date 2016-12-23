#ifndef __I2CON_CLIENT_HPP__
#define __I2CON_CLIENT_HPP__

#include "i2con.hpp"

class I2conClient {
    int             socket_fd;
    uint8_t         bus;

    void send(void *buf, size_t len);
    void recv(void *buf, size_t len);

    public:
        I2conClient();
        ~I2conClient();

        void connect(const std::string &ip, uint8_t bus);
        void disconnect();

        uint8_t read8(uint16_t adr, uint8_t reg);
        uint16_t read16(uint16_t adr, uint8_t reg);
        void write8(uint16_t adr, uint8_t reg, uint8_t b);
        void write16(uint16_t adr, uint8_t reg, uint16_t w);
        //int read(void *buf, int len);
        //int write(void *buf, int len);

};


#endif
