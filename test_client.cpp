#include "i2conclient.hpp"

int main() {
    I2conClient i2cc(1);
    uint16_t    r,g,b;

    i2cc.connect("192.168.140.112");

    b = i2cc.read8(0x29, 0x92);
    printf("read %02x\n", b);

    r = i2cc.read16(0x29, 0x96);
    g = i2cc.read16(0x29, 0x98);
    b = i2cc.read16(0x29, 0x9A);

    printf("r=%04x g=%04x b=%04x\n", r, g, b);

    i2cc.disconnect();
}
