#include "i2conclient.hpp"

int main() {
    I2conClient i2cc_1;
    I2conClient i2cc_2;
    uint16_t    r,g,b;

    i2cc_1.connect("192.168.140.112", 1);
    i2cc_2.connect("192.168.140.112", 1);

    b = i2cc_1.read8(0x29, 0x92);
    printf("read %02x\n", b);

    r = i2cc_1.read16(0x29, 0x96);
    r = i2cc_2.read16(0x29, 0x96);
    g = i2cc_1.read16(0x29, 0x98);
    g = i2cc_2.read16(0x29, 0x98);
    b = i2cc_1.read16(0x29, 0x9A);
    b = i2cc_2.read16(0x29, 0x9A);

    printf("r=%04x g=%04x b=%04x\n", r, g, b);

    i2cc_1.disconnect();
}
