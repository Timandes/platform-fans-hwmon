# 安装

[README.zh_CN.md](README.zh_CN.md)

本项目可以作为 DKMS 管理的内核模块安装，也可以手动加载进行测试。

日常使用建议选择 DKMS。手动加载只适合短时间验证。

## 安装依赖

在目标机器上安装构建工具和匹配当前内核的 headers：

```bash
sudo apt update
sudo apt install -y gcc make dkms "linux-headers-$(uname -r)"
```

在最小化 Debian/fnOS shell 中，一些工具可能不在普通用户默认 `PATH` 中。需要时使用完整路径，例如 `/usr/sbin/modinfo`、`/sbin/insmod` 和 `/sbin/rmmod`。

## DKMS 安装

在项目目录执行：

```bash
sudo ./scripts/install.sh
```

验证已安装模块：

```bash
/usr/sbin/modinfo intel_nuc_ec_hwmon
sensors | sed -n '/intel_nuc_ec/,/^$/p'
```

预期 `modinfo` 字段包含：

```text
license:        GPL
author:         Timandes White
name:           intel_nuc_ec_hwmon
```

预期 `sensors` 输出包含：

```text
intel_nuc_ec-isa-0000
Adapter: ISA adapter
CPU Fan:      1755 RPM
System Fan 1: 2030 RPM
System Fan 2: 2180 RPM
```

## DKMS 卸载

在项目目录执行：

```bash
sudo ./scripts/uninstall.sh
```

验证模块不再加载：

```bash
lsmod | grep intel_nuc_ec_hwmon || echo "intel_nuc_ec_hwmon is not loaded"
```

## 手动构建和测试加载

手动加载不会在重启后保留，也不会在内核升级后自动重编译。

构建：

```bash
make
```

加载：

```bash
sudo insmod intel_nuc_ec_hwmon.ko
```

验证：

```bash
sensors | sed -n '/intel_nuc_ec/,/^$/p'
```

卸载：

```bash
sudo rmmod intel_nuc_ec_hwmon
```

## 手动清理

如果使用的是 `make install` 而不是 DKMS：

```bash
sudo make uninstall
```

如果使用了 DKMS，优先执行：

```bash
sudo ./scripts/uninstall.sh
```
