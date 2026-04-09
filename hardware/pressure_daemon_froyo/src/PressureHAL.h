#pragma once
#include <stdint.h>
#include <mutex>

#define ADDR_PRESSURE 0x5C
#define REG_CTRL_REG1  0x20
#define REG_PRESS_OUT_XL 0x28
#define REG_PRESS_OUT_L  0x29
#define REG_PRESS_OUT_H  0x2A

class PressureHAL {
public:
    PressureHAL();
    ~PressureHAL();
    bool init();
    float readPressure();

private:
    int i2c_fd;
    std::mutex i2c_mutex;
    bool openI2C();
    bool writeByte(uint8_t address, uint8_t reg, uint8_t value);
    bool readBlock(uint8_t address, uint8_t reg, uint8_t* buffer, int length);
};
