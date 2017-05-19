#ifndef STUBS_H
#define STUBS_H


#ifndef __arm__
class Ci2c_smbus {
public:
    int W1b(uint8_t, uint8_t, uint8_t) {}
    int Wbb(uint8_t, uint8_t, uint8_t *, uint8_t) {}
    int init(int b) { return 0; }
    int init(const char *) { return 0; }
    int set_bus(int) {}
    int set_bus(const char *) {}
};
#endif


#endif // STUBS_H
