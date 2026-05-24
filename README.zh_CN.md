# Platform Fans hwmon

`platform-fans-hwmon` 是一个只读 Linux hwmon 框架，用于暴露现有内核驱动尚未支持的平台特定风扇转速传感器。

内核模块名为 `platform_fans_hwmon`。它提供通用 hwmon core 和平台特定描述。第一个支持的平台是 Intel NUC9 EC V9，导出的 hwmon name 保持为 `intel_nuc_ec`。

本项目通过标准 hwmon ABI 暴露风扇 RPM，使 `sensors`、CoolerControl 和监控系统可以读取 `fan*_input` 与 `fan*_label`。

## 支持的平台

见 [docs/supported-platforms.md](docs/supported-platforms.md)。

当前平台：

- `intel-nuc-ec-v9`
  - Intel NUC9 Extreme Compute Element / Kit
  - Intel NUC9 Pro Compute Element / Kit
  - EC identifier：`SPG_EC` 或 `ELM_EC`
  - hwmon name：`intel_nuc_ec`

## Intel NUC9 EC V9 传感器

```text
fan1_label = CPU Fan
fan1_input = EC V9 register 0x41B

fan2_label = System Fan 1
fan2_input = EC V9 register 0x41E

fan3_label = System Fan 2
fan3_input = EC V9 register 0x421
```

## 安装

见 [INSTALL.zh_CN.md](INSTALL.zh_CN.md)。

默认 DKMS 安装：

```bash
sudo ./scripts/install.sh
sudo modprobe platform_fans_hwmon
```

限定平台安装：

```bash
sudo ./scripts/install.sh --platform intel-nuc-ec-v9
```

## 贡献平台支持

见 [docs/contributing-platform.md](docs/contributing-platform.md) 和 [docs/platform-template.md](docs/platform-template.md)。

平台支持必须有明确匹配条件，并在真实硬件上验证。本驱动不会扫描未知 EC、MMIO 或 IO port 范围来猜测风扇转速。

## 安全

默认平台支持是只读的。本模块不暴露 PWM 控制，不修改风扇曲线，也不写入风扇控制寄存器。

风扇控制属于另一类风险，不属于默认模块范围。

## 许可证

本项目使用 GNU General Public License version 2。
