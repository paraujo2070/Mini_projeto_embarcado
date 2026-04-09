#include "PressureHAL.h"
#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <stdlib.h>

PressureHAL::PressureHAL() : i2c_fd(-1) {}

PressureHAL::~PressureHAL() {
    if (i2c_fd >= 0) close(i2c_fd);
}

bool PressureHAL::openI2C() {
    std::lock_guard<std::mutex> lock(i2c_mutex);
    if (i2c_fd >= 0) return true;
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    return (i2c_fd >= 0);
}

bool PressureHAL::init() {
    // Unbind por segurança (Endereço 0x5C)
    system("echo '1-005c' > /sys/bus/i2c/drivers/lps25h/unbind 2> /dev/null");
    
    if (!openI2C()) return false;

    // 0x90 = Power ON, 1Hz output rate
    return writeByte(ADDR_PRESSURE, REG_CTRL_REG1, 0x90);
}

float PressureHAL::readPressure() {
    uint8_t buf[3];
    // Leitura dos 3 bytes de pressão (XL, L, H) com auto-incremento (0x80)
    if (!readBlock(ADDR_PRESSURE, REG_PRESS_OUT_XL, buf, 3)) return -1.0f;
    
    // Monta o valor de 24 bits
    int32_t raw_press = (int32_t)(buf[0] | (buf[1] << 8) | (buf[2] << 16));
    
    // Conversão do LPS25H: dividir por 4096 para obter hPa
    return (float)raw_press / 4096.0f;
}

bool PressureHAL::writeByte(uint8_t address, uint8_t reg, uint8_t value) {
    std::lock_guard<std::mutex> lock(i2c_mutex);
    if (ioctl(i2c_fd, I2C_SLAVE, address) < 0) return false;
    uint8_t buf[2] = {reg, value};
    return write(i2c_fd, buf, 2) == 2;
}

bool PressureHAL::readBlock(uint8_t address, uint8_t reg, uint8_t *buffer, int length) {
    std::lock_guard<std::mutex> lock(i2c_mutex);
    if (ioctl(i2c_fd, I2C_SLAVE, address) < 0) return false;
    uint8_t reg_cmd = reg | 0x80;
    write(i2c_fd, &reg_cmd, 1);
    return read(i2c_fd, buffer, length) == length;
}
