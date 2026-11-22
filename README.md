## 大金空气传感器
下图为BRY88AB151K版，其右上为甲醛扩展侧盖，向后滑动打开；不带甲醛字样的为BRY88AA151K版，不支持甲醛扩展

> **本仓库新增的 OLED 配置、组件与固件全部基于 BRY88AB151K（AB 版）**，请勿与 BRY88AA151K（AA 版）混用：两版 PCB 引脚与外设不同，AA 版没有甲醛扩展侧盖，也不支持甲醛模块。

<img src="https://raw.githubusercontent.com/louliangsheng/daikin-air-sensor/main/image/BRY88AB151K.jpg" width="300">

## 编译以及烧录
1.按照参考链接[视频教程](https://www.bilibili.com/video/BV1PY4y1y7sE)进行拆解。

2.根据需要将header文件夹中的pm2005.h、pm2105.h、cm1106.h放入ESPHome配置文件夹中，然后根据传感器选择相应的yaml文件编译固件，其中PM2.5传感器银色金属外壳是pm2005，黑色塑料外壳的是pm2105（**这是早期批次的特征，后期批次请见下文「拆机辨认」一节**），若没有编译环境，可直接使用firmware文件夹中编译好的factory固件刷写。

3.将Power&IIC排线插到PCB板上，BRY88AB151K版为短接图片中GPIO0两个针脚（其中一个为GND），短接后上电即可进入BOOT刷写模式，然后将图片中的RXD TXD GND连接到USB转TTL刷机板的TX RX GND上，USB插到电脑上，进入[WEB ESPHome](https://web.esphome.io/) 烧录固件；BRY88AA151K版刷机原理相同，需要短接GPIO0和GND进入BOOT后通过芯片U0 RXTX刷写；进入BOOT后PCB板上的LED灯不亮，若无法烧录可尝试对调RXD TXD针脚或一直短接GPIO0/GND并重新上电。

<img src="https://raw.githubusercontent.com/louliangsheng/daikin-air-sensor/refs/heads/main/image/replication.jpg" width="350"><img src="https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2018/08/esp32-pinout-chip-ESP-WROOM-32.png" width="430">

4.刷写完成后重新上电，连接DAIKIN Fallback Hotspot热点进行配网，BRY88AB151K版可安装[甲醛模块BRY88HA111](https://mp.weixin.qq.com/s?search_click_id=7568137682827411716-1679579213259-0915290085&__biz=MzI2MTQ1Mzk5NA==&mid=2247507885&idx=1&sn=4e776305cd6160c879ebb7c7b14114b3&chksm=ea58be68dd2f377e69ff886ba1ef2e415e8a3beee49b2b690f129c9dc010533a1c179c52a617&scene=7&clicktime=1679579213&enterid=1679579213&subscene=0&ascene=65&devicetype=iOS16.3.1&version=18002225&nettype=WIFI&lang=zh_CN&countrycode=CN&fontScale=100&exportkey=n_ChQIAhIQqp9dEP%2FcvKNVHjT9MLSoURLbAQIE97dBBAEAAAAAALfEBAuiQiEAAAAOpnltbLcz9gKNyK89dVj0tP45T%2Fk5VwDaZiPSX7nxBsvQzJfAO87KKKXNywrNXQSAgmyxqgaAGN37ms8QtxnxcvyjU5Ec%2FVkE3UVETqpzCG7kW9sFuH2XkRVj5qAT06RIDakT%2BXpfhqK24j9zp3%2Bxuq2SrOr3gwBi3sqsPCcmRKwUZ3zEUPUGofxD%2B4HrNPWxEKcwCH48Rzs49u%2FqjaAQfOjcn9rVs2X2sWnFcLAP3%2BlwZONDO423YffTUZW4811HiZ755Q%3D%3D&pass_ticket=VG%2B%2FH3lCltnJWspHiUjdHQtR83Q%2BxCGYBd2vxTKXFSsgfMHaA28Ud%2BsfO15hWyOYtfamz1JiG7DhYWErlwaccA%3D%3D&wx_header=3)；配网完成后下载firmware/ota文件夹内设备对应固件进行升级，升级后需要重新配网；CO2传感器启动需要大概3分钟，启动期间数值为550ppm；在进行CO2校准时，请确保放入室外或此时CO2浓度足够低。

<img src="https://mmbiz.qpic.cn/mmbiz_png/HhGBahfz4QZ6IMPibRCbowicxMjSgsZNiaCX7C6A6RgkyOuGIbDl0eCWA5cjsypn0RPjo7rjbAhqvtTxANthGQZmw/640?wx_fmt=png&wxfrom=5&wx_lazy=1&wx_co=1" width="450">

## 拆机辨认：先确认批次，再选 YAML

原作者 README 里写的「PM2.5 传感器银色金属外壳是 pm2005、黑色塑料外壳是 pm2105」，描述的是**早期批次**的机器，不能直接套用到所有机器：

| 批次 | PM2.5 传感器外观 | 实测 CM1106 固件 | 建议配置 |
|---|---|---|---|
| 早期 | 银色金属外壳，或黑色塑料外壳（特征明显、易分辨） | 银壳机 `V1.06.3.00`（Bit0 反转）、黑壳机 `V5.06.3.02`（Bit0 正常） | 银壳用 `-pm2005-oled.yaml`，黑壳用 `-pm2105-oled.yaml` |
| 后期 | **哑光银色**，表面带**厂家标识/丝印** | 以「读取 CM1106 版本」实测为准 | 按实测版本号选择（见下） |

- 后期批次的 PM2.5 模块是哑光银色、带厂家标识，颜色与早期的亮银/黑色都不一样，**只看壳色会认错**，建议拆机后直接看模块本体上的厂家标识与丝印。
- 最稳妥的确认方式：先刷任一版固件，在 Web 界面或 ESPHome 日志里点「读取 CM1106 版本」——`V1.0x` 系列用 `bit0_inverted: true`（`-pm2005-oled.yaml`），`V5.0x` 系列用 `false`（`-pm2105-oled.yaml`）；两版固件其余功能完全相同。

## CM1106 固件版本差异（实测）

同一款 BRY88AB151K/AA151K 主机，可能配到 **不同固件版本** 的 CM1106 二氧化碳模块。实测两台设备固件相差一个大版本，串口协议在个别字节上的语义并不一致——直接照规格书写驱动会「这台能用、那台不行」。

| 项目 | PM2005 主机 | PM2105 主机 |
|---|---|---|
| CM1106 固件版本（`0x1E`） | `V1.06.3.00` | `V5.06.3.02` |
| STATUS Bit0（预热位）语义 | **反转**：0=预热中 / 1=运行中 | **正常**：1=预热中 / 0=运行中 |
| `0x1E` 响应长度 | **15 字节**（`16 0C 1E` + 11 字节数据，含 1 字节保留） | **14 字节**（`16 0B 1E` + 10 字节数据，无保留字节） |

### 协议差异一：STATUS Bit0（预热位）极性随固件版本不同

- CM1106-C V0.6 规格书写的是 Bit0=1 表示预热、0 表示正常运行。
- `V5.06.3.02`（PM2105）符合规格：`STATUS=0x00` 时读数连续跟踪。
- `V1.06.3.00`（PM2005）**写反了**：`STATUS=0x00` 时读数卡在 550ppm，`STATUS=0x01` 后才连续跟踪。
- 若照规格书把 0 当「正常」、1 当「预热」，PM2005 会在预热结束时误判为「进入预热」，结果是预热期间发布冻结读数、预热结束后反而停止发布。

### 协议差异二：`0x1E`（读固件版本）响应长度随固件版本不同

- 规格书：`16 0C 1E` + 11 字节数据（前 10 字节 ASCII 版本号 + 1 字节保留）+ CS，共 15 字节。
- `V1.06.3.00`（PM2005）符合：返回 15 字节，能读出 `V1.06.3.00`。
- `V5.06.3.02`（PM2105）只返回 **14 字节**（`16 0B 1E` + 10 字节数据，没有保留字节）；按 15 字节硬读会报 `Reading from UART timed out at byte 14!`。
- 因此读 `0x1E` 必须先读 3 字节帧头 `16 LEN CMD`，再**按 LEN 字段动态读取**剩余字节，不能写死长度。

## 改造说明

在 2026.11 版 ESPHome（ESP-IDF 5.5）上，把原来的裸 `cm1106.h` 升级成外部组件 `cm1106_pro`，并做了以下针对真实硬件的修正，让两台不同固件的机器都能正确读数：

1. **`bit0_inverted` 配置项**：按设备区分 STATUS Bit0 极性。PM2005（`V1.0x`）设 `true`，PM2105（`V5.0x`）设 `false`；默认 `false`，保持与规格书一致。
2. **`0x1E` 固件版本按 LEN 动态解析**：先读帧头再按 LEN 读剩余，同时兼容 15 字节（`V1.0x`）与 14 字节（`V5.0x`）两种响应；配合 `0x1F` 序列号读取，可在 Web 界面用「读取 CM1106 版本」按钮一键查询型号/固件，用来判别该用哪套 Bit0 配置。
3. **状态位分级发布**：`co2` 仅发布有效值，预热/错误/超量程（BIT1/BIT2·3）不发布；另暴露 `co2_raw`、`co2_status`、`co2_errors`、`co2_last_valid_age` 等诊断量。
4. **诊断日志**：CM1106 原始帧（含校验和判定）、STATUS 变化、预热时长与 5 分钟超时提醒、ABC（`0x10`）下发与 ACK 均有日志，方便对照上表排查。
5. **ABC 自动基线校准**：默认关闭，Web 开关可下发 `11 07 10 64 [00|02] cycle baseline 64 CS` 并校验 ACK `16 01 10 D9`；周期/基准按 CM1106-C V0.6 的 1–30 天、400–1500ppm，不采用官方更宽的范围。

> 判别方法（无需拆机）：先点「读取 CM1106 版本」，`V1.0x` 系列按 `bit0_inverted: true`、`V5.0x` 系列按 `false`；也可观察 STATUS 字节与读数是否吻合来判定（STATUS=0 时读数仍冻结在 550ppm，说明该机 Bit0 写反，应设 `true`）。

## 2026 改造版源码与固件（OLED 显示）

上面「改造说明」对应的完整源码、两份配置和编译好的固件都在本仓库里（**全部基于 BRY88AB151K / AB 版，AA 版请勿直接使用**），**不需要再改动 ESPHome 安装目录**（旧版把 `pm2005` 组件塞进 `site-packages`，这里已改为本地组件）。

### 文件清单

| 路径 | 说明 |
|---|---|
| `components/cm1106_pro/` | 增强版 CM1106 驱动（UART），含 `bit0_inverted`、`0x1E` 按 LEN 动态解析、状态位分级发布、ABC 校准与 `read_version` 动作 |
| `components/pm2005/` | PM2005 / PM2105 的 I²C 组件（`platform: pm2005`，`type: PM2005` 或 `PM2105`） |
| `components/boot_delay/` | 冷启动延时组件：SSD1306 四脚模块上电慢，初始化前先等待 5s（分片喂狗，不会触发 WDT 复位） |
| `fonts/` | OLED 界面用 Departure Mono 字体（MIT，见 `fonts/LICENSE`） |
| `daikin-bry88ab151k-pm2005-oled.yaml` | PM2005 配置（CM1106 `V1.0x`，早期银色金属外壳批次），`bit0_inverted: true` |
| `daikin-bry88ab151k-pm2105-oled.yaml` | PM2105 配置（CM1106 `V5.0x`，早期黑色塑料外壳批次），`bit0_inverted: false` |
| `firmware/2026/*-factory.bin` | 完整刷写镜像（从 `0x0` 写入，Web ESPHome / esptool 用） |
| `firmware/2026/*-ota.bin` | OTA 升级镜像（网页 OTA / ESPHome 仪表盘用） |

与上游 `header/*.h` + 原 YAML 的两点差异：

- 上游 YAML 用 `includes: cm1106.h` 的裸头文件 + `custom` 平台，需要按上面第 2 步把头文件**复制到 ESPHome 配置目录**才能编译；本版改为仓库内本地组件，配好 `external_components` 即可直接编译，无需复制头文件。
- PM2005/PM2105 的差异被收敛成 YAML 里的两个参数：`type:`（`PM2005`/`PM2105`）和 `bit0_inverted:`（`true`/`false`），固件与驱动完全共用一份。

### 编译

环境：ESPHome `2026.11.0`（ESP-IDF 5.5），在仓库根目录执行：

```bash
esphome compile daikin-bry88ab151k-pm2005-oled.yaml
esphome compile daikin-bry88ab151k-pm2105-oled.yaml
```

> 环境说明（仅供参考）：本项目的 YAML 与组件是在 **VS Code + PlatformIO（pio）** 环境中编辑、并用 ESPHome CLI 编译验证的；不要求使用相同环境，ESPHome 仪表盘或官方命令行同样可以编译。

编译产物在 `.esphome/build/daikin-air-sensor/build/`：`firmware.factory.bin`（整包）与 `firmware.ota.bin`（OTA）。两份配置的 `esphome.name` 相同，**编译产物会互相覆盖**，需要哪种就先编译哪种并另存。

### 刷写

- 新机 / 救砖：用 `firmware/2026/daikin-bry88ab151k-pm2005-factory.bin` 或 `-pm2105-factory.bin`，从 `0x0` 整包写入（[Web ESPHome](https://web.esphome.io/) 或 `esptool.py write_flash 0x0 <file>`）。
- 已在跑 ESPHome、要升级：如果你使用的是原作者的固件，必须要使用串口连接转换连接器到主板上刷写，因为本固件升级了ESPHome 2026.11.0的大版本，底层架构发生了极大变化，除非你自己更新过ESP home版本，否则不线刷的话，可能会变砖。
- WiFi 凭据没有写进 YAML，也没有打进 bin：首次上电连 `DAIKIN Fallback Hotspot` 配网即可；如需固定凭据，取消 YAML 中 `ssid`/`password` 两行的注释后自行填写。

## 参考链接
- 感谢C佬的[视频教程](https://www.bilibili.com/video/BV1PY4y1y7sE)
- 感谢[严泽远](https://github.com/nixieclock)大佬的PM2.5传感器[PM2105](http://www.gassensor.com.cn/ParticulateSensor/info_itemid_104.html) Header文件以及各种证件照
- 感谢MiWyf大佬的甲醛传感器[ZE08-CH20](http://style.winsensor.com/pro_pdf/ZE08-CH2O.pdf) [Header文件](https://github.com/miwyf/ze08-ch2o-esphome)
- [CM1106二氧化碳传感器](http://www.gassensor.com.cn/CO2Sensor/info_itemid_94.html) [Header文件](https://github.com/LeoDJ/ESPHome_Nodes)

## 来源与致谢

- 本项目基于 [louliangsheng/daikin-air-sensor](https://github.com/louliangsheng/daikin-air-sensor)（BRY88AA151K/BRY88AB151K 大金空气传感器 ESPHome 固件）二次开发，主要改造集中在 CM1106 二氧化碳驱动：按设备固件版本适配 STATUS Bit0 预热位极性（`bit0_inverted`）、`0x1E` 固件版本按 LEN 动态解析、状态位分级发布与诊断日志等，详见上文「CM1106 固件版本差异」与「改造说明」两节。
- 上游仓库未单独声明开源 License（GitHub 显示 `license: null`）。本项目沿用上游的开放引用方式，不额外附加限制性协议；代码仅供个人学习与交流使用，不用于商业用途。如需商用或再分发，请先取得原作者及下列各位参考作者的授权，并保留出处与致谢。
- 感谢原作者 [louliangsheng](https://github.com/louliangsheng)，以及原 README 致谢中的 C佬（[视频教程](https://www.bilibili.com/video/BV1PY4y1y7sE)）、[严泽远](https://github.com/nixieclock)（PM2105/PM2005 Header 与资料）、MiWyf（[ZE08-CH2O Header](https://github.com/miwyf/ze08-ch2o-esphome)）、[LeoDJ](https://github.com/LeoDJ/ESPHome_Nodes)（CM1106 Header）。
- 本项目对上述硬件协议差异的实测记录：PM2005 主机 CM1106 固件 `V1.06.3.00`（STATUS Bit0 极性反转、`0x1E` 返回 15 字节），PM2105 主机 CM1106 固件 `V5.06.3.02`（STATUS Bit0 符合规格、`0x1E` 返回 14 字节）。
