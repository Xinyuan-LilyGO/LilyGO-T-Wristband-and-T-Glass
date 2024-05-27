<h1 align = "center">🌟LilyGO T-Wristband and T-Glass🌟</h1>

## News

- T-Wristband and T-Glass have the same hardware, but the viewing area is different due to the casing.
- `LilyGO T-Wristband and T-Glass` depends on lvgl **v8.4.x** version. Currently lvgl has released lvgl **v9.0** version. When compilation error occurs, please check whether lvgl belongs to **v8.4.x**
- The visual size of T-Wristband and T-Glass is different from the actual screen size. All examples are based on the lvgl graphics framework. Other image frameworks such as TFT_eSPI are not available here.

## 1️⃣Support Product

| Product(PinMap)  | SOC            | Flash | PSRAM     | Resolution | Visible Area |
| ---------------- | -------------- | ----- | --------- | ---------- | ------------ |
| [T-Glass][1]     | ESP32-S3 FN4R2 | 4MB   | 2MB(QSPI) | 294x126    | 126x126      |
| [T-Wristband][2] | ESP32-S3 FN4R2 | 4MB   | 2MB(QSPI) | 294x126    | 126x250      |

| Current consumption | Working current             | sleep current | sleep mode          |
| ------------------- | --------------------------- | ------------- | ------------------- |
| [T-Glass][1]        | (240MHz) WiFi On 90~230+ mA | about 300uA   | touch button wakeup |
| [T-Wristband][2]    | (240MHz) WiFi On 90~230+ mA | about 300uA   | touch button wakeup |

[1]: https://www.lilygo.cc/products/t-glass
[2]: https://www.lilygo.cc/products/t-wristband-s3

## 2️⃣Examples

```txt
examples/
├── Glass
│   ├── Glass6DoF
│   ├── GlassBatteryVoltage
│   ├── GlassDeepSleep
│   ├── GlassFactory
│   ├── GlassHelloWorld
│   ├── GlassRtcAlarm
│   ├── GlassRtcDateTime
│   ├── GlassTouchButton
│   ├── GlassTouchButtonEvent
│   └── GlassVoiceActivityDetection
└── Wristband
    ├── Wristband6DoF
    ├── WristbandBatteryVoltage
    ├── WristbandDeepSleep
    ├── WristbandDisplayRotation
    ├── WristbandDisplayVisualWindow1
    ├── WristbandFactory
    ├── WristbandHelloWorld
    ├── WristbandLightSleep
    ├── WristbandRtcAlarm
    ├── WristbandRtcDateTime
    ├── WristbandTouchButton
    └── WristbandTouchButtonEvent
```

## 3️⃣ PlatformIO Quick Start (Recommended)

1. Install [Visual Studio Code](https://code.visualstudio.com/) and [Python](https://www.python.org/)
2. Search for the `PlatformIO` plugin in the `VisualStudioCode` extension and install it.
3. After the installation is complete, you need to restart `VisualStudioCode`
4. After restarting `VisualStudioCode`, select `File` in the upper left corner of `VisualStudioCode` -> `Open Folder` -> select the `LilyGO T-Wristband and T-Glass` directory
5. Wait for the installation of third-party dependent libraries to complete
6. Click on the `platformio.ini` file, and in the `platformio` column
7. Uncomment one of the lines `src_dir = xxxx` to make sure only one line works
8. Click the (✔) symbol in the lower left corner to compile
9. Connect the board to the computer USB
10. Click (→) to upload firmware
11. Click (plug symbol) to monitor serial output
12. If it cannot be written, or the USB device keeps flashing, please check the **FAQ** below

## 4️⃣ Install from Arduino Library Manager (recommended)

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Install [Arduino ESP32 V 2.0.5 or above and below V3.0](https://docs.espressif.com/projects/arduino-esp32/en/latest/getting_started.html), Library based on the Stable version, Please make sure the link in Boards Manager is
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. `Sketch` -> `Include Library` -> `Manage Libraries`
4. `Library Search` -> `LilyGO T-Wristband and T-Glass` -> `Install` -> `Install ALL`
5. `Library Search` -> `lvgl` -> `v8.4.0` -> `Install`
6. `Library Search` -> `SensorLib` -> `v0.1.8` -> `Install`
7. `File` -> `Examples` -> `LilyGO T-Wristband and T-Glass` -> `Select the corresponding example`
8. `Tools` -> , Make your selection according to the table below
   | Arduino IDE Setting                  | Value                                             |
   | ------------------------------------ | ------------------------------------------------- |
   | Board                                | **ESP32S3 Dev Module**                            |
   | Port                                 | Your port                                         |
   | USB CDC On Boot                      | Enable                                            |
   | CPU Frequency                        | 240MHZ(WiFi)                                      |
   | Core Debug Level                     | None                                              |
   | USB DFU On Boot                      | Disable                                           |
   | Erase All Flash Before Sketch Upload | Disable                                           |
   | Events Run On                        | Core1                                             |
   | Flash Mode                           | QIO 80MHZ                                         |
   | Flash Size                           | **4MB(32Mb)**                                     |
   | Arduino Runs On                      | Core1                                             |
   | USB Firmware MSC On Boot             | Disable                                           |
   | Partition Scheme                     | **Default 4M with spiffs(1.2M APP/1.5MB SPIFFS)** |
   | PSRAM                                | **QSPI PSRAM**                                    |
   | Upload Mode                          | **UART0/Hardware CDC**                            |
   | Upload Speed                         | 921600                                            |
   | USB Mode                             | **CDC and JTAG**                                  |

   - The options in bold are required, others are selected according to actual conditions.

9. Select `Port`
10. Click `upload` , Wait for compilation and writing to complete
11. If it cannot be written, or the USB device keeps flashing, please check the **FAQ** below

## 5️⃣ Arduino IDE Manual installation (not recommended)

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Install [Arduino ESP32 V 2.0.5 or above and below V3.0](https://docs.espressif.com/projects/arduino-esp32/en/latest/getting_started.html), Library based on the Stable version, Please make sure the link in Boards Manager is
   ```url 
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Download `LilyGO T-Wristband and T-Glass` , move to Arduino library folder (e.g. C:\Users\YourName\Documents\Arduino\libraries)
4. Copy all folders in [libdeps folder](./libdeps/) to Arduino library folder (e.g. C:\Users\YourName\Documents\Arduino\libraries)
5. `Tools` -> , Make your selection according to the table below
   | Arduino IDE Setting                  | Value                                             |
   | ------------------------------------ | ------------------------------------------------- |
   | Board                                | **ESP32S3 Dev Module**                            |
   | Port                                 | Your port                                         |
   | USB CDC On Boot                      | Enable                                            |
   | CPU Frequency                        | 240MHZ(WiFi)                                      |
   | Core Debug Level                     | None                                              |
   | USB DFU On Boot                      | Disable                                           |
   | Erase All Flash Before Sketch Upload | Disable                                           |
   | Events Run On                        | Core1                                             |
   | Flash Mode                           | QIO 80MHZ                                         |
   | Flash Size                           | **4MB(32Mb)**                                     |
   | Arduino Runs On                      | Core1                                             |
   | USB Firmware MSC On Boot             | Disable                                           |
   | Partition Scheme                     | **Default 4M with spiffs(1.2M APP/1.5MB SPIFFS)** |
   | PSRAM                                | **QSPI PSRAM**                                    |
   | Upload Mode                          | **UART0/Hardware CDC**                            |
   | Upload Speed                         | 921600                                            |
   | USB Mode                             | **CDC and JTAG**                                  |

   - The options in bold are required, others are selected according to actual conditions.

6. `File` -> `Examples` -> `LilyGO T-Wristband and T-Glass` -> `Select the corresponding example`
7. Select `Port`
8. Click `upload` , Wait for compilation and writing to complete
9. If it cannot be written, or the USB device keeps flashing, please check the **FAQ** below

# 6️⃣ ESP32 basic examples

- [BLE Examples](https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE)
- [WiFi Examples](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi)
- [SPIFFS Examples](https://github.com/espressif/arduino-esp32/tree/master/libraries/SPIFFS)
- [FFat Examples](https://github.com/espressif/arduino-esp32/tree/master/libraries/FFat)
- For more examples of esp32 chip functions, please refer to [arduino-esp32-libraries](https://github.com/espressif/arduino-esp32/tree/master/libraries)

# 7️⃣ FAQ

1. The board uses USB as the **JTAG** upload port. When printing serial port information on **USB_CDC_ON_BOOT** configuration needs to be turned on.
   If the port cannot be found when uploading the program or the USB has been used for other functions, the port does not appear.
   Please enter the upload mode manually.

   1. Connect the board via the USB cable
   2. Press and hold the **BOOT button** , While still pressing the **BOOT button**, press **RST button**
   3. Release the **RST button**
   4. Release the **BOOT button**
   5. Upload sketch

2. If the above is invalid, burn the [binary file](./firmware/README.MD) to check whether the hardware is normal
3. If you use external power supply instead of USB-C, please turn off the **USB_CDC_ON_BOOT** option. This is because the board will wait for USB access when it starts.

   - For Arduino IDE users, it can be turned off in the options , Please note that turning off **USB_CDC_ON_BOOT** will turn off Serial redirection to USB-C. At this time, you will not see any Serial message output when opening the port from USB-C, but output from GPIO43 and GPIO44.

   ```c
   Tools -> USB CDC On Boot -> Disable
   ```

   - For Platformio users, you can add the following compilation flags in the ini file

   ```c
   build_flags =
       ; Enable -DARDUINO_USB_CDC_ON_BOOT will start printing and wait for terminal access during startup
       ; -DARDUINO_USB_CDC_ON_BOOT=1

       ; Enable -UARDUINO_USB_CDC_ON_BOOT will turn off printing and will not block when using the battery
       -UARDUINO_USB_CDC_ON_BOOT
   ```
4. The JD9613 display RAM is only 1/2 the screen size and does not support rotation.Directions 0 and 1 can adjust the vertical direction, horizontal 1 and 3 are the same direction.
   ```c
   // Set the screen orientation to portrait , 0 and 2 are two opposite vertical directions
    amoled.setRotation(0);
    amoled.setRotation(2);

   // Set the screen orientation to horizontal. 1 and 3 are all in the same direction and cannot be changed.
    amoled.setRotation(1);
    amoled.setRotation(3);
   ```

# 8️⃣ Resource

| Product(PinMap)  | schematic                                                   |
| ---------------- | ----------------------------------------------------------- |
| [T-Glass][1]     | [schematic](./schematic/LilyGO-T-Wristband-and-T-Glass.pdf) |
| [T-Wristband][2] | [schematic](./schematic/LilyGO-T-Wristband-and-T-Glass.pdf) |

##### T-Glass & T-Wristband datasheet

- [JD9613 Driver](./datasheet/JD9613_DS_Preliminary_V0.02_20210325.pdf)
- [ESP32S3-FN4R2](https://www.espressif.com.cn/en/support/documents/technical-documents?keys=&field_type_tid%5B%5D=842)
- [BHI260AP](https://www.bosch-sensortec.com/products/smart-sensor-systems/bhi260ap/)
- [PCF85063A](https://www.nxp.com/products/analog-and-mixed-signal/real-time-clocks/rtcs-with-ic-bus/tiny-real-time-clock-calendar-with-alarm-function-and-ic-bus:PCF85063A)

# 8️⃣ Depends on required libraries

- [SensorLib 0.1.8](https://github.com/lewisxhe/SensorsLib)
- [lvgl 8.4.0](https://github.com/lvgl/lvgl/tree/release/v8.4)
- [MadgwickAHRS](https://github.com/arduino-libraries/MadgwickAHRS)
<!-- - [SparkFun_MAX3010x_Sensor_Library](https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library) -->
