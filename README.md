# BUPT EasyX 图形化贪吃蛇

这是基于 `main_improved.c` 的 OJ 版贪吃蛇重构出的 Visual Studio + EasyX 图形化工程。项目支持单人模式、AI 对战模式、限时挑战模式、更换时装、设置界面、背景音乐、事件音效、多地图尺寸和可扩展道具系统。

## 运行方式

1. 安装 Visual Studio 2022，并勾选“使用 C++ 的桌面开发”。
2. 安装 EasyX，确认 `graphics.h` 和 EasyX 库能被 Visual Studio 找到。
3. 打开 `BUPT_Eating_Snake_Graphics.sln`。
4. 选择 `Debug | x64` 或 `Release | x64`，生成并运行。

工程构建后会把 `assets` 目录复制到输出目录。若运行时某张贴图缺失，程序会使用备用色块绘制。

## 当前功能

- 欢迎界面：单人模式、AI 对战模式、限时挑战模式、更换时装、设置、退出游戏。
- 设置界面：N 步自动增长开关，20x20 / 50x50 / 100x100 地图，三档窗口分辨率，音乐开关，音效开关。
- 键盘控制：W/A/S/D 控制方向，E 在 AI 对战中射箭，1 加速，2 减速，P 暂停，R 重新开始。
- 开局等待：进入游戏后先加载地图，检测到 W/A/S/D 后才正式开始移动。
- 大地图适配：障碍物、食物和道具数量会随地图尺寸扩展，渲染按当前地图和分辨率动态计算格子像素。
- AI 对战新道具：弓箭、护盾、尖刺陷阱、减速时钟。
- 音频：背景音乐循环播放，开始、吃食物、道具、陷阱、射箭命中、碰撞均有独立音效。

## 文件结构

- `include/common.h`：公共常量、枚举、结构体。
- `src/game.c`：地图、移动、增长、碰撞、道具、胜负、计分和音效事件。
- `src/ai.c`：AI 寻路和低/中/高三档策略。
- `src/input.c`：Win32 `GetAsyncKeyState` 输入检测。
- `src/render_easyx.c`：EasyX 窗口、贴图加载、菜单和游戏画面绘制。
- `src/audio.c`：WinMM 背景音乐与事件音效。
- `src/ui.c`：菜单流程、设置流程、游戏循环和结算界面。
- `tools/generate_assets.py`：生成默认 PNG 贴图。
- `tools/generate_audio.py`：生成默认 WAV 音频。
- `tools/build_design_doc.py`：生成新版概要设计说明书。

## 扩展接口

- 新增道具：在 `include/common.h` 增加 `CellType`，在 `src/game.c` 增加生成和效果，在 `src/render_easyx.c` 增加贴图映射，必要时在 `src/audio.c` 增加音效。
- 新增皮肤：在 `assets/<皮肤名>/` 下放入同名 PNG，并在 `src/render_easyx.c` 的 `SKINS` 表增加一项。
- 新增音效：把 WAV 放入 `assets/audio/`，在 `include/common.h` 增加 `SoundEvent`，再修改 `src/audio.c` 的 `soundPath`。
- 调整 AI：主要修改 `src/ai.c` 的 `itemValueForAi`、`decideMedium`、`hardCandidateScore`。
- 调整设置项：在 `GameConfig` 增加字段，在 `Ui_runSettings` 修改值，在 `Render_drawSettings` 显示。

## 设计书

运行以下命令可重新生成设计书：

```powershell
python tools\build_design_doc.py
```

输出文件位于 `docs\图形化版贪吃蛇概要设计说明书_新版.docx`。
