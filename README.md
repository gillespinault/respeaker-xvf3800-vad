# ReSpeaker XVF3800 VAD Project

Advanced Voice Activity Detection using **ESP-SR VADNet** on the ReSpeaker XVF3800 with XIAO ESP32-S3.

## Features

- **XVF3800 Audio Processing**: AEC, beamforming, noise suppression (hardware)
- **ESP-SR VADNet**: Neural network-based VAD (software on ESP32-S3)
- **Visual Feedback**: LED ring indicates speech detection
  - Blue: Idle/noise
  - White: Speech detected
  - Orange LED: Sound direction (DoA)

## Hardware

- [ReSpeaker XMOS XVF3800 with XIAO ESP32S3](https://www.seeedstudio.com/ReSpeaker-XVF3800-4-Mic-Array-With-XIAO-ESP32S3-p-6489.html)
- Speaker connected via JST connector

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   4 MEMS Microphones                    │
└─────────────────────────┬───────────────────────────────┘
                          ▼
┌─────────────────────────────────────────────────────────┐
│              XMOS XVF3800 (Hardware DSP)                │
│  • Acoustic Echo Cancellation (AEC)                     │
│  • 4-channel Beamforming                                │
│  • Noise Suppression                                    │
│  • Direction of Arrival (DoA)                           │
│  • Automatic Gain Control (AGC)                         │
└─────────────────────────┬───────────────────────────────┘
                          │ I2S (16kHz, 32-bit)
                          ▼
┌─────────────────────────────────────────────────────────┐
│              XIAO ESP32-S3 (Software AI)                │
│  • ESP-SR Audio Front-End (AFE)                         │
│  • VADNet (Neural Network VAD)                          │
│  • LED Control via I2C                                  │
└─────────────────────────────────────────────────────────┘
```

## Requirements

- ESP-IDF v5.2+
- ESP-SR component

## Setup

1. Install ESP-IDF: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/
2. Clone this repository
3. Configure and build:
   ```bash
   idf.py set-target esp32s3
   idf.py menuconfig  # Configure ESP-SR -> VADNet
   idf.py build flash monitor
   ```

## I2C Communication (XVF3800)

- Address: `0x2C`
- SDA: GPIO 5
- SCL: GPIO 6

## I2S Audio (from XVF3800)

- BCK: GPIO 8
- WS: GPIO 7
- DIN: GPIO 43
- DOUT: GPIO 44
- Sample Rate: 16000 Hz

## License

MIT

## References

- [ESP-SR Documentation](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/index.html)
- [XVF3800 User Guide](https://www.xmos.com/documentation/XM-014888-PC/html/modules/fwk_xvf/doc/user_guide/)
- [Seeed Studio Wiki](https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_getting_started/)
