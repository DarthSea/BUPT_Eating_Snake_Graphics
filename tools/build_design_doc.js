const fs = require("fs");
const {
  Document, Packer, Paragraph, TextRun, HeadingLevel, AlignmentType,
  BorderStyle, PageBreak, LevelFormat, TabStopType, TabStopPosition, ShadingType,
} = require("docx");

const FONT = "Microsoft YaHei";
const FONT_MONO = "Consolas";
const BLUE = "2B6CB0";
const GRAY = "666666";
const DARK = "1A1A2E";

function h1(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_1, spacing: { before: 360, after: 200 },
    children: [new TextRun({ text, font: FONT, size: 36, bold: true, color: DARK })],
  });
}
function h2(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_2, spacing: { before: 280, after: 160 },
    children: [new TextRun({ text, font: FONT, size: 30, bold: true, color: BLUE })],
  });
}
function h3(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_3, spacing: { before: 200, after: 120 },
    children: [new TextRun({ text, font: FONT, size: 26, bold: true, color: DARK })],
  });
}
function para(text, opts = {}) {
  return new Paragraph({
    spacing: { after: 120, line: 360 },
    children: [new TextRun({ text, font: FONT, size: 22, color: opts.color || DARK })],
  });
}
function boldPara(label, text) {
  return new Paragraph({
    spacing: { after: 80, line: 360 },
    children: [
      new TextRun({ text: label, font: FONT, size: 22, bold: true, color: DARK }),
      new TextRun({ text, font: FONT, size: 22, color: DARK }),
    ],
  });
}
function codeBlock(code) {
  const lines = code.split("\n");
  return lines.map((line, i) => new Paragraph({
    spacing: { after: 0, line: 280 },
    indent: { left: 360 },
    children: [new TextRun({ text: line || " ", font: FONT_MONO, size: 18, color: "333333" })],
  }));
}
function listItem(text, level = 0) {
  return new Paragraph({
    numbering: { reference: "bullets", level },
    spacing: { after: 60, line: 320 },
    children: [new TextRun({ text, font: FONT, size: 22, color: DARK })],
  });
}
function numItem(text, level = 0) {
  return new Paragraph({
    numbering: { reference: "numbers", level },
    spacing: { after: 60, line: 320 },
    children: [new TextRun({ text, font: FONT, size: 22, color: DARK })],
  });
}
function empty() {
  return new Paragraph({ spacing: { after: 80 }, children: [] });
}

const doc = new Document({
  styles: {
    default: { document: { run: { font: FONT, size: 22 } } },
    paragraphStyles: [
      { id: "Heading1", name: "Heading 1", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 36, bold: true, font: FONT, color: DARK },
        paragraph: { spacing: { before: 360, after: 200 }, outlineLevel: 0 } },
      { id: "Heading2", name: "Heading 2", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 30, bold: true, font: FONT, color: BLUE },
        paragraph: { spacing: { before: 280, after: 160 }, outlineLevel: 1 } },
      { id: "Heading3", name: "Heading 3", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 26, bold: true, font: FONT, color: DARK },
        paragraph: { spacing: { before: 200, after: 120 }, outlineLevel: 2 } },
    ],
  },
  numbering: {
    config: [
      { reference: "bullets",
        levels: [
          { level: 0, format: LevelFormat.BULLET, text: "•", alignment: AlignmentType.LEFT,
            style: { paragraph: { indent: { left: 720, hanging: 360 } } } },
          { level: 1, format: LevelFormat.BULLET, text: "◦", alignment: AlignmentType.LEFT,
            style: { paragraph: { indent: { left: 1080, hanging: 360 } } } },
        ] },
      { reference: "numbers",
        levels: [
          { level: 0, format: LevelFormat.DECIMAL, text: "%1.", alignment: AlignmentType.LEFT,
            style: { paragraph: { indent: { left: 720, hanging: 360 } } } },
          { level: 1, format: LevelFormat.DECIMAL, text: "%1.%2", alignment: AlignmentType.LEFT,
            style: { paragraph: { indent: { left: 1080, hanging: 360 } } } },
        ] },
    ],
  },
  sections: [{
    properties: {
      page: {
        size: { width: 11906, height: 16838 },
        margin: { top: 1440, right: 1440, bottom: 1440, left: 1440 },
      },
    },
    children: [
      // ==================== 封面 ====================
      empty(), empty(), empty(), empty(), empty(), empty(), empty(),
      new Paragraph({
        alignment: AlignmentType.CENTER, spacing: { after: 200 },
        children: [new TextRun({ text: "图形化贪吃蛇", font: FONT, size: 56, bold: true, color: DARK })],
      }),
      new Paragraph({
        alignment: AlignmentType.CENTER, spacing: { after: 120 },
        children: [new TextRun({ text: "概要设计说明书", font: FONT, size: 40, bold: true, color: BLUE })],
      }),
      new Paragraph({
        alignment: AlignmentType.CENTER, spacing: { after: 240 },
        children: [new TextRun({ text: "基于 OJ 版字符贪吃蛇重构的 Visual Studio + EasyX 图形化工程", font: FONT, size: 22, color: GRAY })],
      }),
      empty(), empty(), empty(),
      para("技术栈：C 语言 + EasyX 图形库 + Win32 API + XInput", { color: GRAY }),
      para("编译环境：Visual Studio 2022 (MSVC) + MSBuild", { color: GRAY }),
      para("运行平台：Windows 10/11 x64", { color: GRAY }),
      empty(), empty(), empty(), empty(), empty(),
      new Paragraph({
        alignment: AlignmentType.CENTER,
        children: [new TextRun({ text: "黄海彬  卢麒云  刘景炫", font: FONT, size: 26, color: DARK })],
      }),
      new Paragraph({ children: [new PageBreak()] }),

      // ==================== 第一章 项目概述 ====================
      h1("第一章 项目概述"),
      h2("1.1 项目背景"),
      para("本项目源自北京邮电大学计算机导论课程大作业。原始题目要求使用标准输入输出实现一个 20×20 字符贪吃蛇游戏（OJ 版），核心逻辑包含 W/A/S/D 方向控制、吃食物得分、N 步增长、撞墙/障碍物/自身死亡判定。"),
      para("图形化版在 OJ 版基础上进行了全面升级：保留核心游戏逻辑和数据结构，将字符界面替换为 EasyX 图形库的贴图渲染，将 AI 自动寻路替换为玩家键盘/鼠标/手柄实时操控，将标准输入输出交互替换为菜单驱动的图形界面。代码采用模块化分层架构，游戏内核、渲染、输入、音频、AI 决策各自独立为单独模块，通过公共接口通信。"),

      h2("1.2 功能清单"),
      para("以下按编号列出图形化版的全部功能，每个功能标注难度等级（基础/进阶）："),
      numItem("单人模式（基础）：经典贪吃蛇玩法，W/A/S/D 控制方向，吃食物得分，撞墙/障碍/自身死亡。"),
      numItem("AI 对战模式（进阶）：与 AI 控制的蛇同场竞技，支持低/中/高三档难度。"),
      numItem("限时挑战模式（进阶）：90 秒内挑战最高分，多样道具刷新。"),
      numItem("本地多人模式（进阶）：双人同屏对抗，P1 使用 WASD，P2 使用方向键，90 秒限时，死亡即判负或时间到比分数。"),
      numItem("多样道具系统（进阶）：加分食物、加速食物、减速食物、护盾、陷阱等 11 种道具类型，AI 对战专属道具池（弓箭/战斗护盾/尖刺陷阱/减速时钟）。"),
      numItem("更换地图样式（基础）：三套皮肤——像素森林（草丛纹理）、霓虹夜行（金属纹理+霓虹灯效）、冰原挑战（冰块纹理）。"),
      numItem("设置界面（基础）：N 步自动增长开关、分辨率选择（1280×720~2560×1440）、全屏模式、背景音乐/音效开关。"),
      numItem("多地图尺寸（进阶）：20×20、50×50、100×100 三种尺寸，开局前自选。"),
      numItem("随机事件系统（进阶）：多样模式专属——地图轰炸（红色预警区+定时爆破）和刀光箭影（边界墙格射箭），10 秒间隔随机触发。"),
      numItem("多输入支持（进阶）：键盘（WASD/方向键）+ 鼠标（光标方位+左键射箭）+ XInput 手柄（双控制器+摇杆方向+RB 射箭）。"),
      numItem("音频系统（基础）：背景音乐循环播放，开始/吃食物/道具/陷阱/射箭命中/碰撞共 11 种独立音效。"),
      numItem("粒子特效与动画（进阶）：200 粒子池，吃食物爆炸/死亡碎片/轰炸火花触发，分数弹跳动效，文字发光/描边效果。"),

      h2("1.3 运行环境"),
      listItem("操作系统：Windows 10 或 Windows 11（64 位）"),
      listItem("开发工具：Visual Studio 2022，需勾选 使用 C++ 的桌面开发 工作负荷"),
      listItem("图形库：EasyX（需安装到 VS 的 include 和 lib 目录）"),
      listItem("编译方式：MSBuild Debug x64，命令行一键构建"),
      listItem("手柄支持：XInput 兼容手柄（Xbox 协议），最多同时连接两个"),
      new Paragraph({ children: [new PageBreak()] }),

      // ==================== 第二章 系统架构 ====================
      h1("第二章 系统架构"),
      h2("2.1 分层结构"),
      para("系统采用六层分层架构，上层依赖下层，下层不感知上层。各层职责如下："),
      ...codeBlock(
`┌─────────────────────────────────────────┐
│          main.c  程序入口 + 崩溃捕获       │  ← 最上层
├─────────────────────────────────────────┤
│          ui.c  UI 流程层                  │  ← 菜单/设置/游戏循环
├─────────────────────────────────────────┤
│  input.c     render_easyx.c    audio.c   │  ← 输入/渲染/音频层
│  +gamepad.c                             │
├─────────────────────────────────────────┤
│          game.c  游戏逻辑层               │  ← 地图/移动/碰撞/道具/事件
├─────────────────────────────────────────┤
│          ai.c   AI 决策层                │  ← 寻路/道具评估/攻击策略
├─────────────────────────────────────────┤
│          common.h  公共定义层             │  ← 枚举/结构体/常量/工具函数
└─────────────────────────────────────────┘`),
      empty(),

      h2("2.2 文件组织结构"),
      ...codeBlock(
`BUPT_Eating_Snake_Graphics/
├── include/                    # 头文件（8 个）
│   ├── common.h                # 公共常量、枚举、结构体
│   ├── game.h                  # 游戏逻辑对外接口
│   ├── ai.h                    # AI 决策接口
│   ├── render.h                # 渲染接口
│   ├── input.h                 # 输入接口（键盘+鼠标）
│   ├── input_gamepad.h         # 手柄输入接口
│   ├── audio.h                 # 音频接口
│   └── ui.h                    # UI 流程接口
├── src/                        # 源文件（9 个）
│   ├── main.c                  # 程序入口 + 崩溃捕获
│   ├── common.c                # 工具函数实现
│   ├── game.c                  # 游戏逻辑（最大模块，~1800 行）
│   ├── ai.c                    # AI 寻路与决策
│   ├── render_easyx.c          # EasyX 渲染 + UI + 粒子 + 动画
│   ├── input.c                 # 键盘 + 鼠标输入
│   ├── input_gamepad.c         # XInput 手柄输入
│   ├── audio.c                 # WinMM 音频播放
│   └── ui.c                    # 菜单流程 + 游戏循环
├── assets/                     # 贴图 + 音效资源
│   ├── default/                # 皮肤：像素森林（17 张 .bmp）
│   ├── neon/                   # 皮肤：霓虹夜行
│   ├── ice/                    # 皮肤：冰原挑战
│   └── audio/                  # 音效文件（11 个 .wav）
└── tools/                      # 辅助脚本
    ├── generate_assets.py      # 贴图自动生成
    └── build_design_doc.js     # 文档生成`),
      empty(),

      h2("2.3 模块依赖关系"),
      para("所有模块均依赖 common.h 中定义的枚举、结构体和常量。game.c 是核心模块，提供 Game_init、Game_update 等 30 多个公开函数。render_easyx.c 读取 GameState 渲染画面，input.c 将用户输入写入 MenuInput 结构，ui.c 协调渲染和输入完成菜单流程和游戏循环。audio.c 独立运行，仅通过 Audio_playEvent 被调用。"),
      new Paragraph({ children: [new PageBreak()] }),

      // ==================== 第三章 数据结构 ====================
      h1("第三章 数据结构设计"),
      h2("3.1 核心枚举"),

      h3("方向枚举 Direction"),
      para("定义四个基本方向和一个空方向。所有移动、转向、射箭操作均使用此枚举。"),
      ...codeBlock(`typedef enum Direction {
    DIR_UP = 0, DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_NONE
} Direction;`),

      h3("游戏模式 GameMode"),
      para("区分四种游戏模式。GameState.config.mode 在初始化时确定，后续不可更改。不同模式在 Game_applyModeDefaults 中设置不同的默认参数（移动速度、限时、道具池）。"),
      ...codeBlock(`typedef enum GameMode {
    MODE_SINGLE = 0, MODE_AI_BATTLE, MODE_TIME_CHALLENGE, MODE_LOCAL_MULTIPLAYER
} GameMode;`),

      h3("格子类型 CellType"),
      para("地图上每个格子的状态。蛇的身体不存储在 cells 数组中（仅存于 Snake.body[]），避免蛇体与道具的数据冲突。CELL_EMPTY 表示空地，CELL_WALL 表示不可穿越的墙壁，其余为各类食物、道具和障碍物。"),
      ...codeBlock(`typedef enum CellType {
    CELL_EMPTY = 0, CELL_WALL, CELL_OBSTACLE,
    CELL_FOOD, CELL_FOOD_BONUS, CELL_FOOD_SPEED, CELL_FOOD_SLOW,
    CELL_TRAP, CELL_SHIELD,
    CELL_BATTLE_BOW, CELL_BATTLE_SHIELD, CELL_BATTLE_SPIKE, CELL_BATTLE_CLOCK
} CellType;`),

      h2("3.2 Snake 蛇结构体"),
      para("蛇的所有状态集中存储在此结构体中。设计要点："),
      listItem("蛇身体使用 Pos body[] 数组而非链表。最大长度为 MAX_SNAKE_LEN = 地图面积，可预先分配。数组支持 O(1) 随机访问，移动时仅需从尾部向前整体后移一个位置，无需动态内存分配。"),
      listItem("dir 表示当前移动方向，nextDir 表示下一帧将改变为的方向。这种双缓冲机制允许玩家在本步完成前预输入下一次转向，同时防止反方向输入导致误死。"),
      listItem("限时效果（shieldMs / speedMs / slowMs）使用毫秒计时器，每帧由 reduceEffectTimers 递减，到期自动清除。"),
      listItem("bowArrows 和 shieldCharges 为 AI 对战和本地多人模式专属资源，分别表示持有的弓箭数量和护盾充能次数。"),
      ...codeBlock(
`typedef struct Snake {
    Pos body[MAX_SNAKE_LEN];    // 身体坐标数组，body[0] 为头部
    int length;                 // 当前长度（最少 3）
    Direction dir;              // 当前移动方向
    Direction nextDir;          // 下一帧方向（缓冲反方向保护）
    int score;                  // 累计得分
    int stepCount;              // N 步增长计数器
    bool alive;                 // 存活状态
    int shieldMs;               // 护盾剩余时间（毫秒）
    int speedMs;                // 加速剩余时间（毫秒）
    int slowMs;                 // 减速剩余时间（毫秒）
    int dirChangeCooldownMs;    // 方向变更冷却（减速时生效）
    int slowStepCounter;        // 减速时的步数计数器
    int bowArrows;              // 弓箭持有数量
    int shieldCharges;          // 战斗护盾充能次数
} Snake;`),
      empty(),

      h2("3.3 GameState 游戏状态"),
      para("全局唯一的游戏状态结构体，贯穿整个游戏生命周期。设计要点："),
      listItem("cells[][] 数组存储地图上的地形和道具，蛇身体不在其中。这种分离设计使得道具放置、蛇移动、碰撞检测三者之间不会发生数据冲突。"),
      listItem("player 和 ai 两条蛇始终存在，但仅在对应模式下 alive 为 true。本地多人模式的 P2 复用 ai 蛇槽位，通过 p1ControlMethod / p2ControlMethod 区分输入来源。"),
      listItem("arrows 数组管理弓箭飞行物（最大 64 个），event 字段管理随机事件状态，particles 数组管理粒子特效（最大 200 个）。soundEvents 是生产者-消费者模式的音效队列：游戏逻辑 push 事件，UI 层 pull 并调用 Audio_playEvent 播放。"),
      ...codeBlock(
`typedef struct GameState {
    CellType cells[MAX_MAP_SIZE][MAX_MAP_SIZE];
    Snake player;                                // 玩家蛇（单人 / P1）
    Snake ai;                                    // AI 蛇 / 本地多人 P2
    ArrowProjectile arrows[MAX_ACTIVE_ARROWS];   // 弓箭飞行物池
    GameConfig config;                           // 本局配置快照
    GameResult result;                           // 当前结果
    int speedLevel;                              // 速度档 -2..2
    int moveTimerMs;                             // 步进计时器
    int elapsedMs;                               // 已用时间
    int remainingSeconds;                        // 剩余秒数
    unsigned int randomSeed;                     // 随机种子
    SoundEvent soundEvents[MAX_SOUND_EVENTS];    // 待播放音效队列
    int soundEventCount;
    char statusText[128];                        // 状态文字
    RandomEventState event;                      // 随机事件状态
    Particle particles[MAX_PARTICLES];           // 粒子池
} GameState;`),
      empty(),

      h2("3.4 其他关键结构"),
      boldPara("ArrowProjectile：", "弓箭飞行物。active 标记是否活跃，pos 存储当前棋盘坐标，dir 存储飞行方向，ownerIndex 标识发射者（-1=无主箭矢/箭雨），moveTimerMs 控制每 35ms 移动一格。"),
      boldPara("RandomEventState：", "随机事件状态机。activeEvent 标记当前事件类型，eventTimerMs 倒计时事件剩余时间（12 秒），phaseTimerMs 控制子阶段（3 秒间隔）。bombWarning 和 bombActive 驱动预警-爆破的视觉状态切换。zones[3] 存储三个矩形轰炸区的行列范围，borderSources 存储箭雨边界发射源的坐标和方向。"),
      boldPara("Particle：", "粒子特效单元。使用浮点坐标 (x,y) 和速度 (vx,vy) 实现亚像素运动，lifeMs 控制生命周期（600-1200ms），最后 500ms 半径逐渐缩小至 1px 模拟消散效果。"),
      boldPara("GameConfig：", "游戏配置。mode/variant/mapSize 等字段在 Game_init 时快照到 GameState 中，后续不可更改，保证游戏过程中的一致性。p1ControlMethod 和 p2ControlMethod 仅在本地多人模式下生效。"),
      new Paragraph({ children: [new PageBreak()] }),

      // ==================== 第四章 模块详细设计 ====================
      h1("第四章 模块详细设计"),

      // 4.1 game.c
      h2("4.1 游戏核心模块（game.c，~1800 行）"),
      boldPara("职责：", "地图生成、蛇移动、碰撞检测、道具效果、计分、胜负判定、随机事件调度、弓箭飞行物管理。这是整个项目最大的模块。"),
      empty(),
      boldPara("实现原理——StepPlan 决策-执行分离模式：", ""),
      para("每次蛇移动前，先调用 buildStepPlan 构建一个 StepPlan 结构体，将本步所有可能的结果预先计算出来（下一个位置、碰到什么道具、是否死亡、是否增长）。碰撞检测在 plan 上交叉验证后再由 applySnakeMove 和 applyItemEffect 分别执行移动和道具效果。这种分离设计使得双蛇碰撞检测可以在两个 plan 之间相互检查而不需要回滚。"),
      boldPara("实现原理——道具维护策略：", ""),
      para("maintainItems 函数在每步移动后运行，确保地图上的各类道具始终保持在目标数量。根据游戏模式调用 maintainNormalItems（常规/多样道具）或 maintainBattleItems（AI 对战道具）。道具放置分两阶段：第一阶段随机尝试最多 mapSize²×4 次，第二阶段扫描全图从随机偏移开始线性寻找空位（兜底，防止大地图随机失败导致卡死）。"),
      boldPara("实现原理——随机事件调度：", ""),
      para("updateRandomEvents 在 Game_update 中每帧被调用。当 sinceLastEventMs 达到间隔阈值（正常 10 秒，本地多人最后 30 秒加速为 5 秒，限时挑战缩短 25%）时，随机选择轰炸或箭雨事件。事件持续 12 秒，子阶段通过 phaseTimerMs 控制节奏（轰炸每 3 秒一次、箭雨每 2 秒一次）。bombWarning 在爆破前 1 秒激活，驱动橙色预警渲染和 AI 逃避行为。"),
      boldPara("实现原理——弓箭飞行系统：", ""),
      para("ArrowProjectile 以 35ms 间隔逐格飞行（比蛇的 110-140ms 快 3-4 倍）。updateArrows 每帧遍历活跃箭矢，移动后调用 applyArrowHit 检查：碰到墙壁/障碍物则消失；碰到敌方蛇则命中。命中头部（hitIndex==0）直接死亡，命中身体则截断蛇身，发射者获得 20 分奖励。无主箭矢（ownerIndex=-1，来自箭雨事件）对任何碰到的蛇造成同等伤害。"),
      empty(),
      boldPara("关键函数：", ""),
      listItem("Game_init(state, config)：初始化地图、放置蛇、生成障碍物和道具、设置随机种子"),
      listItem("Game_update(state, deltaMs)：主更新函数——计时器推进→随机事件更新→步进循环→道具维护"),
      listItem("buildStepPlan(state, snake)：构建单步计划，计算 next 位置、碰撞判定、增长标记"),
      listItem("addBattleCollisions(state, p1Plan, p2Plan)：双蛇碰撞交叉检查，处理头碰头和错身而过"),
      listItem("applyItemEffect(state, snake, opponent, plan)：根据吃到道具类型施加效果（加分/加速/护盾/弓箭/减速/尖刺即死）"),
      listItem("maintainItems(state)：维护道具数量，不足时在空位生成"),
      listItem("updateRandomEvents(state, deltaMs)：事件状态机——间隔计时、事件触发、子阶段管理、十二秒后结束"),
      listItem("fireArrow(state, shooterIndex)：从蛇头前方一格发射弓箭，复用箭头数组"),
      listItem("spawnBorderArrow(state, from, dir)：从边界墙格向场内方向生成无主箭矢"),
      listItem("finishIfNeeded(state)：根据蛇存活状态和模式判定游戏结果（单人死/P1胜/P2胜/时间到比分数/平局）"),

      // 4.2 ai.c
      h2("4.2 AI 决策模块（ai.c，~660 行）"),
      boldPara("职责：", "为 AI 对战模式提供每步的方向决策，支持低/中/高三档难度，实时感知随机事件并规避危险。"),
      empty(),
      boldPara("实现原理——核心寻路算法：", ""),
      para("AI 使用 BFS 广度优先搜索作为基础寻路引擎。三个独立的 BFS 函数服务于不同目的：bfsFirstStepTo 返回从起点到目标路径上的第一步方向（用于路径追踪），bfsDistance 计算两点间最短路径长度（用于距离评分），floodAreaAfterMove 模拟移动后的可达区域面积（用于生存空间评估）。BFS 在初始化 visited 矩阵时，将墙壁、障碍物、无护盾时的陷阱/尖刺、以及轰炸事件全时段标记的轰炸区都设为已访问，从而天然实现障碍规避。"),
      boldPara("实现原理——三级难度评分：", ""),
      para("低难度（decideEasy）使用贪心策略，选择曼哈顿距离最近的食物的方向；若无可行方向则回退到最大空间方向。中难度（decideMedium）先用 bfsFirstStepTo 寻最佳道具，再用 floodAreaAfterMove 验证移动后生存空间是否充足。高难度（decideHard）对四个候选方向逐一调用 hardCandidateScore 计算综合评分，评分公式包含：可达面积得分（×3）、道具价值（×85，弓箭>护盾>时钟）、食物距离惩罚（×18）、玩家距离加成、压缩玩家合法移动数加成。高难度额外检测射击线（Game_hasClearShot）和轰炸区惩罚（isBombZoneActive 时扣 600 分，bombWarning 时扣 1500 分，bombActive 致命时扣 AI_BAD_SCORE/4）。"),
      empty(),
      boldPara("关键函数：", ""),
      listItem("Ai_decideDirection(state, difficulty)：入口函数，按难度分发到对应决策函数"),
      listItem("bfsFirstStepTo(state, start, target)：BFS 返回从起点到目标路径上的第一步方向"),
      listItem("floodAreaAfterMove(state, start, grow)：模拟移动后 BFS 计算可达空格总数"),
      listItem("hardCandidateScore(state, dir)：综合评分函数，评估面积/食物距离/玩家移动数/道具价值/轰炸区风险/箭雨路径"),
      listItem("isSafeCandidate(state, dir)：检查该方向是否安全——不撞墙、不撞自身/玩家、不进入无护盾时的轰炸区"),
      listItem("findBestItemByDistance(state, from, target)：全图扫描找到综合价值最高的道具目标"),
      listItem("itemValueForAi(cell)：为不同道具评估价值权重——弓箭(35)>战斗护盾(28)>时钟(24)>普通护盾(16)"),

      // 4.3 render_easyx.c
      h2("4.3 渲染模块（render_easyx.c，~2100 行）"),
      boldPara("职责：", "EasyX 窗口管理、贴图加载与切换、全部画面的绘制（菜单/设置/游戏棋盘/HUD/结算）、粒子特效系统、动画过渡。"),
      empty(),
      boldPara("实现原理——EasyX 双缓冲管线：", ""),
      para("所有渲染函数遵循统一模式：BeginBatchDraw() 开启双缓冲→绘制所有元素到后台缓冲区→FlushBatchDraw() 一次性显示到前台。这避免了逐元素绘制的闪烁问题。cleardevice() 在每帧开始时清空后台缓冲区。"),
      boldPara("实现原理——贴图加载与皮肤系统：", ""),
      para("SKINS[] 数组定义三套皮肤的名称和文件夹路径。Render_loadSkin 遍历 TEX_COUNT 个贴图槽位，拼接路径为 assets/<folder>/<texture_name>.bmp，使用 EasyX loadimage 加载并缩放到 textureCellSize 尺寸。若文件不存在则使用 FALLBACK_COLORS 数组中的纯色作为备选渲染。"),
      boldPara("实现原理——大地图视口裁剪：", ""),
      para("20×20 地图完整显示；50×50 和 100×100 地图使用局部视口——以玩家蛇头的棋盘坐标为焦点，通过 visibleCellsForMap 计算可见格子数，clampViewportStart 将视口左上角约束在地图范围内，渲染循环仅遍历 startRow..startRow+visibleCells 范围内的格子。视口外的元素（蛇身段、道具、弓箭）通过 isInView 判断后跳过绘制。ground.bmp 贴图使用 putimage 在 visibleCells 范围内循环平铺。"),
      boldPara("实现原理——粒子系统：", ""),
      para("Particle 结构体使用浮点坐标实现亚像素运动。200 个粒子组成全局池（gParticles 数组），init 时全部标记为非活跃。触发时从池中寻找非活跃槽位并填充位置/速度/颜色/生命值。每帧 Render_particlesUpdate 更新所有活跃粒子的位置和生命值，最后 500ms 半径线性缩小至 1px。Render_particlesDraw 遍历所有活跃粒子并用 solidcircle 绘制。性能监控 gUseParticles 在帧均耗时超过 40ms 时自动设为 false 禁用粒子。"),
      boldPara("实现原理——分辨率与全屏自适应：", ""),
      para("menuScale 函数以窗口宽度 1280px 为基准计算缩放因子（0.85~1.5 倍），主按钮高度 58×ms、字号 18×ms、间距 10×ms 全部乘以该因子。hudScale 以棋盘像素 640px 为基准计算，控制棋盘 HUD 文字大小。全屏模式通过 Win32 GetHWnd() 获取 EasyX 窗口句柄，SetWindowLongPtr 修改窗口样式为 WS_POPUP（无边框），SetWindowPos 铺满整个屏幕。"),
      empty(),
      boldPara("关键函数：", ""),
      listItem("Render_drawWelcome(selected)：欢迎菜单——渐变背景+标题+编号卡片按钮+小蛇装饰动画+粒子"),
      listItem("Render_drawGame(render, state, paused, waiting)：游戏主画面——棋盘平铺+视口渲染+蛇+道具+弓箭+轰炸区+箭雨闪烁+HUD+卡片侧栏+死亡闪烁+粒子"),
      listItem("Render_drawGameOver(state, selectedAction)：结算画面——结果标题+双方分数+重新开始/返回菜单按钮"),
      listItem("Render_loadSkin(render, skinId)：加载指定皮肤的全部贴图文件，若文件缺失则使用备选色"),
      listItem("Render_particlesUpdate(deltaMs) / Render_particlesDraw() / Render_spawnParticles()：粒子系统三函数"),
      listItem("drawRoundedRect / drawCardWithShadow / drawMenuBgSnakes / drawTextGlow：组件级渲染辅助函数"),

      // 4.4 输入模块
      h2("4.4 输入模块（input.c + input_gamepad.c，~240 行）"),
      boldPara("职责：", "将键盘、鼠标、XInput 手柄的物理输入统一转化为 MenuInput 结构体和 Direction 枚举，供 UI 流程和游戏循环使用。"),
      empty(),
      boldPara("实现原理——键盘检测：", ""),
      para("使用 Win32 GetAsyncKeyState(vk) & 0x8000 检测按键电平（isKeyDown），用于方向控制——允许长按连续移动。使用按键状态前后帧比较检测边沿（keyPressed），用于菜单操作——确保每次按键只触发一次。MenuInput 结构体封装了所有操作：move/left/right（导航）、confirm/cancel（确认/取消）、fire（射箭）、speedUp/speedDown（调速）、pause（暂停）、p2Fire（P2 射箭）。"),
      boldPara("实现原理——鼠标检测：", ""),
      para("EasyX GetMouseMsg() 在 Input_updateMouse 中消费鼠标消息队列，记录当前坐标（gMouseX/Y）、左键点击、右键点击状态。Input_readMouseDirection 计算鼠标屏幕坐标相对于蛇头屏幕位置（BOARD_LEFT + 视口偏移 + 格子坐标）的方位角和距离，超过 cellSize/2 阈值后映射为四方向。Input_mouseInRect 用于菜单 hover 检测，仅在鼠标实际移动时更新选中项（通过比较前后帧坐标），避免干扰键盘导航。"),
      boldPara("实现原理——XInput 手柄检测：", ""),
      para("XInputGetState 轮询两个手柄槽位，读取 Gamepad.sThumbLX/LY（左摇杆坐标，范围 -32768~32767）。死区阈值设为 ±10000（约 30%），防止漂移误触。方向判定：比较 |lx| 和 |ly|，较大轴决定主方向。按键状态通过前帧后帧比较实现边沿检测。本地多人时两个手柄分别分配给 P1 和 P2。"),
      empty(),
      boldPara("关键函数：", ""),
      listItem("Input_readMenu(input, out)：综合键盘/鼠标/手柄输入，填充 MenuInput 结构体"),
      listItem("Input_readPlayerDirection()：WASD 方向读取（电平检测）"),
      listItem("Input_readPlayer2Direction()：方向键方向读取"),
      listItem("Input_readMouseDirection(snakeHead, startRow, startCol, cellSize)：鼠标方位→方向"),
      listItem("Input_updateMouse()：消费 EasyX 鼠标消息队列，更新全局鼠标状态"),
      listItem("Input_gamepadConnected(slot)：检测手柄是否连接"),
      listItem("Input_gamepadDirection(slot)：左摇杆值→四方向（带死区）"),
      listItem("Input_gamepadButtonPressed(slot, buttonMask)：边沿检测手柄按键"),

      // 4.5 音频模块
      h2("4.5 音频模块（audio.c，~110 行）"),
      boldPara("职责：", "播放在菜单和游戏过程中触发的背景音乐和事件音效，支持独立开关。"),
      empty(),
      boldPara("实现原理：", ""),
      para("背景音乐使用 WinMM mciSendString API 实现。初始化时发送 open + type waveaudio 命令打开 bgm.wav 文件并分配别名 snake_bgm，随后发送 play snake_bgm repeat 命令启动循环播放。关闭时发送 close snake_bgm 释放资源。事件音效使用 PlaySound API 异步播放（SND_ASYNC 标志），每个 SoundEvent 枚举值对应一个 .wav 文件路径。游戏中 Game_pushSoundEvent 将音效事件压入队列，UI 层每帧消费队列并调用 Audio_playEvent 播放。musicEnabled 和 soundEnabled 两个布尔开关分别控制 BGM 和音效的启停。"),
      empty(),
      boldPara("关键函数：", ""),
      listItem("Audio_init(music, sound)：初始化音频系统，按开关状态决定是否立即播放 BGM"),
      listItem("Audio_shutdown()：停止 BGM 并释放 MCI 资源"),
      listItem("Audio_playEvent(event)：异步播放事件音效（PlaySound + SND_ASYNC）"),
      listItem("Audio_setMusicEnabled / Audio_setSoundEnabled：开关控制函数"),

      // 4.6 ui.c
      h2("4.6 UI 流程模块（ui.c，~780 行）"),
      boldPara("职责：", "管理从程序启动到退出的完整菜单流程，协调渲染和输入实现每局游戏的主循环。"),
      empty(),
      boldPara("实现原理——菜单循环模式：", ""),
      para("每个菜单页面（欢迎/地图大小/玩法规则/操控方式/难度/时装/设置/结算）都遵循统一的循环模式：调用渲染函数绘制页面→调用 Input_readMenu 读取用户输入→根据 move/confirm/cancel 字段更新状态→Sleep(16ms) 控制帧率→循环。所有菜单页面统一在循环开始处调用 Input_updateMouse + Input_updateGamepads 以支持多设备输入。"),
      boldPara("实现原理——游戏循环 runOneRound：", ""),
      para("每帧流程：读取当前时间计算 deltaMs→读取 P1/P2 方向（根据 config.p1ControlMethod / p2ControlMethod 选择键盘/鼠标/手柄来源）→等待开始判定（按所选操控方式触发：键盘方向/鼠标左键/手柄 RB）→读取菜单操作（暂停/射箭/调速/取消）→处理射箭（P1:E+左键+RB，P2:/+左键+RB）→处理调速（1/2，多人模式禁用）→如果未暂停且已开始则调用 Game_update(deltaMs)→消费音效队列并播放→调用 Render_drawGame 绘制画面→更新粒子系统→Sleep(10ms) 控制帧率。"),
      empty(),
      boldPara("关键函数：", ""),
      listItem("Ui_runWelcome(input, render)：欢迎菜单主循环，返回选中的 MenuAction"),
      listItem("Ui_chooseMapSize(input, render, mapSize, isMulti)：地图大小选择（20/50/100），多人仅 20/50"),
      listItem("Ui_chooseControls(input, render, config)：本地多人操控方式分配（P1/P2 各选一种）"),
      listItem("Ui_chooseControlsSingle(input, render, config)：单人/限时操控方式选择"),
      listItem("Ui_chooseVariant / Ui_chooseDifficulty / Ui_chooseSkin / Ui_runSettings：其他选择界面"),
      listItem("Ui_runGame(input, render, state)：单局游戏主循环——runOneRound 循环→结算→重新开始/返回菜单"),
      new Paragraph({ children: [new PageBreak()] }),

      // ==================== 第五章 核心算法 ====================
      h1("第五章 核心算法"),

      h2("5.1 蛇移动 StepPlan 模式"),
      para("蛇的每次移动被抽象为 StepPlan 结构体，将'决策'和'执行'两个阶段分离："),
      ...codeBlock(
`typedef struct StepPlan {
    Pos next;         // 下一步头部坐标
    Direction dir;    // 移动方向（来自 snake->nextDir）
    CellType item;    // 目标格的物品类型
    bool dead;        // 本步是否导致死亡
    bool stays;       // 是否原地不动（减速时）
    bool grow;        // 是否增长（吃了食物或到达 N 步）
    bool shieldUsed;  // 是否消耗了护盾
} StepPlan;`),
      para("流程：buildStepPlan 根据 nextDir 计算 next 坐标→检验合法性（出界/碰墙/碰障碍/踩陷阱/踩尖刺）→检验轰炸区（若 bombActive 且在区域内，有护盾则标记 shieldUsed，无护盾则标记 dead）→判断是否增长（吃到食物或 stepCount 达到 growthInterval 值）→碰撞检测（addSingleSnakeCollisions 或 addBattleCollisions）→applySnakeMove 执行移动→applyItemEffect 执行道具效果。"),
      para("两种特殊的 StepPlan：buildStayPlan 用于减速状态下的'停留步'（不移动、不死亡、不增长），snakeMovesThisBattleStep 根据 slowMs 和 slowStepCounter 决定本步是正常移动还是停留。"),

      h2("5.2 碰撞检测"),
      boldPara("单人模式：", "addSingleSnakeCollisions 仅检测蛇头是否碰到自身身体。注意'忽略尾部'逻辑：若本步不增长（grow=false），蛇尾将离开当前尾部格子，因此检测自身碰撞时不应包括该格子。"),
      boldPara("双蛇模式：", "addBattleCollisions 额外处理三种情况：两头进入同一格→同归于尽（RESULT_DRAW）；两头错身而过（P1 去 P2 位置且 P2 去 P1 位置）→同归于尽；P1 头碰 P2 身体（或反之）→碰者死。同样需要考虑对方尾部即将离开的格子。"),
      boldPara("事件碰撞：", "轰炸事件中，buildStepPlan 在 buildStepPlan 阶段检测蛇头是否在 isBombZoneActive 且 bombActive 的格子内。有战斗护盾或普通护盾时消耗护盾（优先消耗 shieldCharges），无护盾时直接标记死亡。applyArrowHit 处理弓箭命中：命中头部→直接死亡，命中身体→从命中段开始截断（target.length = hitIndex），发射者获得 20 分。无主箭矢（ownerIndex==-1）对任意蛇都造成同等伤害。"),

      h2("5.3 AI 寻路与评分"),
      para("AI 的寻路引擎基于 BFS（广度优先搜索），是三个辅助函数的基础："),
      listItem("bfsFirstStepTo：标准 BFS，返回从起点到目标的第一步步长，用于路径追踪"),
      listItem("bfsDistance：标准 BFS，返回最短路径长度，用于距离评分"),
      listItem("floodAreaAfterMove：BFS 从起点出发，返回可达空格总数，用于评估移动后的生存空间"),
      para("BFS 在初始化 visited 矩阵时，将墙壁、障碍物、无护盾时的陷阱/尖刺、以及 isBombZoneActive 为真的整个轰炸区都标记为已访问。这保证了 AI 在寻路阶段就天然避开危险区域，而不是在结果阶段再过滤。高难度 AI 的 hardCandidateScore 评分公式综合考虑了六个维度：可达面积（×3）、道具价值（×85）、食物距离（-18）、与玩家距离加成（5 格内 ×28/14）、压制玩家合法移动数（4-playerMoves×35）、是否保持当前方向（+8）。额外加分项包括：有弓箭且有射击线（+260）、无弓箭时吃弓箭（+220）、无护盾时吃护盾（+120）。惩罚项包括：轰炸区活跃非预警（-600）、轰炸预警（-1500）、轰炸爆破无护盾（AI_BAD_SCORE/4）、无护盾踩尖刺（-1000）。"),

      h2("5.4 随机事件系统"),
      para("随机事件在多样模式（VARIANT_DIVERSE）下的所有游戏模式中生效。核心状态机在 updateRandomEvents 中："),
      listItem("间隔计时：10 秒正常间隔，本地多人最后 30 秒加速到 5 秒，限时挑战模式额外缩短 25%。事件结束后才开始计时下一次间隔，保证两事件间至少有 10 秒无事件窗口。"),
      listItem("地图轰炸（EVENT_BOMBARDMENT）：初始化时在地图内部随机生成 3 个矩形区，总面积约占地图 1/4。每个区的最小面积为 1 格。12 秒内每 3 秒爆破一次（共 4 次），每次爆破前 1 秒进入 bombWarning（橙色预警），爆破时 bombActive=true 持续 500ms（深红色）。蛇在爆破区且 bombActive 时：有护盾则消耗护盾，无护盾即死。"),
      listItem("刀光箭影（EVENT_ARROW_STORM）：初始化时随机选取地图周长一半数量的边界墙格（不重复），每格预计算指向场内的方向。12 秒内每 2 秒发射一次（共 6 波），每个边界源生成无主箭矢（ownerIndex=-1，spawnBorderArrow）。无主箭矢对碰到的任意蛇都造成伤害。渲染：发射瞬间边界格黄色方块闪烁 400ms。"),

      h2("5.5 大地图视口裁剪"),
      para("20×20 地图完整显示在棋盘区域内。50×50 和 100×100 地图使用局部视口。视口计算分为四步："),
      numItem("cellSizeForMap 根据棋盘像素空间和地图大小计算单格像素尺寸，50×50 及以上地图最小 18px 保证可辨认。"),
      numItem("visibleCellsForMap 根据棋盘像素空间和 cellSize 计算可见格子数，不超过地图总尺寸。"),
      numItem("clampViewportStart 将蛇头坐标作为焦点，计算视口左上角坐标（focus - visibleCells/2），约束到 [0, mapSize - visibleCells] 范围内确保不越界。"),
      numItem("渲染循环仅遍历 startRow 到 startRow+visibleCells 范围内的格子，isInView 宏在绘制蛇/道具/弓箭时过滤掉视口外的元素。"),
      para("地面纹理通过双层循环在可见范围内平铺 ground.bmp：putimage 的坐标基于 BOARD_LEFT + (col - startCol) * cellSize。"),

      h2("5.6 随机数生成"),
      para("游戏使用经典的 32 位线性同余生成器（LCG）产生伪随机数：seed = seed × 1103515245 + 12345。取随机数时使用 (seed >> 16) % limit 而非 seed % limit，因为 LCG 的高 16 位比特的统计特性远好于低位——低位周期极短（最低位每两次交替一次），直接取余会导致大地图上障碍物和道具的分布出现明显偏差。种子初始值 = time(NULL) XOR 状态指针地址，确保每次运行结果不同。"),
      new Paragraph({ children: [new PageBreak()] }),

      // ==================== 第六章 扩展接口 ====================
      h1("第六章 扩展接口"),
      para("项目在设计之初就预留了扩展接口，以下列出常见扩展需求的操作方法："),

      boldPara("新增道具：", ""),
      listItem("在 common.h 的 CellType 枚举中添加新类型"),
      listItem("在 game.c 的 maintainItems 中添加放置逻辑（调用 spawnSpecificItem）"),
      listItem("在 game.c 的 applyItemEffect 中添加效果处理"),
      listItem("在 render_easyx.c 的 TextureId 中添加贴图槽位，在 TEXTURE_NAMES 和 FALLBACK_COLORS 中添加对应条目"),
      listItem("如需新音效，在 common.h 的 SoundEvent 中添加枚举，在 audio.c 的 soundPath 中添加文件路径，放入对应的 .wav 文件"),

      boldPara("新增皮肤（地图样式）：", ""),
      listItem("在 assets/<皮肤名>/ 下放入 17 张同名 .bmp 贴图"),
      listItem("在 render_easyx.c 的 SKINS[] 数组中添加条目（folder + name）"),
      listItem("如需生成贴图，在 tools/generate_assets.py 的 SKINS 字典中添加配色定义，运行脚本自动生成"),

      boldPara("新增游戏模式：", ""),
      listItem("在 common.h 的 GameMode 中增加枚举值，GameResult 中增加对应结果值"),
      listItem("在 game.c 的 Game_applyModeDefaults 中设置默认参数（速度、限时、道具池）"),
      listItem("在 game.c 的 Game_init 中添加初始化和蛇放置"),
      listItem("在 game.c 的 Game_update 中添加步进分支"),
      listItem("在 main.c 中连线菜单入口，在 render_easyx.c 中增加侧栏渲染分支"),

      boldPara("新增音效：", ""),
      listItem("在 common.h 的 SoundEvent 中增加枚举值"),
      listItem("在 audio.c 的 soundPath 函数中增加对应 .wav 文件路径"),
      listItem("将 .wav 文件放入 assets/audio/ 目录"),

      boldPara("调整 AI 策略：", ""),
      listItem("修改 ai.c 中的 itemValueForAi 调整道具权重"),
      listItem("修改 hardCandidateScore 调整评分公式"),
      listItem("修改 decideMedium / decideHard 调整各自策略"),
      listItem("修改 isBombDanger / isBombZoneActive 调整事件感知灵敏度"),

      boldPara("新增设置项：", ""),
      listItem("在 common.h 的 GameConfig 中添加新字段"),
      listItem("在 ui.c 的 Ui_runSettings 中添加修改逻辑"),
      listItem("在 render_easyx.c 的 Render_drawSettings 中添加显示行"),
    ],
  }],
});

Packer.toBuffer(doc).then(buf => {
  const out = "E:/C语言项目/BUPT_Eating_Snake_Graphics/docs/图形化贪吃蛇概要设计说明书_最终版.docx";
  fs.writeFileSync(out, buf);
  console.log("Done: " + out);
});
