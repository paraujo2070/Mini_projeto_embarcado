package com.android.temp_froyo;

interface ITempListener {
    void onTemperatureUpdate(float temperature);
    void onHumidityUpdate(float humidity);
    void onAmbientTemperatureUpdate(float ambientTemperature);
    void onClimateUpdate(float temperature, float humidity, float ambientTemperature);
}
