<h1><img src="https://github.com/RackRanger/landing-page/blob/main/static/Ricon.png" height=70 align="center"/> &nbsp; RackRanger firmware </h1>

#### Built with PlatformIO for the ESP32

## Features
The RackRanger firmware gathers data from a plethora of sensors. It also serves as an endpoint that can be scraped by Prometheus for logging and visualization. On top of that, it can also push to Loki to log alerts from enviroment readings.

## Build and run

- Install PlatformIO Core
```bash
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
```

- Run command to build, upload & debug

```bash
~/.platformio/penv/bin/platformio run -t upload && ~/.platformio/penv/bin/platformio device monitor
```
