# 安装

返回 [README.zh_CN.md](README.zh_CN.md)。英文版：[INSTALL.md](INSTALL.md)。

## 安装依赖

```bash
sudo apt update
sudo apt install -y gcc make dkms "linux-headers-$(uname -r)"
```

在最小 Debian 或 fnOS shell 中，`modinfo` 可能不在默认用户 `PATH` 中。需要时使用 `/usr/sbin/modinfo`。

## DKMS 安装

默认安装：

```bash
sudo ./scripts/install.sh
sudo modprobe platform_fans_hwmon
```

限定当前第一个平台安装：

```bash
sudo ./scripts/install.sh --platform intel-nuc-ec-v9
```

诊断时指定运行平台：

```bash
sudo modprobe platform_fans_hwmon platform=intel-nuc-ec-v9
```

`platform=` 参数默认仍要求平台身份检查通过。

## 验证

```bash
/usr/sbin/modinfo platform_fans_hwmon
sensors | sed -n '/intel_nuc_ec/,/^$/p'
```

查找 hwmon 设备：

```bash
for h in /sys/class/hwmon/hwmon*; do
  [ "$(cat "$h/name" 2>/dev/null)" = "intel_nuc_ec" ] || continue
  echo "hwmon=$h"
  cat "$h"/fan*_label
  cat "$h"/fan*_input
done
```

Intel NUC9 预期标签：

```text
CPU Fan
System Fan 1
System Fan 2
```

## DKMS 卸载

```bash
sudo ./scripts/uninstall.sh
```

## 手动构建和测试加载

```bash
make
sudo insmod platform_fans_hwmon.ko
sensors | sed -n '/intel_nuc_ec/,/^$/p'
sudo rmmod platform_fans_hwmon
```

## 手动清理

```bash
make clean
```
