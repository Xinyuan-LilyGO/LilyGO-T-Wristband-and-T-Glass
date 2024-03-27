/**
 * @file      particleSensor.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2023-10-07
 *
 */
#include <Wire.h>
#include <MAX30105.h>   //https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library
#include "spo2_algorithm.h"

MAX30105 particleSensor;

uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

static bool online = true;

#define SENSOR_SDA      41
#define SENSOR_SCL      40

bool setupParticleSensor()
{
    Wire1.begin(SENSOR_SDA, SENSOR_SCL);
    // Initialize sensor
    if (!particleSensor.begin(Wire1, I2C_SPEED_FAST)) { //Use default I2C port, 400kHz speed
        Serial.println(F("MAX30105 was not found. Please check wiring/power."));
        // while (1);
        online = false;
        return false;
    }

    particleSensor.setup(); //default configure

    return true;
}


uint32_t getParticleSensorIR()
{
    if (!online)return 0;
    return particleSensor.getIR();
}


uint32_t getParticleSensorRed()
{
    if (!online)return 0;
    return particleSensor.getRed();
}

float getParticleSensorTemp()
{
    if (!online)return 0;
    return particleSensor.readTemperature();
}

bool isParticleSensorOnline()
{
    return online;
}









