# Intel NUC EC hwmon Design

## 背景

NAS-11 使用 Intel NUC9 平台，DMI 显示主板为 `NUC9i7QNB`、产品为 `NUC9i7QNX`。fnOS / Debian 12 当前的 `sensors` 与 `/sys/class/hwmon` 只能看到 CPU、NVMe、PCH、Wi-Fi、USB-C 等传感器，不能看到风扇 RPM。ACPI 注册了 `\_TZ_.FAN0` 到 `\_TZ_.FAN4` 五个 `PNP0C0B` fan cooling device，但这些设备只有 `cur_state=0`、`max_state=1`，不提供 RPM。

已用只读脚本验证 Intel NUC EC MMIO 可读：

```text
LGMR=0xFE410001
EC_MMIO_BASE=0xFE410000
EC_MMIO_ENABLED=1
IDENTIFIER=SPG_EC
FAN1_TYPE=FANCPU FAN1_RPM=1755
FAN2_TYPE=FANSYS1 FAN2_RPM=2067
FAN3_TYPE=FANSYS2 FAN3_RPM=2195
```

VirtualSMC 的 Intel NUC EC 映射显示，`QXCFL579` 固件属于 `Intel_EC_V9`，覆盖 NUC9 Extreme Compute Element / Kit，例如 `NUC9i5QNB`、`NUC9i7QNB`、`NUC9i9QNB` 及对应 `QNX` 产品。EC V9 风扇寄存器为 `0x41B`、`0x41E`、`0x421`，大端 16 位。

## 目标

构建一个独立维护的 out-of-tree Linux hwmon 内核模块，使 Intel NUC9 EC V9 平台的三个风扇 RPM 以标准 hwmon ABI 暴露给 `sensors`、CoolerControl 和监控系统。

第一版只支持 NUC9 / EC V9，不做泛化的 Intel NUC 全系列支持，不做风扇控制。

## 非目标

- 不写 EC 寄存器。
- 不导出 `pwm*`、`pwm*_enable` 或任何风扇控制接口。
- 不支持普通台式机 Super I/O、BMC/IPMI、ITE/Nuvoton/Winbond 主板传感器。
- 不在第一版支持 NUC8、NUC10、NUC11 或其他 EC 版本。
- 不提交 Linux upstream；项目结构保留未来 upstream 的可能性即可。

## 方案选择

采用 **NUC9 专用 out-of-tree hwmon 内核模块 + DKMS 支持**。

已排除方案：

- 只保留用户态脚本：实现简单，但不能被 `sensors`、CoolerControl 和标准 hwmon 消费者识别。
- 手动 `.ko` 无 DKMS：适合验证，但 fnOS 内核升级后维护成本高。
- 泛化 Intel NUC EC V8/V9/VA/VB：范围过大，需要更多硬件样本和寄存器验证。

模块命名：

- 项目目录：`nuc9-ec-hwmon`
- 模块名：`intel_nuc_ec_hwmon`
- hwmon name：`intel_nuc_ec`

模块名不带 `nuc9`，为未来支持更多 Intel NUC EC 版本保留空间；第一版代码和 README 必须明确只支持 NUC9 EC V9。

## 项目结构

```text
/Users/timandes/Projects/fnos/nuc9-ec-hwmon
  README.md
  Makefile
  dkms.conf
  intel_nuc_ec_hwmon.c
  scripts/
    install.sh
    uninstall.sh
  docs/
    superpowers/
      specs/
        2026-05-24-intel-nuc-ec-hwmon-design.md
```

## 驱动行为

模块加载流程：

1. 通过 DMI 表匹配 Intel NUC9 系列。
2. 读取 LPC 设备 `0000:00:1f.0` PCI config offset `0x98` 的 `LGMR`。
3. 验证 EC MMIO window enabled，且 base 非 0。
4. `ioremap` EC MMIO base。
5. 读取 EC identifier `0x400-0x405`，必须为 `SPG_EC` 或 `ELM_EC`。
6. 注册 hwmon device。

hwmon 导出：

```text
fan1_label = "CPU Fan"
fan1_input = read_be16(ec + 0x41B)

fan2_label = "System Fan 1"
fan2_input = read_be16(ec + 0x41E)

fan3_label = "System Fan 2"
fan3_input = read_be16(ec + 0x421)
```

文件名使用 Linux hwmon 标准 ABI 的 `fanX_input` / `fanX_label`，不使用自定义 sysfs 文件名。label 内容使用用户友好的名称。

## 硬件匹配

DMI 匹配第一版覆盖 NUC9 Extreme 和 NUC9 Pro 系列：

- `NUC9i5QNB`
- `NUC9i7QNB`
- `NUC9i9QNB`
- `NUC9i5QNX`
- `NUC9i7QNX`
- `NUC9i9QNX`
- `NUC9V7QNB`
- `NUC9VXQNB`
- `NUC9V7QNX`
- `NUC9VXQNX`

匹配策略：

- board name 或 product name 命中上述型号之一即可进入 EC 检查。
- EC identifier 必须命中 `SPG_EC` 或 `ELM_EC`。
- DMI 与 EC identifier 双重条件都满足时才注册 hwmon。

这样避免模块在非 NUC9 机器上误读 EC MMIO。

## 错误处理

- DMI 不匹配：返回 `-ENODEV`，不注册 hwmon。
- LPC 设备不存在：返回 `-ENODEV`。
- `LGMR` 未启用或 base 为 0：返回 `-ENODEV`。
- `ioremap` 失败：返回 `-ENOMEM`。
- EC identifier 不匹配：释放映射并返回 `-ENODEV`。
- RPM 读取值为 0 或异常大：第一版原样导出，不做策略判断；README 说明读数来自 EC 原始 RPM。

模块卸载时必须注销 hwmon device 并 `iounmap` EC MMIO。

## 构建与部署

手动构建：

```bash
make
sudo insmod intel_nuc_ec_hwmon.ko
sensors
sudo rmmod intel_nuc_ec_hwmon
```

手动安装：

```bash
sudo make install
sudo depmod -a
sudo modprobe intel_nuc_ec_hwmon
```

DKMS 安装：

```bash
sudo ./scripts/install.sh
```

DKMS 卸载：

```bash
sudo ./scripts/uninstall.sh
```

NAS-11 当前已安装 `linux-headers-6.18.18-trim` 和 `make`，但未安装 `gcc`、`dkms`。正式部署前需要在目标机安装构建工具，或在匹配内核 headers 的构建环境中生成 `.ko`。

## 验证

基本验证：

```bash
make
sudo insmod intel_nuc_ec_hwmon.ko
dmesg | tail -50
sensors
find /sys/class/hwmon -maxdepth 2 -type f -name 'fan*_input' -o -name 'fan*_label'
sudo rmmod intel_nuc_ec_hwmon
```

预期 `sensors` 能看到类似：

```text
intel_nuc_ec-isa-0000
Adapter: ISA adapter
CPU Fan:        1755 RPM
System Fan 1:   2067 RPM
System Fan 2:   2195 RPM
```

回归验证：

- 与 `/tmp/nuc9-ec-fan-read.sh` 的输出对比，RPM 应在同一量级并随时间变化。
- 模块卸载后 `/sys/class/hwmon/.../intel_nuc_ec` 消失。
- 非匹配机器上加载应拒绝注册。

## 参考项目

未发现成熟的 Intel NUC9 / `QXCFL579` / `SPG_EC` / `Intel_EC_V9` Linux hwmon 项目可直接复用。

可参考的相邻项目：

- `passiveEndeavour/it5570-fan`：ITE IT5570 EC hwmon 模块，包含 DKMS 支持和 hwmon README 写法。
- `fxd0h/lattepanda-sigma-ec-hwmon`：LattePanda Sigma EC hwmon 驱动，项目结构包含 `dkms.conf`、Kconfig/MAINTAINERS 片段。
- `zeule/asus-ec-sensors`：ASUS EC sensors，已进入 mainline kernel，证明 EC register 到 hwmon 是可接受的驱动模式。
- `nbfc-linux/nbfc-linux`：用户态 EC 风扇控制工具，适合了解 EC 访问方式，但不适合作为本项目主体。

## 后续扩展

未来可以在不破坏第一版 ABI 的前提下增加：

- 更多 Intel NUC EC V9 机型 DMI。
- EC V9 温度/电压只读导出。
- NUC8/NUC10/NUC11 的 EC 版本映射。
- Kconfig、MAINTAINERS、Documentation/hwmon 文档片段，为 upstream 做准备。

