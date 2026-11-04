# 三色灯循环

适用：Seeed XIAO ESP32-C3、带限流电阻的共阴三色灯模块。

| 模块 | 开发板 |
| --- | --- |
| R | D6 / GPIO21 |
| Y | D7 / GPIO20 |
| G | D10 / GPIO10 |
| GND | GND |

USB-C 供电。程序每秒切换一次：红 → 黄 → 绿 → 红；只点亮一种颜色。
不需要 AM312，也不需要电池。

安装并配置 PlatformIO 后，在本目录运行 `pio run` 编译；运行
`pio run -t upload --upload-port /dev/cu.usbmodem101` 烧录。
本机当前通过 uv 缓存运行 PlatformIO，未将 `pio` 安装到全局 PATH。
本项目工具目录为 `/Users/hanzhang/Projects/MyPlaygroud/door-alert/.platformio`，
运行时设置 `PLATFORMIO_CORE_DIR` 指向该目录。
串口速率为 115200，输出颜色、GPIO、运行时间和循环次数。
串口名称在重新插拔后可能变化，烧录前应核对开发板。

修改 `src/main.cpp` 中 `intervalMs` 可调整每种颜色的持续时间。

原板备份：`../../backups/ac276e7fd320-before-traffic-light-20261101.bin`
SHA256：`c9fb7f9ca5ecc0cc7ae1fe58e94e45c6dd72b4c7ab0bde4ad4cdaea05b2b9612`

## 2026-11-03 实机验证

- 目标板 MAC：`AC:27:6E:7F:D3:20`；当前端口：`/dev/cu.usbmodem101`。
- PlatformIO 8.0.0，Arduino ESP32 3.3.12，原生 ARM64 RISC-V GCC 14.2。
- 构建成功，程序 Flash 占用 277582 字节；烧录后各段写入哈希校验通过。
- `verification/serial.log` 和 `verification/result.json` 保存实际板上输出。
- 连续10条颜色状态通过检查，顺序为红→黄→绿，间隔1000–1001ms。
- 此验证证明固件运行与输出顺序；灯模块实际发光、接触可靠性须现场确认。
- 本机平台缓存的 `platform.json` 将无关的 Xtensa 编译器及两种 GDB 标为
  optional，以避免为本次 ESP32-C3 串口烧录下载无用工具；未修改编译器或框架源码。
