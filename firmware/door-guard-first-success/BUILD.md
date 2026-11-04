# 本机构建

```sh
export PLATFORMIO_CORE_DIR=/Users/hanzhang/Projects/MyPlaygroud/door-alert/.platformio
export UV_TOOL_DIR=/private/tmp/door-alert-uv-tools
export UV_TOOL_BIN_DIR=/private/tmp/door-alert-uv-bin
cd /Users/hanzhang/Projects/MyPlaygroud/door-alert/firmware/door-guard
uv --cache-dir /private/tmp/door-alert-uv-cache tool run --offline --from platformio pio run
```

烧录环境选`-e rear`或`-e front`，加`-t upload --upload-port <对应串口>`。
烧录前核对MAC；不要仅凭端口编号确定角色。

本机平台缓存将无关Xtensa编译器及两种GDB包标为optional，省去无关下载。
未修改RISC-V编译器或Arduino框架。平台8.0.0，Arduino ESP32 3.3.12，
GCC 14.2.0+20260121原生ARM64，PlatformIO Core 6.2.0。

驱动官方下载：https://github.com/sparkfun/SparkFun_VL53L5CX_Arduino_Library
ESP-NOW接口：https://docs.espressif.com/projects/arduino-esp32/en/latest/api/espnow.html

算法检查：

```sh
clang++ -std=c++17 -Wall -Wextra -Werror -Iinclude tests/detection_test.cpp -o /private/tmp/door-detection-test
/private/tmp/door-detection-test
```
