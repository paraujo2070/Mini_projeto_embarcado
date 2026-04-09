#include "JoystickHAL.h"
#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <stdlib.h>

JoystickHAL::JoystickHAL() : i2c_fd(-1) {}

JoystickHAL::~JoystickHAL() {
    if (i2c_fd >= 0) close(i2c_fd);
}

bool JoystickHAL::openI2C() {
    std::lock_guard<std::mutex> lock(i2c_mutex);
    if (i2c_fd >= 0) return true;
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    if (i2c_fd < 0) {
        std::cerr << "Failed to open /dev/i2c-1" << std::endl;
        return false;
    }
    return true;
}

bool JoystickHAL::init() {
    // Unbind por segurança antes de iniciar
    int ret = system("echo '1-0046' > /sys/bus/i2c/drivers/simple-mfd-i2c/unbind 2> /dev/null");
    (void)ret;
    return openI2C();
}

bool JoystickHAL::readByte(uint8_t address, uint8_t reg, uint8_t *value) {
    std::lock_guard<std::mutex> lock(i2c_mutex);
    if (ioctl(i2c_fd, I2C_SLAVE, address) < 0) return false;
    if (write(i2c_fd, &reg, 1) != 1) return false;
    if (read(i2c_fd, value, 1) != 1) return false;
    return true;
}

int JoystickHAL::readJoystick() {
    uint8_t val = 0;
    if (!readByte(ADDR_JOYSTICK, REG_JOYSTICK, &val)) return -1;
    return (int)val;
}
