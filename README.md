# firmware

PlatformIO based project for the ESP32

How to run:

- Install PlatformIO Core

```bash
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
```

- Run command to build, upload & debug

```bash
~/.platformio/penv/bin/platformio run -t upload && ~/.platformio/penv/bin/platformio device monitor
```
