# 鼠标+手柄操控 设计规格书

> 2026-06-12

---

## 一、操控方式枚举

```c
typedef enum ControlMethod {
    CONTROL_KEYBOARD_WASD = 0,   // 键盘 WASD + E
    CONTROL_KEYBOARD_ARROWS,     // 键盘 ↑↓←→ + /
    CONTROL_MOUSE,               // 鼠标指向 + 右键射箭
    CONTROL_GAMEPAD_1,           // 手柄 1 (XInput slot 0)
    CONTROL_GAMEPAD_2            // 手柄 2 (XInput slot 1)
} ControlMethod;
```

---

## 二、数据流

```
键盘 Input_readMenu(keys) ────┐
鼠标 Input_readMouse()  ──────┤
手柄 Input_readGamepad(slot) ──┤
                               ├──→ MenuInput ──→ ui.c / game loop
蛇方向 Input_readPlayerDir*() ─┘
```

所有输入源统一汇入 `MenuInput` 结构体（已有），游戏循环不需要区分来源。

---

## 三、鼠标操控

### 3.1 菜单模式
- 移动鼠标到按钮上方 → 自动切换 `selected`（hover 高亮）
- 左键点击 → `confirm`
- 右键点击 → `cancel`

### 3.2 游戏模式
- 持续读取鼠标屏幕坐标 → 换算为棋盘坐标
- 计算鼠标棋盘位置相对于蛇头位置的方位：
  - |dx| > |dy| 且 dx > 0 → DIR_RIGHT
  - |dx| > |dy| 且 dx < 0 → DIR_LEFT
  - |dy| > |dx| 且 dy < 0 → DIR_UP
  - |dy| > |dx| 且 dy > 0 → DIR_DOWN
- 右键按下 → fire

### 3.3 函数接口
```c
// input.h 新增
Direction Input_readMouseDirection(Pos snakeHead, int startRow, int startCol, int cellSize);
bool Input_mouseInRect(int left, int top, int right, int bottom);
bool Input_mouseClicked(void);
bool Input_mouseRightClicked(void);
void Input_updateMouse(void);
```

---

## 四、XInput 手柄操控

### 4.1 手柄读取
- 链接 `Xinput.lib`（VS 自带）
- 每次轮询 `XInputGetState(0, &state)` 和 `XInputGetState(1, &state)`
- 轮询频率：每帧一次（~16ms），XInput 轮询开销可忽略

### 4.2 映射表

| 手柄按键 | 菜单功能 | 游戏功能 |
|----------|----------|----------|
| 左摇杆上/下 | 菜单上下移动 | 蛇方向 上/下 |
| 左摇杆左/右 | 菜单左右切换 | 蛇方向 左/右 |
| A 键 | 确认 | 射箭 |
| B 键 | 返回 | — |
| Start | 确认（进入） | 暂停 |

### 4.3 摇杆死区
- 摇杆值范围 -32768 ~ 32767
- 死区阈值：±10000（约 30%），防止漂移误触

### 4.4 函数接口
```c
// input_gamepad.h 新增
typedef enum GamepadButton {
    GAMEPAD_A, GAMEPAD_B, GAMEPAD_X, GAMEPAD_Y,
    GAMEPAD_DPAD_UP, GAMEPAD_DPAD_DOWN, GAMEPAD_DPAD_LEFT, GAMEPAD_DPAD_RIGHT,
    GAMEPAD_START, GAMEPAD_BACK
} GamepadButton;

bool Input_gamepadConnected(int slot);
bool Input_gamepadButtonPressed(int slot, GamepadButton button);
Direction Input_gamepadDirection(int slot);
void Input_updateGamepad(int slot);
```

---

## 五、操控方式选择界面

### 5.1 触发时机
仅在 `MODE_LOCAL_MULTIPLAYER` 时，在 `Ui_chooseVariant` 之后显示。

### 5.2 界面布局
```
        本地多人 — 选择操控方式

   玩家一                        玩家二
  ┌──────────────┐           ┌──────────────┐
  │ 🔵 键盘 WASD  │  ◀ 当前  │ ⚪ 键盘 方向键 │
  │ ⚪ 键盘 方向键 │           │ ⚪ 鼠标        │
  │ ⚪ 鼠标        │           │ ⚪ 手柄 1      │
  │ ⚪ 手柄 1      │           │ ⚪ 手柄 2      │
  │ ⚪ 手柄 2      │           │                │
  └──────────────┘           └──────────────┘

   玩家一: W/S 切换  玩家二: ↑/↓ 切换  Enter 确认
```

### 5.3 限制规则
- 鼠标最多 1 人使用（后选的人鼠标选项灰掉）
- 手柄按 slot 区分（slot 0=手柄1, slot 1=手柄2）
- 未连接的手柄选项灰掉
- P1 和 P2 不能选同一种键盘方式

### 5.4 默认值
- P1 默认 `CONTROL_KEYBOARD_WASD`
- P2 默认 `CONTROL_KEYBOARD_ARROWS`

---

## 六、游戏循环改动

在 `runOneRound` 中，根据 `state->config.p1Control` 和 `state->config.p2Control` 选择对应读取函数：

```c
switch (p1Control) {
case CONTROL_MOUSE: dir = Input_readMouseDirection(...); break;
case CONTROL_GAMEPAD_1: dir = Input_gamepadDirection(0); break;
case CONTROL_GAMEPAD_2: dir = Input_gamepadDirection(1); break;
default: dir = Input_readPlayerDirection(); break;
}
```

---

## 七、GameConfig 新增字段

```c
ControlMethod p1ControlMethod;
ControlMethod p2ControlMethod;
```

仅在 `MODE_LOCAL_MULTIPLAYER` 时生效。

---

## 八、文件改动清单

| 文件 | 操作 | 内容 |
|------|------|------|
| `include/common.h` | 修改 | 新增 `ControlMethod` 枚举，`GameConfig` 新增 2 字段 |
| `include/input.h` | 修改 | 新增鼠标/手柄函数声明、MenuInput 新增 mouseX/mouseY |
| `src/input.c` | 修改 | 鼠标读取实现 |
| `include/input_gamepad.h` | 新建 | 手柄函数声明 |
| `src/input_gamepad.c` | 新建 | XInput 手柄实现 |
| `src/ui.c` | 修改 | 新增 `Ui_chooseControls` 界面、runOneRound 多源输入 |
| `src/render_easyx.c` | 修改 | 操控选择界面渲染 |
| `src/main.c` | 修改 | 多人模式流程加入操控选择 |
| `BUPT_Eating_Snake_Graphics.vcxproj` | 修改 | 链接 `Xinput.lib`、新增编译文件 |
