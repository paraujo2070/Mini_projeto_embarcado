#include "TempHAL.h"
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <stdlib.h>

TempHAL::TempHAL() : i2c_fd(-1) {}

TempHAL::~TempHAL() {
    if (i2c_fd >= 0) close(i2c_fd);
}

bool TempHAL::openI2C() {
    std::lock_guard<std::mutex> lock(i2c_mutex);
    if (i2c_fd >= 0) return true;
    i2c_fd = open("/dev/i2c-1", O_RDWR);
    return (i2c_fd >= 0);
}

bool TempHAL::loadCalibration() {
    uint8_t buf[16];
    if (!readBlock(ADDR_TEMP, 0x30, buf, 16)) return false;

    // Calibração de Temperatura (T0, T1 degC e T0, T1 OUT)
    T0_degC = (float)((buf[2] | ((buf[5] & 0x03) << 8))) / 8.0f;
    T1_degC = (float)((buf[3] | ((buf[5] & 0x0C) << 6))) / 8.0f;
    T0_out = (int16_t)(buf[12] | (buf[13] << 8));
    T1_out = (int16_t)(buf[14] | (buf[15] << 8));

    // Calibração de Umidade (H0, H1 rH e H0, H1 OUT)
    H0_rh = (float)buf[0] / 2.0f;
    H1_rh = (float)buf[1] / 2.0f;
    H0_out = (int16_t)(buf[6] | (buf[7] << 8));
    H1_out = (int16_t)(buf[10] | (buf[11] << 8));

    return true;
}

bool TempHAL::init() {
    system("echo '1-005f' > /sys/bus/i2c/drivers/hts221/unbind 2> /dev/null");
    if (!openI2C()) return false;
    
    // Config: Power On, BDU (Block Data Update) para evitar leitura parcial, 1Hz
    writeByte(ADDR_TEMP, 0x20, 0x81);
    return loadCalibration();
}

float TempHAL::readTemperature() {
    uint8_t buf[2];
    if (!readBlock(ADDR_TEMP, 0x2A, buf, 2)) return -999.0f;
    int16_t T_out = (int16_t)(buf[0] | (buf[1] << 8));
    return (T1_degC - T0_degC) * (float)(T_out - T0_out) / (float)(T1_out - T0_out) + T0_degC;
}

float TempHAL::readHumidity() {
    uint8_t buf[2];
    if (!readBlock(ADDR_TEMP, 0x28, buf, 2)) return -1.0f;
    int16_t H_out = (int16_t)(buf[0] | (buf[1] << 8));
    float h_rh = (H1_rh - H0_rh) * (float)(H_out - H0_out) / (float)(H1_out - H0_out) + H0_rh;
    return (h_rh > 100.0f) ? 100.0f : (h_rh < 0.0f ? 0.0f : h_rh);
}

float TempHAL::readCPUTemperature() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.is_open()) return -999.0f;
    std::string line;
    if (std::getline(file, line)) return std::stof(line) / 1000.0f;
    return -999.0f;
}

float TempHAL::readAmbientTemperature(float factor) {
    float t_sense = readTemperature();
    float t_cpu = readCPUTemperature();
    if (t_sense < -100.0f || t_cpu < -100.0f) return -999.0f;
    return t_sense - ((t_cpu - t_sense) / factor);
}

bool TempHAL::writeByte(uint8_t address, uint8_t reg, uint8_t value) {
    std::lock_guard<std::mutex> lock(i2c_mutex);
    if (ioctl(i2c_fd, I2C_SLAVE, address) < 0) return false;
    uint8_t buf[2] = {reg, value};
    return write(i2c_fd, buf, 2) == 2;
}

bool TempHAL::readBlock(uint8_t address, uint8_t reg, uint8_t *buffer, int length) {
    std::lock_guard<std::mutex> lock(i2c_mutex);
    if (ioctl(i2c_fd, I2C_SLAVE, address) < 0) return false;
    uint8_t reg_cmd = reg | 0x80;
    write(i2c_fd, &reg_cmd, 1);
    return read(i2c_fd, buffer, length) == length;
}
