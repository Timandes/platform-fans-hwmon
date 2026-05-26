# NAS-9 IT8613E Validation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Validate whether an IT8613E-capable `it87` driver can expose NAS-9 fan RPM readings, then document NAS-9 as externally supported only if runtime validation succeeds.

**Architecture:** Treat NAS-9 as an external Linux hwmon driver validation task. First reject the withdrawn v4 IT8613E patch series, then identify a fixed IT8613E candidate before any module is built or loaded on NAS-9. Only after a fixed candidate passes source inspection should the worker build a temporary `it87.ko`, collect fan RPM evidence through the normal hwmon ABI, and update this project with documentation and static tests.

**Tech Stack:** Linux hwmon ABI, `it87` kernel module, Debian 12 remote shell, kernel module build tools, POSIX shell, Python `unittest` static tests, Markdown documentation.

---

## Scope Check

This plan covers one subsystem: validating external `it87` support for the NAS-9 IT8613E Super I/O chip and documenting the result. It does not implement a `platform_fans_hwmon` backend. If the external driver path fails, the next step is a separate design for a board-specific read-only backend with exact tachometer registers and RPM conversion rules.

## File Structure

- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/docs/validation/nas9-it8613e-it87.txt`
  - Raw validation transcript copied from NAS-9 after a successful temporary driver load.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/docs/supported-platforms.md`
  - Add NAS-9 as an externally supported platform only after `fan*_input` is observed.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/README.md`
  - Add NAS-9 to the externally supported platform summary only after validation succeeds.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/README.zh_CN.md`
  - Add the Chinese NAS-9 externally supported platform summary only after validation succeeds.
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/tests/test_static_project.py`
  - Add static tests that require the NAS-9 docs and validation transcript to include the important facts.

Remote-only temporary files on NAS-9:

- Create: `/tmp/it8613e-it87-candidate/`
  - Temporary build workspace for the selected fixed IT8613E candidate.
- Create: `/tmp/nas9-it8613e-it87-validation.txt`
  - Remote validation transcript.
- Create: `/tmp/nas9-it8613e-it87-failed.txt`
  - Remote failure transcript when the driver cannot build, bind, or expose nonzero fan RPM.

## Task 1: Reconfirm NAS-9 Baseline

**Files:**
- Remote only: `/tmp/nas9-it8613e-baseline.txt`

- [ ] **Step 1: Capture current NAS-9 hardware and hwmon state**

Run:

```bash
ssh -t timandes@nas-9.timandes.net 'cat > /tmp/collect-nas9-it8613e-baseline.sh <<'"'"'EOF'"'"'
#!/bin/sh
set -eu
{
  echo "== date =="
  date -Is
  echo "== uname =="
  uname -a
  echo "== dmi =="
  for f in sys_vendor product_name product_version board_vendor board_name board_version bios_vendor bios_version; do
    printf "%s=" "$f"
    cat "/sys/class/dmi/id/$f" 2>/dev/null || true
  done
  echo "== current hwmon =="
  for h in /sys/class/hwmon/hwmon*; do
    [ -e "$h/name" ] || continue
    printf "hwmon=%s name=" "$h"
    cat "$h/name"
    for f in "$h"/fan*_input "$h"/fan*_label "$h"/temp*_label "$h"/temp*_input; do
      [ -e "$f" ] || continue
      printf "%s=" "$f"
      cat "$f" 2>/dev/null || true
    done
  done
  echo "== sensors =="
  sensors 2>/dev/null || true
  echo "== sensors-detect =="
  sudo /usr/sbin/sensors-detect --auto
} | tee /tmp/nas9-it8613e-baseline.txt
EOF
chmod +x /tmp/collect-nas9-it8613e-baseline.sh
/tmp/collect-nas9-it8613e-baseline.sh'
```

- [ ] **Step 2: Verify baseline still matches the design spec**

Run:

```bash
ssh timandes@nas-9.timandes.net 'grep -E "DS2308|Dinson|IT8613E|0xa30|to-be-written|k10temp|amdgpu|nvme" /tmp/nas9-it8613e-baseline.txt'
```

Expected: output includes `DS2308`, `Dinson`, `ITE IT8613E Super IO Sensors`, `address 0xa30`, `driver \`to-be-written\``, and no nonzero `fan*_input` line.

## Task 2: Select A Fixed IT8613E Candidate Before Building

**Files:**
- Remote create: `/tmp/it8613e-it87-candidate-notes.txt`
- Remote create on failure: `/tmp/nas9-it8613e-it87-failed.txt`

- [ ] **Step 1: Record the withdrawn v4 patch safety gate**

Run:

```bash
ssh timandes@nas-9.timandes.net 'cat > /tmp/it8613e-it87-candidate-notes.txt <<'"'"'EOF'"'"'
The upstream v4 IT8613E patch series is withdrawn.
The worker must not load the withdrawn v4 patch series on NAS-9.
Known issue: NULL pointer dereference in the IT8613E probe path.
A fixed IT8613E candidate must be selected before any temporary it87.ko is built or loaded.
EOF
cat /tmp/it8613e-it87-candidate-notes.txt'
```

Expected: output includes `withdrawn`, `NULL pointer`, `must not load the withdrawn v4 patch series`, and `fixed IT8613E candidate`.

- [ ] **Step 2: Verify the running kernel can build external modules**

Run:

```bash
ssh timandes@nas-9.timandes.net 'test -e /lib/modules/$(uname -r)/build/Makefile && echo "kernel build tree present: /lib/modules/$(uname -r)/build"'
```

Expected: output starts with `kernel build tree present:`. If this fails, stop and record `/tmp/nas9-it8613e-it87-failed.txt` with:

```bash
ssh timandes@nas-9.timandes.net 'printf "%s\n" "missing kernel build tree for $(uname -r)" > /tmp/nas9-it8613e-it87-failed.txt'
```

- [ ] **Step 3: Confirm the withdrawn v4 patch is not an acceptable runtime candidate**

Run:

```bash
ssh timandes@nas-9.timandes.net 'cat >> /tmp/it8613e-it87-candidate-notes.txt <<'"'"'EOF'"'"'
Rejected candidate:
- Source: https://www.spinics.net/lists/kernel/msg6003724.html
- Follow-up: https://www.spinics.net/lists/kernel/msg6009013.html
- Reason: the series was dropped after a NULL pointer issue was reported.
EOF
grep -E "Rejected candidate|6003724|6009013|NULL pointer" /tmp/it8613e-it87-candidate-notes.txt'
```

Expected: output records the rejected candidate and the NULL pointer reason.

- [ ] **Step 4: Stop if no fixed candidate is available**

Run:

```bash
ssh timandes@nas-9.timandes.net 'cat > /tmp/nas9-it8613e-it87-failed.txt <<'"'"'EOF'"'"'
No fixed IT8613E candidate has been selected.
The withdrawn v4 upstream patch series was not loaded.
Next step: find a maintained IT8613E-capable it87 candidate or start a new board-specific read-only backend design.
EOF
cat /tmp/nas9-it8613e-it87-failed.txt'
```

Expected: output states that no module was loaded. If a fixed candidate is found later, replace this stop step with candidate-specific source inspection and build steps in a new plan update before loading anything.

## Task 3: Runtime-Validate A Fixed Temporary it87 Module

**Files:**
- Remote create: `/tmp/nas9-it8613e-it87-validation.txt`
- Remote create on failure: `/tmp/nas9-it8613e-it87-failed.txt`

- [ ] **Step 1: Verify the candidate safety notes before loading**

Run:

```bash
ssh timandes@nas-9.timandes.net 'grep -E "fixed IT8613E candidate|must not load the withdrawn v4 patch series|NULL pointer" /tmp/it8613e-it87-candidate-notes.txt'
```

Expected: output includes the safety gate. If `/tmp/it8613e-it87-candidate/module/it87.ko` does not exist, stop here and use Task 7 failure handling.

- [ ] **Step 2: Load the fixed temporary module and collect hwmon output**

Run this only after `/tmp/it8613e-it87-candidate/module/it87.ko` exists and its source has been inspected to avoid the withdrawn v4 NULL pointer issue:

```bash
ssh -t timandes@nas-9.timandes.net 'sudo sh -c '"'"'
set -eu
out=/tmp/nas9-it8613e-it87-validation.txt
fail=/tmp/nas9-it8613e-it87-failed.txt
rm -f "$out" "$fail"
rmmod it87 2>/dev/null || true
rmmod hwmon_vid 2>/dev/null || true
insmod /tmp/it8613e-it87-candidate/module/it87.ko
{
  echo "== date =="
  date -Is
  echo "== lsmod =="
  lsmod | egrep "^(it87|hwmon_vid)"
  echo "== modinfo =="
  /usr/sbin/modinfo /tmp/it8613e-it87-candidate/module/it87.ko | egrep "^(filename|description|vermagic|depends):"
  echo "== hwmon =="
  for h in /sys/class/hwmon/hwmon*; do
    [ -e "$h/name" ] || continue
    printf "hwmon=%s name=" "$h"
    cat "$h/name"
    for f in "$h"/fan*_input "$h"/fan*_label "$h"/temp*_label "$h"/temp*_input; do
      [ -e "$f" ] || continue
      printf "%s=" "$f"
      cat "$f" 2>/dev/null || true
    done
  done
  echo "== sensors =="
  sensors 2>/dev/null || true
  echo "== dmesg it87 tail =="
  dmesg | grep -iE "it87|it8613|hwmon" | tail -80 || true
} | tee "$out"
if ! grep -Eq "name=it8613|it8613-isa|IT8613E" "$out"; then
  cp "$out" "$fail"
  echo "it87 did not expose an IT8613E hwmon device" >&2
  exit 2
fi
if ! awk -F= "/fan[0-9]+_input=/ { if (\\$2 + 0 > 0) found=1 } END { exit found ? 0 : 1 }" "$out"; then
  cp "$out" "$fail"
  echo "it87 exposed no nonzero fan RPM" >&2
  exit 3
fi
'"'"''
```

Expected: command exits 0. The transcript contains an IT8613E-backed hwmon device and at least one nonzero `fan*_input`.

- [ ] **Step 3: Remove the temporary module after capture**

Run:

```bash
ssh -t timandes@nas-9.timandes.net 'sudo sh -c '"'"'rmmod it87 2>/dev/null || true; rmmod hwmon_vid 2>/dev/null || true; lsmod | egrep "^(it87|hwmon_vid)" || true'"'"''
```

Expected: no `it87` line remains in `lsmod`.

- [ ] **Step 4: Inspect the captured transcript**

Run:

```bash
ssh timandes@nas-9.timandes.net 'cat /tmp/nas9-it8613e-it87-validation.txt'
```

Expected: output includes `fan`, `RPM` or `fan*_input`, and an IT8613E device name. If Step 1 failed, inspect `/tmp/nas9-it8613e-it87-failed.txt` instead and stop before modifying project documentation.

## Task 4: Add Tests For Successful NAS-9 Documentation

**Files:**
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/tests/test_static_project.py`

- [ ] **Step 1: Write the failing static assertions**

In `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/tests/test_static_project.py`, extend the `readme` expected tuple in `test_docs_explain_platform_framework_scope_and_links` so it contains:

```python
                "nas9-ds2308-it8613e-it87",
                "IT8613E",
                "it87",
```

Extend the `supported` expected tuple in `test_docs_explain_platform_framework_scope_and_links` so it contains:

```python
                "nas9-ds2308-it8613e-it87",
                "DS2308",
                "Dinson",
                "IT8613E",
                "0xa30",
                "docs/validation/nas9-it8613e-it87.txt",
```

Extend the `readme` expected tuple in `test_zh_cn_docs_use_generalized_names_and_localized_links` so it contains:

```python
                "nas9-ds2308-it8613e-it87",
                "IT8613E",
                "it87",
```

Add this new test method to `StaticProjectTests`:

```python
    def test_nas9_it8613e_validation_transcript_is_recorded(self):
        validation = self.read("docs/validation/nas9-it8613e-it87.txt")

        self.assert_contains_all(
            validation,
            (
                "== lsmod ==",
                "== hwmon ==",
                "== sensors ==",
                "it87",
                "IT8613E",
                "fan",
            ),
        )
```

- [ ] **Step 2: Run the focused tests to verify they fail**

Run:

```bash
python3 -m unittest tests.test_static_project.StaticProjectTests.test_docs_explain_platform_framework_scope_and_links tests.test_static_project.StaticProjectTests.test_zh_cn_docs_use_generalized_names_and_localized_links tests.test_static_project.StaticProjectTests.test_nas9_it8613e_validation_transcript_is_recorded -q
```

Expected: FAIL because the NAS-9 documentation and validation transcript are not yet present.

## Task 5: Add Successful Validation Documentation

**Files:**
- Create: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/docs/validation/nas9-it8613e-it87.txt`
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/docs/supported-platforms.md`
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/README.md`
- Modify: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/README.zh_CN.md`

- [ ] **Step 1: Copy the NAS-9 validation transcript into the repo**

Run:

```bash
mkdir -p /Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/docs/validation
scp timandes@nas-9.timandes.net:/tmp/nas9-it8613e-it87-validation.txt /Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/docs/validation/nas9-it8613e-it87.txt
```

Expected: local file exists and contains the successful remote transcript.

- [ ] **Step 2: Update supported platforms**

In `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/docs/supported-platforms.md`, add this row after the MS-01 row:

```markdown
| `nas9-ds2308-it8613e-it87` | `it8613` | Validated IT8613E-capable `it87` | Validated on NAS-9 / DS2308 |
```

Add this section after `minisforum-ms01-nct6775`:

````markdown
## nas9-ds2308-it8613e-it87

This platform is supported by an IT8613E-capable `it87` hwmon driver. It does
not require `platform_fans_hwmon`.

Observed DMI data:

- System vendor: `Default string`
- Product name: `DS2308`
- Board vendor: `Default string`
- Board name: `Dinson`
- BIOS version: `DS2308.A..002`

Detected sensor chip:

```text
ITE IT8613E Super IO Sensors
ISA address 0xa30
driver it87 with IT8613E support
```

Validation transcript:

```text
docs/validation/nas9-it8613e-it87.txt
```

Do not add NAS-9-specific code to `platform_fans_hwmon` while this external
driver path exposes the required fan RPM values.
````

- [ ] **Step 3: Update English README**

In `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/README.md`, add this item under `Externally supported platforms`:

```markdown
- `nas9-ds2308-it8613e-it87`
  - NAS-9 / DS2308 / board `Dinson`
  - Sensor chip: ITE `IT8613E`
  - External Linux hwmon driver path: IT8613E-capable `it87`
  - hwmon name: `it8613`
```

- [ ] **Step 4: Update Chinese README**

In `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/README.zh_CN.md`, add this item under `外部驱动支持的平台`:

```markdown
- `nas9-ds2308-it8613e-it87`
  - NAS-9 / DS2308 / board `Dinson`
  - 传感器芯片：ITE `IT8613E`
  - 外部 Linux hwmon 驱动路径：支持 IT8613E 的 `it87`
  - hwmon name：`it8613`
```

## Task 6: Verify And Commit Successful Documentation

**Files:**
- Verify all files from Tasks 4 and 5.

- [ ] **Step 1: Run the focused static tests**

Run:

```bash
python3 -m unittest tests.test_static_project.StaticProjectTests.test_docs_explain_platform_framework_scope_and_links tests.test_static_project.StaticProjectTests.test_zh_cn_docs_use_generalized_names_and_localized_links tests.test_static_project.StaticProjectTests.test_nas9_it8613e_validation_transcript_is_recorded -q
```

Expected: PASS.

- [ ] **Step 2: Run the full static test suite**

Run:

```bash
python3 -m unittest discover -s tests -q
```

Expected: `OK`.

- [ ] **Step 3: Run shell syntax checks**

Run:

```bash
sh -n scripts/install.sh && sh -n scripts/uninstall.sh && sh -n tools/collect-platform-info.sh
```

Expected: command exits 0.

- [ ] **Step 4: Inspect staged diff**

Run:

```bash
git diff -- README.md README.zh_CN.md docs/supported-platforms.md docs/validation/nas9-it8613e-it87.txt tests/test_static_project.py
```

Expected: diff only adds NAS-9 external-driver documentation, the captured validation transcript, and static tests.

- [ ] **Step 5: Commit successful validation docs**

Run:

```bash
git add README.md README.zh_CN.md docs/supported-platforms.md docs/validation/nas9-it8613e-it87.txt tests/test_static_project.py
git commit -m "docs: record nas9 it8613e validation"
```

Expected: commit succeeds. Commit message must not include `Co-Authored-By`.

## Task 7: Failure Handling When IT8613E-Capable it87 Does Not Work

**Files:**
- Create only if validation fails: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/docs/validation/nas9-it8613e-it87-failed.txt`

- [ ] **Step 1: Copy the failed validation transcript**

Run this only if Task 3 exits nonzero:

```bash
mkdir -p /Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/docs/validation
scp timandes@nas-9.timandes.net:/tmp/nas9-it8613e-it87-failed.txt /Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/docs/validation/nas9-it8613e-it87-failed.txt
```

Expected: local failure transcript exists.

- [ ] **Step 2: Commit the failure evidence without adding NAS-9 as supported**

Run:

```bash
git add docs/validation/nas9-it8613e-it87-failed.txt
git commit -m "docs: record nas9 it8613e validation failure"
```

Expected: commit succeeds. Do not modify `README.md`, `README.zh_CN.md`, or `docs/supported-platforms.md` in the failure path.

- [ ] **Step 3: Stop before implementation**

Run:

```bash
git status --short --branch
```

Expected: clean working tree except for the new failure-evidence commit. The next work item must be a new brainstorming/design cycle for a board-specific read-only IT8613E backend.

## References

- Spec: `/Users/timandes/Projects/fnos/nuc9-ec-hwmon/.worktrees/feature-platform-fans-hwmon-design/docs/superpowers/specs/2026-05-26-nas9-it8613e-design.md`
- Linux kernel IT8613E patch series: <https://www.spinics.net/lists/kernel/msg6003724.html>
- IT8613E configuration patch: <https://www.spinics.net/lists/kernel/msg6003730.html>
- Linux kernel `it87` documentation: <https://dri.freedesktop.org/docs/drm/hwmon/it87.html>
- LKDDb `SENSORS_IT87`: <https://cateee.net/lkddb/web-lkddb/SENSORS_IT87.html>
