# Intel NUC EC hwmon

[English](README.md)

Intel NUC9 EC V9 风扇转速计的只读 Linux hwmon 驱动。

本模块通过标准 hwmon ABI 暴露 Intel NUC9 EC 风扇 RPM，因此 `sensors`、CoolerControl 和监控采集器可以像读取普通 hwmon 传感器一样读取 `fan1_input`、`fan2_input` 和 `fan3_input`。

## 支持的硬件

第一版支持范围刻意保持狭窄：

- Intel NUC9 Extreme Compute Element / Kit：
  - `NUC9i5QNB`
  - `NUC9i7QNB`
  - `NUC9i9QNB`
  - `NUC9i5QNX`
  - `NUC9i7QNX`
  - `NUC9i9QNX`
- Intel NUC9 Pro Compute Element / Kit：
  - `NUC9V7QNB`
  - `NUC9VXQNB`
  - `NUC9V7QNX`
  - `NUC9VXQNX`

模块还要求 Intel NUC EC identifier 为 `SPG_EC` 或 `ELM_EC`。

## 暴露的传感器

```text
fan1_label = CPU Fan
fan1_input = EC V9 register 0x41B

fan2_label = System Fan 1
fan2_input = EC V9 register 0x41E

fan3_label = System Fan 2
fan3_input = EC V9 register 0x421
```

模块是只读的。它不暴露 PWM 控制，不写入 EC 寄存器。

## 安装

DKMS 安装、手动测试加载、验证和卸载步骤见 [INSTALL.zh_CN.md](INSTALL.zh_CN.md)。

## 验证

查找 hwmon 设备：

```bash
for h in /sys/class/hwmon/hwmon*; do
  [ "$(cat "$h/name" 2>/dev/null)" = "intel_nuc_ec" ] || continue
  echo "hwmon=$h"
  cat "$h"/fan*_label
  cat "$h"/fan*_input
done
```

预期 label：

```text
CPU Fan
System Fan 1
System Fan 2
```

已在 Intel NUC9i7QNX / NUC9i7QNB 与 fnOS kernel `6.18.18-trim` 上验证：

```text
intel_nuc_ec-isa-0000
Adapter: ISA adapter
CPU Fan:      1755 RPM
System Fan 1: 2030 RPM
System Fan 2: 2180 RPM
```

DKMS 安装验证结果：

```text
filename: /lib/modules/6.18.18-trim/updates/dkms/intel_nuc_ec_hwmon.ko
dkms: intel-nuc-ec-hwmon/0.1.0, 6.18.18-trim, x86_64: installed
lsmod: intel_nuc_ec_hwmon
```

## 安全性

本模块只读取 EC MMIO 寄存器。它不写入风扇控制寄存器，不修改风扇曲线，也不暴露 `pwm*` 控制接口。

## 许可证

本项目使用 GNU General Public License version 2。
