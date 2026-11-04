# VL53L5CX最小I²C诊断

只运行于门后板AC:27:6E:7F:C3:B4；不启用Wi-Fi、ESP-NOW、测距初始化或灯控制。
分别测试SDA=GPIO6/SCL=GPIO7及互换组合，每组测试100kHz和400kHz。
扫描十进制地址8–119；若发现十进制41（0x29），读取设备ID和修订号，
严格匹配0xF0及0x02后才报告matches_vl53l5cx=true。
程序只进行I²C通信，信号引脚不输出5V，不改变模块供电。

该诊断会停止门后状态广播，门前闪黄灯属于预期。需要恢复配套固件时，
使用相邻door-guard-first-success项目的rear环境，按MAC核对烧录。
没有应答不能独立证明模块损坏；仍可能是供电、焊点、线路、电平转换或主控引脚问题。

2026-11-04实测：正确GPIO6/7在100kHz、400kHz均发现0x29，ID=0xF0、revision=0x02。
反向GPIO7/6无应答，符合正确接线。记录见verification/scan.jsonl。

当前实机随后烧录了ranging环境：不启用无线，加载原SparkFun驱动进行8×8、10Hz测距。
初始化成功，采到160帧，但采样期间有效区域数始终为0，因此不能认定光学测距正常。
下一步需要让传感器正对30–50cm处白纸，确保光学窗口未遮挡后再观察。
记录见verification/ranging.jsonl和ranging-result.json。

白纸后续诊断：white-paper-raw.jsonl记录14–15个区域status=5，原始距离约18–44mm（统计包含无目标区域），其余多为255。过滤后valid=0与诊断程序排除40mm以下距离有关，不能直接解读为芯片无法测距。尚未验证固定30–50cm白纸距离的对应关系。
