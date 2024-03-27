/**
 * @file      particleSensor.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2023-10-09
 *
 */
#pragma once



bool setupParticleSensor();
uint32_t getParticleSensorIR();
uint32_t getParticleSensorRed();
float getParticleSensorTemp();
bool isParticleSensorOnline();

// bool updateParticleSensor();






