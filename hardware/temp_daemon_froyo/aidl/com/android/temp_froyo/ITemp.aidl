package com.android.temp_froyo;
import com.android.temp_froyo.ITempListener;

interface ITemp {
    float getTemperature();
    float getHumidity();
    float getAmbientTemperature(float factor);

    void registerListener(ITempListener listener, float factor);
    void unregisterListener(ITempListener listener);
}
