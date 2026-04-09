package com.android.temp_froyo;

interface ITemp {
    float getTemperature();
    float getHumidity();
    float getAmbientTemperature(float factor);
}
