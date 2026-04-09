#pragma once
#include <stdint.h>
#include <mutex>

#define ADDR_JOYSTICK 0x46
#define REG_JOYSTICK 0xF2

class JoystickHAL {
public:
    JoystickHAL();
    ~JoystickHAL();
    bool init();
    int readJoystick();

private:
    int i2c_fd;
    std::mutex i2c_mutex;
    bool openI2C();
    bool readByte(uint8_t address, uint8_t reg, uint8_t* value);
};
