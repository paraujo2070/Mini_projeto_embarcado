#pragma once
#include <stdint.h>
#include <mutex>

#define ADDR_TEMP 0x5F

class TempHAL {
public:
    TempHAL();
    ~TempHAL();
    bool init();
    float readTemperature();
    float readHumidity();
    float readAmbientTemperature(float factor);

private:
    int i2c_fd;
    std::mutex i2c_mutex;
    
    // Coeficientes de Calibração do HTS221
    float T0_degC, T1_degC, H0_rh, H1_rh;
    int16_t T0_out, T1_out, H0_out, H1_out;

    bool openI2C();
    bool loadCalibration();
    float readCPUTemperature();
    bool writeByte(uint8_t address, uint8_t reg, uint8_t value);
    bool readBlock(uint8_t address, uint8_t reg, uint8_t* buffer, int length);
};
