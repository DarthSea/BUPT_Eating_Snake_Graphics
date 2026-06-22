const fs = require("fs");
const { Document, Packer, Paragraph, TextRun, HeadingLevel, AlignmentType, LevelFormat, PageBreak, BorderStyle, TabStopType, TabStopPosition } = require("docx");

const F = "Microsoft YaHei";
const M = "Consolas";
const B = "2B6CB0";
const D = "1A1A2E";
const G = "666666";

function h1(t) { return new Paragraph({ heading: HeadingLevel.HEADING_1, spacing: { before: 320, after: 200 }, children: [new TextRun({ text: t, font: F, size: 34, bold: true, color: D })] }); }
function h2(t) { return new Paragraph({ heading: HeadingLevel.HEADING_2, spacing: { before: 260, after: 140 }, children: [new TextRun({ text: t, font: F, size: 28, bold: true, color: B })] }); }
function h3(t) { return new Paragraph({ heading: HeadingLevel.HEADING_3, spacing: { before: 200, after: 100 }, children: [new TextRun({ text: t, font: F, size: 24, bold: true, color: D })] }); }
function p(t, o = {}) { return new Paragraph({ spacing: { after: 100, line: 340 }, children: [new TextRun({ text: t, font: F, size: 22, color: o.c || D })] }); }
function bp(l, t) { return new Paragraph({ spacing: { after: 60, line: 340 }, children: [new TextRun({ text: l, font: F, size: 22, bold: true, color: D }), new TextRun({ text: t, font: F, size: 22, color: D })] }); }
function li(t) { return new Paragraph({ numbering: { reference: "b", level: 0 }, spacing: { after: 50, line: 300 }, children: [new TextRun({ text: t, font: F, size: 22, color: D })] }); }
function ni(t) { return new Paragraph({ numbering: { reference: "n", level: 0 }, spacing: { after: 50, line: 300 }, children: [new TextRun({ text: t, font: F, size: 22, color: D })] }); }
function co(l) { return l.map(line => new Paragraph({ spacing: { after: 0, line: 260 }, indent: { left: 360 }, children: [new TextRun({ text: line || " ", font: M, size: 18, color: "333333" })] })); }
function emp() { return new Paragraph({ spacing: { after: 60 }, children: [] }); }

const doc = new Document({
  styles: {
    default: { document: { run: { font: F, size: 22 } } },
    paragraphStyles: [
      { id: "Heading1", name: "Heading 1", basedOn: "Normal", next: "Normal", quickFormat: true, run: { size: 34, bold: true, font: F, color: D }, paragraph: { spacing: { before: 320, after: 200 }, outlineLevel: 0 } },
      { id: "Heading2", name: "Heading 2", basedOn: "Normal", next: "Normal", quickFormat: true, run: { size: 28, bold: true, font: F, color: B }, paragraph: { spacing: { before: 260, after: 140 }, outlineLevel: 1 } },
      { id: "Heading3", name: "Heading 3", basedOn: "Normal", next: "Normal", quickFormat: true, run: { size: 24, bold: true, font: F, color: D }, paragraph: { spacing: { before: 200, after: 100 }, outlineLevel: 2 } },
    ],
  },
  numbering: { config: [
    { reference: "b", levels: [{ level: 0, format: LevelFormat.BULLET, text: "·", alignment: AlignmentType.LEFT, style: { paragraph: { indent: { left: 720, hanging: 360 } } } }] },
    { reference: "n", levels: [{ level: 0, format: LevelFormat.DECIMAL, text: "%1.", alignment: AlignmentType.LEFT, style: { paragraph: { indent: { left: 720, hanging: 360 } } } }] },
  ] },
  sections: [{
    properties: { page: { size: { width: 11906, height: 16838 }, margin: { top: 1440, right: 1440, bottom: 1440, left: 1440 } } },
    children: [
      emp(), emp(), emp(), emp(),
      new Paragraph({ alignment: AlignmentType.CENTER, spacing: { after: 160 }, children: [new TextRun({ text: "贪吃蛇 OJ 版", font: F, size: 52, bold: true, color: D })] }),
      new Paragraph({ alignment: AlignmentType.CENTER, spacing: { after: 100 }, children: [new TextRun({ text: "概要设计说明书", font: F, size: 38, bold: true, color: B })] }),
      new Paragraph({ alignment: AlignmentType.CENTER, spacing: { after: 280 }, children: [new TextRun({ text: "基于 BFS 的 AI 自动寻路贪吃蛇", font: F, size: 22, color: G })] }),
      p("姓  名：黄海彬", { c: D }), p("技术栈：C 语言（C99 标准）", { c: G }), p("开发环境：GCC / MSVC + 命令行", { c: G }),
      emp(), emp(), emp(), emp(), emp(),
      new Paragraph({ children: [new PageBreak()] }),

      h1("第一章  项目概述"),
      h2("1.1  项目背景"),
      p("贪吃蛇是一款经典的街机游戏，起源于 1976 年 Gremlin 公司的 Blockade。1997 年被预装到诺基亚 6110 手机后实现全球普及。游戏的核心玩法为操控蛇头移动、吞食食物使蛇身增长，同时避免撞到墙壁、障碍物或自身。"),
      p("本项目是北京邮电大学计算机导论课程的大作业 OJ 版。OJ 版要求程序通过标准输入输出与评测系统交互：评测系统输入初始地图和 N 值，程序自行决策每一步的移动方向并输出，评测系统反馈新的食物坐标或终止信号。与图形化版不同，OJ 版不涉及图形渲染和用户交互，专注于 AI 自动寻路决策算法的设计与优化。"),

      h2("1.2  功能需求"),
      p("OJ 版需要实现以下核心功能："),
      ni("读取 20 行 × 20 列初始字符地图 + 第 21 行的 N 值"),
      ni("每一回合输出移动方向（W/A/S/D）和当前分数"),
      ni("读取评测系统反馈的食物坐标（或 100 100 终止信号）"),
      ni("蛇身增长规则：吃食物立即加一节；每 N 步自动加一节（两条件同时满足仅加一节）"),
      ni("死亡判定：撞墙、撞障碍物、撞自身（含反方向移动）"),
      ni("AI 自动决策：在不依赖外部输入的情况下，每步计算最优移动方向"),

      h2("1.3  输入输出协议"),
      bp("输入格式：", "前 20 行为 20×20 字符地图（每行 20 个字符，无空格），第 21 行为整数 N（步增长间隔）。每回合反馈一行的两个整数：新的食物行号和列号（0~19），或 100 100 表示游戏结束，或 20 20 表示本回合无新食物。"),
      bp("输出格式：", "每回合输出一行：一个方向字符（W/A/S/D）和一个整数（当前得分），中间换行分隔。输出后立即调用 fflush(stdout) 刷新缓冲区。"),
      bp("地图字符含义：", "# 为墙壁（地图边缘），. 为空地，H 为蛇头，B 为蛇身，F 为食物，O 为障碍物。"),
      new Paragraph({ children: [new PageBreak()] }),

      h1("第二章  数据结构设计"),
      h2("2.1  全局常量"),
      ...co([
        "#define MAP_SIZE 20        // 地图固定 20x20",
        "#define WALL '#'           // 墙壁字符",
        "#define SNAKE_HEAD 'H'     // 蛇头",
        "#define SNAKE_BODY 'B'     // 蛇身",
        "#define FOOD 'F'           // 食物",
        "#define OBSTACLE 'O'       // 障碍物",
        "#define MAX_SNAKE_LEN 400  // 最大蛇长 = 地图面积",
        "#define FOOD_SCORE 10      // 每个食物 10 分",
      ]),
      p("地图大小为固定的 20×20，所有坐标范围为 0~19。蛇最大长度理论上可达 400 节（占满整个地图）。"),

      h2("2.2  方向与状态枚举"),
      ...co([
        "enum DIR { DIR_UP = 0, DIR_DOWN, DIR_LEFT, DIR_RIGHT };",
        "enum GAME_STATUS { OK = 0, KILL_BY_WALL, KILL_BY_SELF, KILL_BY_OBSTACLE, OVER };",
      ]),
      p("DIR 枚举定义四个基本方向。GAME_STATUS 枚举记录蛇的死亡原因：撞墙、撞自身、撞障碍物，OVER 表示评测系统发出终止信号。"),

      h2("2.3  核心结构体"),
      ...co([
        "typedef struct { int x; int y; } Pos;",
        "",
        "typedef struct Snake_Node {",
        "    Pos pos;",
        "    struct Snake_Node* next_node;",
        "} SN, *pSN;",
        "",
        "typedef struct Snake {",
        "    pSN _pSnake;        // 蛇头节点（链表头）",
        "    pSN _pFood;         // 食物位置节点",
        "    enum DIR _dir;      // 当前移动方向",
        "    enum GAME_STATUS _status;  // 游戏状态",
        "    int len;            // 当前长度",
        "    int score;          // 累计得分",
        "    int stepCount;      // 自上次增长后的步数",
        "    int growthN;        // N 步增长间隔",
        "} Snake, *pSnake;",
      ]),
      p("设计要点：蛇的身体采用单向链表存储，蛇头为链表头节点。选择链表而非数组的原因："),
      li("在 C 语言标准下，无需预先估计最大长度，灵活性更高"),
      li("移动时只需在头部插入新节点，尾部删除节点（若不增长），O(1) 时间复杂度"),
      li("删除尾部前需遍历到倒数第二个节点（O(n)），但蛇长通常不超过数十字节，遍历开销可忽略"),
      p("食物单独存储为 pSN 节点（仅利用其 pos 字段），避免在全图中搜索食物位置。"),
      new Paragraph({ children: [new PageBreak()] }),

      h1("第三章  核心算法设计"),
      h2("3.1  算法总览"),
      p("OJ 版的核心挑战是：蛇必须自主决策每一步的移动方向，在吃食物得分的同时避免死亡。算法采用七层优先级递减的决策策略："),
      ni("BFS 寻食物 + 安全移动 + 虚拟尾部可达性检查"),
      ni("BFS 寻食物（封堵尾部）+ 安全移动 + 虚拟检查"),
      ni("近距离食物 + flood 区域面积检查"),
      ni("BFS 寻尾部（跟随自己尾巴）"),
      ni("安全方向中曼哈顿距离最近食物的方向 + flood 面积决胜"),
      ni("安全方向中 flood 面积最大者"),
      ni("任意不碰撞方向（兜底）"),
      p("这七层策略从前到后依次降级，保证蛇在任何情况下都能找到一个合法的移动方向。"),

      h2("3.2  BFS 寻路算法"),
      p("bfs(sx, sy, gx, gy, blockTail) 函数是 AI 决策的核心引擎。它执行标准广度优先搜索，从起点 (sx,sy) 出发寻找目标 (gx,gy) 的最短路径。"),
      bp("实现原理：", "将地图抽象为二维网格，墙壁和障碍物为不可达节点。蛇的身体根据 blockTail 参数决定是否标记尾部节点为障碍（blockTail=1 时整条蛇都阻塞，blockTail=0 时尾部节点视为可达）。visited 数组记录已访问节点，queue 数组实现 BFS 队列。pr 数组记录每个节点的前驱坐标，用于回溯路径。"),
      p("BFS 找到目标后，通过 pr 数组从目标逐层回溯到起点的邻居，返回第一步的方向枚举值。若找不到路径则返回 -1。"),
      ...co([
        "int bfs(pSnake ps, int sx, int sy, int gx, int gy, int blockTail) {",
        "    // 1. 标记墙壁、障碍物、蛇身为已访问",
        "    // 2. 若 blockTail=1，尾部也标记（整条蛇全堵）",
        "    // 3. BFS 主循环：四方向扩展",
        "    // 4. 找到目标后通过 pr 回溯，返回第一步方向",
        "    return -1;  // 无路径",
        "}",
      ]),

      h2("3.3  Flood Fill 区域面积评估"),
      p("flood(sx, sy, grow) 函数使用 BFS 泛洪算法计算从起点 (sx,sy) 出发可达的空地总数。grow 参数为 true 时蛇尾不移动（蛇变长），整条蛇都标记为障碍；grow 为 false 时尾部节点标记为可达。"),
      p("区域面积用于评估移动后的生存空间——面积越大，蛇未来可走的步数越多，越不容易被困死。在 P5 和 P6 策略中作为决胜条件使用。"),

      h2("3.4  安全移动检测"),
      p("safeMove(ps, dir) 函数检查沿 dir 方向移动一步是否安全。检测流程："),
      ni("计算新的头部坐标，检查是否出界/撞墙/撞障碍物"),
      ni("检查是否撞蛇身（考虑是否增长，若不增长则忽略尾部）"),
      ni("BFS 检查从新头部到未来尾部的可达性（能否追到自己尾巴）"),
      p("三步全通过才视为安全。BFS 可达性检查是防止蛇走入选定方向的'死胡同'——即空间足够窄导致蛇头无法回到尾部。"),

      h2("3.5  七层决策策略详解"),
      bp("P1（最优）：", "BFS 寻食物（不封堵尾部）+ safeMove + BFS 从食物到尾部（blockTail=1）的可达性检查。保证吃完食物后还能活着。"),
      bp("P2（次优）：", "BFS 寻食物（封堵尾部）做同样检查。blockTail=1 模拟蛇已经吃掉食物增长后的状态。"),
      bp("P3（近距离取食）：", "若食物在曼哈顿距离 ≤3 以内且 flood 面积 > len+10，直接去吃。近距离食物优先吃掉避免错过。"),
      bp("P4（跟随尾部）：", "BFS 寻自己的尾巴（blockTail=0 和 1 各尝试一次）+ safeMove。用于在无食物时的生存策略——绕圈子等待新食物。"),
      bp("P5（距离+面积）：", "遍历四个方向，对每个安全方向计算到食物的曼哈顿距离和移动后 flood 面积。选距离最近者，同距离选面积最大者。"),
      bp("P6（最大面积）：", "遍历四个方向，选 flood 面积最大的安全方向。纯粹的生存策略。"),
      bp("P7（兜底）：", "选任意不碰撞的方向。保证至少能走一步。"),
      new Paragraph({ children: [new PageBreak()] }),

      h1("第四章  程序流程"),
      h2("4.1  主函数"),
      ...co([
        "int main() {",
        "    Snake snake = { 0 };",
        "    initGame(&snake);   // 读取地图 + 初始化蛇",
        "    gameLoop(&snake);   // 主循环",
        "    return 0;",
        "}",
      ]),

      h2("4.2  初始化流程"),
      p("initGame 调用 readMap 读取 20 行地图字符和第 21 行的 N 值，定位蛇头坐标和食物坐标。然后调用 initSnake 通过八方向搜索从蛇头出发，沿相邻的 B 字符逐段重建蛇身链表。蛇身初始化后清理地图中的残余 B 标记防止误判。"),

      h2("4.3  游戏主循环"),
      p("gameLoop 是 OJ 交互的核心循环："),
      ni("调用 decideDirection 计算本步最优方向"),
      ni("输出方向字符和当前分数（fflush 刷新）"),
      ni("调用 moveSnake 执行移动——检查反方向、碰撞、增长、吃食物"),
      ni("读取评测系统反馈的新食物坐标"),
      ni("若两个数均为 100 则游戏结束，跳出循环"),
      ni("跳出循环后打印最终地图和分数"),
      new Paragraph({ children: [new PageBreak()] }),

      h1("第五章  测试策略"),
      h2("5.1  评测规则"),
      p("OJ 系统提供 10 组测试数据，每组数据包含不同的地图布局、蛇初始位置、食物位置和 N 值。程序自动运行 10 组，取最高分作为最终成绩。每组原始分按公式加权计算：原始分 / N——N 越大（蛇增长速度越快），加权分越低，鼓励蛇在 N 较大的困难条件下取得高分。"),
      p("OJ 总分为 58 分：基础分 50 分（总加权分 ≥500 得满分）+ 排名加成分 8 分（全年级前 5% 得 8 分，依次递减）。"),

      h2("5.2  自我测试方法"),
      li("本地编译后通过管道重定向模拟 OJ 交互：将测试数据写入文本文件，通过 < 输入重定向和 > 输出重定向验证程序行为"),
      li("手动构造边界测试用例：蛇头紧贴墙壁、食物在对角线最远处、N=1（每步都增长）等极端情况"),
      li("统计 10 组测试数据的得分加权和，确认达到预期分数"),

      h1("第六章  总结"),
      p("贪吃蛇 OJ 版是一个典型的 AI 决策类编程题目。核心难点在于：蛇在不断增长的同时必须避免自我围困。本方案通过 BFS 寻路 + Flood Fill 面积评估 + 七层优先级的组合策略，在吃食物和保生存之间动态平衡。BFS 保证找到最短路径，Flood Fill 评估长期生存空间，多层降级策略确保任何情况下都有合法走法。链表存储蛇身虽然遍历效率不及数组，但在 20×20 的小规模地图上性能完全足够，且代码简洁直观。"),

    ],
  }],
});

Packer.toBuffer(doc).then(b => {
  const o = "E:/C语言项目/BUPT_Eating_Snake_Graphics/docs/OJ版贪吃蛇概要设计说明书_黄海彬.docx";
  fs.writeFileSync(o, b);
  console.log("Done: " + o);
});
