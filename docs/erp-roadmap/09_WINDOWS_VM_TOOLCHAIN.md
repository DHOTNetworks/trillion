# 09 — Windows VM Toolchain + Headless Qt Testing (v1.0)

## Machine

Windows 11 ARM64 under Parallels on MacBook Air M1. 210 GB free. Internet OK
(github.com + download.qt.io reachable). Bare: no Git/Python/CMake/Qt/MSVC.
Has reference apps: `C:\BusyWin`, `C:\BusyMagic`, `C:\Program Files\TallyPrime`.

## Target toolchain (ARM64-native where possible)

| Tool | Source (ARM64) | Notes |
|---|---|---|
| Python 3.12 | `python.org/ftp/python/3.12.7/python-3.12.7-arm64.exe` | Needed for `aqtinstall` + analyzers |
| Git | `github.com/git-for-windows` `Git-*-arm64.exe` | x64 emulated build also fine |
| CMake 3.31 | `Kitware/CMake` `cmake-*-windows-arm64.msi` | |
| Ninja | `ninja-build` `ninja-winarm64.zip` | Unzip to `C:\Tools\ninja`, add PATH |
| MSVC Build Tools 2022 | `aka.ms/vs/17/release/vs_BuildTools.exe` | Workload `VCTools` + ARM64 toolchain + Windows SDK (~4-6 GB, 20-40 min) |
| Qt 6.8 `win64_msvc2022_arm64` | via `pip install aqtinstall` | `aqt install-qt windows desktop 6.8.3 win64_msvc2022_arm64 -O C:\Qt` (~1.5 GB) |

Status at doc time: base toolchain DONE (verified 2026-10-12: Python 3.12.7,
Git 2.47.1, CMake 3.31.6 portable at `C:\Tools\cmake-3.31.6-windows-arm64`,
Ninja 1.12.1 at `C:\Tools\ninja`). SQLite headless proof already ran
(`db_check.py`: trial diff 0.00). MSVC + Qt NOT installed — DECIDED 2026-10-12: Mac-only compile, VM stays
headless-lite (no elevation available; CMake MSI failed with 1925).

## Install sequence (run in order, PowerShell admin)

```powershell
# 1. Python + Git + CMake (GUI installers in C:\Temp\erp-tools, tick "Add to PATH")
# 2. Ninja
Expand-Archive C:\Temp\erp-tools\ninja.zip -DestinationPath C:\Tools\ninja
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\Tools\ninja", "Machine")
# 3. MSVC (long — start and continue working)
vs_BuildTools.exe --quiet --wait --add Microsoft.VisualStudio.Workload.VCTools `
  --add Microsoft.VisualStudio.Component.VC.Tools.ARM64 `
  --add Microsoft.VisualStudio.Component.Windows11SDK.22621
# 4. Qt via aqt (after python on PATH)
pip install aqtinstall
aqt install-qt windows desktop 6.8.3 win64_msvc2022_arm64 -O C:\Qt
# 5. Configure (x64 emulation NOT needed — full ARM64 chain)
$env:Qt6_DIR="C:\Qt\6.8.3\msvc2022_arm64\lib\cmake\Qt6"
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target LogicBoardTestSuite EnginesTestSuite
```

## Headless test runs (no display needed)

```powershell
$env:QT_QPA_PLATFORM="offscreen"   # or "minimal"
$env:QT_OFFSCREEN_NO_GL="1"
.\build\LogicBoardTestSuite
.\build\EnginesTestSuite
ctest --test-dir build --output-on-failure
```

Suites: `LogicBoardTestSuite` (2478L: postings, FY, bill series, trial zero-check,
GSTR byte-determinism), `EnginesTestSuite` (643L: mandi/aank/GST/milling/TDS,
migrators). Known gaps they do NOT cover (Docs 03/05): keyboard flow, FY-lock
enforcement, concurrency, audit — add `tests/test_keyboard_flow.cpp` next.

## What runs TODAY without Qt (VM can do now)

- Python analyzers: `bahi_khata_analyzer.py` (needs `pip install pypdf`),
  `bahi_khata_engine` audit CLI (needs Java + UCanAccess jars — Mac-only for now).
- Byte forensics: Jet magic check (done: both `.bds` = Standard Jet DB),
  SQLite header check (done: 31 MB migrated DB intact).
- Schema lint without compile: grep gates for `CREATE TABLE` outside
  `database_manager.cpp`, `WHERE party_name` on `parties`, hardcoded literals
  (`06AAAAA|10822019|SAWAN SINGH|KRISHNA FOODS`) — runnable in PowerShell today.

## Division with Mac

- Mac: full app compile + visual UI pass + Java-bridge `Data.002` audit.
- VM: headless `ctest`, Python checks, `.bds` migrator tests, Busy/Tally behavior
  reference. Do NOT block Phase A fixes on MSVC/Qt completion — grep-gates +
  Python checks run now.
