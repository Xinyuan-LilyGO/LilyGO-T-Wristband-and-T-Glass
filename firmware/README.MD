
## News

* The firmware supports any one in the list below. The firmware used in the list below is the same and there is no need to distinguish it.
* debug.bin outputs all debug messages from USB-CDC

## 1️⃣Support Product

| Product(PinMap)  | Firmware                         |
| ---------------- | -------------------------------- |
| [T-Glass][1]     | [firmware](./T-Glass_0315.bin)   |
| [T-Glass V2][1] | [firmware](./T-Glass_V2_GlassFactory_240805.bin)   |
| [T-Wristband][2] | [firmware](./Wristband_0315.bin) |

[1]: https://www.lilygo.cc/products/t-glass
[2]: https://www.lilygo.cc/products/t-wristband-s3



## 2️⃣How to Flash ?


### Use ESP Download Tool

- Download [Flash_download_tool](https://www.espressif.com.cn/sites/default/files/tools/flash_download_tool_3.9.5_0.zip)

![web_flasher](./images/esp_downloader.gif)

* Note that after writing is completed, you need to press RST to reset.

### Use Web Flasher

- [ESP Web Flasher Online](https://espressif.github.io/esptool-js/)

![web_flasher](./images/web_flasher.gif)

* Note that after writing is completed, you need to press RST to reset.

### Use command line


If system asks about install Developer Tools, do it.

```
python3 -m pip install --upgrade pip
python3 -m pip install esptool
```

In order to launch esptool.py, exec directly with this:

```
python3 -m esptool
```

For ESP32-S3 use the following command to write

```
esptool --chip esp32s3  --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m 0x0 firmware.bin

```

# 2️⃣FAQ

- Can't upload any sketch，Please enter the upload mode manually.
   1. Connect the board via the USB cable
   2. Press and hold the **BOOT** button , While still pressing the **BOOT** button 
   3. Press **RST** button
   4. Release the **RST** button
   5. Release the **BOOT** button (If there is no **BOOT** button, disconnect IO0 from GND.)
   6. Upload sketch
   7. Press the **RST** button to exit download mode
