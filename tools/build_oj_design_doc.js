const fs = require("fs");
const { Document, Packer, Paragraph, TextRun, HeadingLevel, AlignmentType, LevelFormat, PageBreak } = require("docx");

const F = "Microsoft YaHei", M = "Consolas", B = "2B6CB0", D = "1A1A2E", G = "666666";

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
      new Paragraph({ alignment: AlignmentType.CENTER, spacing: { after: 280 }, children: [new TextRun({ text: "基于 BFS + Flood Fill + 递归前瞻的 AI 决策贪吃蛇", font: F, size: 22, color: G })] }),
      p("姓  名：黄海彬", { c: D }), p("技术栈：C 语言（C99）", { c: G }), p("开发环境：GCC / MSVC + 命令行", { c: G }),
      emp(), emp(), emp(), emp(), emp(),
      new Paragraph({ children: [new PageBreak()] }),

// ================ 第一章 ================
      h1("第一章  项目概述"),
      h2("1.1  项目背景"),
      p("贪吃蛇是一款经典街机游戏，起源于 1976 年 Gremlin 公司的 Blockade，1997 年预装到诺基亚 6110 手机后实现全球普及。核心玩法为操控蛇头移动吞食食物使蛇身增长，同时避免撞到墙壁、障碍物或自身。"),
      p("本项目是北京邮电大学计算机导论课程的大作业 OJ 版。程序通过标准输入输出与评测系统交互：评测系统输入初始地图和 N 值（步增长间隔），程序自主计算每一步的移动方向并输出，评测系统反馈新食物坐标或终止信号。目标是在 10 组不同难度的测试数据中最大化加权总分。"),

      h2("1.2  功能需求"),
      ni("读取 20 行 × 20 列初始字符地图 + 第 21 行的整数 N"),
      ni("每回合输出一个方向字符（W/A/S/D）和当前得分，输出后 fflush(stdout)"),
      ni("读取评测系统反馈的下一食物坐标（或 100 100 终止信号）"),
      ni("蛇身增长：吃食物当即加一节；每 N 步自动加一节；两者同时满足仅加一节"),
      ni("死亡判定：撞墙、撞障碍物、撞自身、反方向移动"),
      ni("AI 全自动决策——程序不依赖任何外部输入进行方向选择"),

      h2("1.3  输入输出协议"),
      bp("输入格式：", "前 20 行为 20×20 字符地图（每行 20 个字符），第 21 行为整数 N。此后每回合输入一行两个整数：食物行号和列号（0~19），或 (100, 100) 表示结束，或 (20, 20) 表示无新食物。"),
      bp("输出格式：", "每回合一行方向字符 + 一行分数整数。输出后立即 fflush(stdout)。"),
      bp("地图字符：", "# = 墙壁（边界），. = 空地，H = 蛇头，B = 蛇身，F = 食物，O = 障碍物。"),
      new Paragraph({ children: [new PageBreak()] }),

// ================ 第二章 ================
      h1("第二章  数据结构设计"),
      h2("2.1  全局宏定义"),
      ...co([
        "#define SIZE 20           // 地图固定 20x20",
        "#define MAX_LEN 400       // 最大蛇长 = 地图面积",
        "#define FOOD_SCORE 10     // 每个食物 10 分",
        "#define INF 1000000000    // 无穷大",
        "#define EMPTY '.'         // 空地",
        "#define WALL '#'          // 墙壁",
        "#define HEAD 'H'          // 蛇头",
        "#define BODY 'B'          // 蛇身",
        "#define FOOD 'F'          // 食物",
        "#define OBSTACLE 'O'      // 障碍物",
      ]),
      p("四个方向宏极大简化了代码："),
      ...co([
        "#define DR(d) ((d)==0 ? -1 : ((d)==2 ? 1 : 0))  // 行偏移：上=-1,下=+1",
        "#define DC(d) ((d)==1 ? -1 : ((d)==3 ? 1 : 0))  // 列偏移：左=-1,右=+1",
        "#define DIR_CHAR(d) (\"WASD\"[(d)])            // 方向枚举→字符",
        "#define IN(r,c) ((r)>=0&&(r)<SIZE&&(c)>=0&&(c)<SIZE) // 坐标合法性",
        "#define REV(a,b) (((a)+2)%4==(b))              // 是否反方向",
        "#define GROW(g,rr,cc) (((g)->hf&&(rr)==(g)->fd.r&&  \\",
        "                         (cc)==(g)->fd.c)||(g)->st+1==(g)->gn)",
      ]),

      h2("2.2  核心结构体"),
      ...co([
        "typedef struct { int r; int c; } Pos;   // 二维坐标",
        "",
        "typedef struct {",
        "    char m[SIZE][SIZE+1];   // 地图数组（20行字符串）",
        "    int sr[MAX_LEN];        // 蛇身行坐标数组，sr[0]=头",
        "    int sc[MAX_LEN];        // 蛇身列坐标数组",
        "    int len;                // 当前长度",
        "    int dir;                // 当前方向（0=上,1=左,2=下,3=右）",
        "    int sco;                // 累计得分",
        "    int st;                 // 距上次增长的步数",
        "    int gn;                 // N 步增长间隔（growthN）",
        "    int hf;                 // 1=地图上有食物, 0=无",
        "    Pos fd;                // 食物坐标",
        "    unsigned char u[SIZE][SIZE];  // BFS visited 复用",
        "    unsigned char qr[MAX_LEN];    // BFS 队列（行）",
        "    unsigned char qc[MAX_LEN];    // BFS 队列（列）",
        "} Snake;",
      ]),
      p("设计要点：蛇身用两个并行数组 sr[] 和 sc[] 存储坐标，而非链表。数组支持 O(1) 随机访问任意节，无需动态分配。BFS 的 visited 矩阵 u[][] 和队列 qr/qc 内嵌在 Snake 结构体中，所有函数复用同一块内存，避免反复 malloc/free。方向编码为整数 0-3（对应 WASD），使反向判断 REV(a,b) 仅需 (a+2)%4==b 一行完成。"),

      new Paragraph({ children: [new PageBreak()] }),

// ================ 第三章 ================
      h1("第三章  核心算法设计"),
      h2("3.1  蛇身初始化"),
      p("initS 函数从蛇头出发，四方向搜索相邻的 BODY 字符，按发现的顺序构建蛇身数组。搜索过程中用 u[][] 标记已访问节点防止重复。完成后清理地图中不属于当前蛇的残余 BODY 标记。采用数组顺序存储（sr[0] 为头，sr[len-1] 为尾），面向算法的随机访问效率优于链表。"),

      h2("3.2  移动与碰撞检测"),
      p("move(g, d) 函数执行一步完整移动："),
      ni("计算新头部坐标，通过 legal 检查合法性"),
      ni("若不增长，将尾部坐标在地图中抹除"),
      ni("蛇身数组整体后移一位，头部插入新坐标"),
      ni("若吃到食物，加分并标记 hf=0"),
      ni("更新步计数 st，到达 gn 时归零"),
      p("legal(g, d) 检测三件事：是否是反方向、是否出界/撞墙/撞障碍、是否撞自身（不增长时忽略尾部——因为尾部即将离开）。"),

      h2("3.3  BFS 寻路"),
      p("bfsT(g, tar, at) 是标准 BFS，返回从蛇头到目标 tar 路径的第一步方向。"),
      bp("关键优化：", "visited 矩阵 u[][] 不仅标记已访问，还编码第一步方向——蛇头位置初始化为 5，每个扩展节点继承父节点的方向编码（1-4 对应 WASD）。BFS 抵达目标时直接从 u[tar.r][tar.c] 还原第一步方向，无需回溯路径。"),
      bp("at 参数：", "为 true 时 block 函数将蛇尾标记为可通过（因为即将离开），模拟增长后的状态。"),

      h2("3.4  Flood Fill 区域评估"),
      p("flood(g, &reach, &doors) 从蛇头 BFS 遍历所有可达空地，返回可达格子总数。同时输出两个辅助指标："),
      li("reach：BFS 是否能到达蛇尾（用于判断蛇能否'转圈'存活）"),
      li("doors：蛇头周边四个方向中有几个是安全的（用于评估被围困风险）"),
      p("reach 和 doors 在后续评分中作为关键风险指标——reach=0 说明蛇被自己的尾巴堵死，doors≤1 说明几乎被围困。"),

      h2("3.5  食物路径安全仿真（safeF）"),
      p("safeF(g, fd1, &steps, &sp) 模拟蛇沿着 BFS 最短路径追踪食物的全过程。每一步重新计算 BFS 到食物目标，适应食物在追索过程中可能发生的变化。"),
      p("仿真过程中每隔几步执行一次 flood 检查，确保蛇不会在追食过程中自我围困。仿真步数上限由蛇到食物的曼哈顿距离 + 35 决定（最少 45 步，最多 110 步）。"),
      p("安全条件：仿真结束时可到达食物、最终 flood 面积 ≥ 蛇长 + 安全余量（N≤2 时余量 10，N≤8 时余量 8，余量 5）、doors > 1（大于 6 节蛇时）。返回值为 1 + 奖励分（步数 ≤6 加 1 分 + 面积 ≥ 蛇长+30 加 1 分），0 表示不安全。"),

      h2("3.6  递归前瞻（fut）"),
      p("fut(g, depth) 是 AI 的核心前瞻函数。对当前蛇位置，遍历四个合法方向，对每个方向模拟移动一步，调用 flood 评估移动后的空间质量。评分公式为：面积×5 + doors×40 + reach 奖励 − 食物距离惩罚 − 空间不足惩罚。然后递归调用 fut(next, depth-1)/4 加入更深层的评估。"),
      p("depth 参数控制前瞻深度：N≤4 时深度 4，N≤16 时深度 3，N≤256 时深度 2，N≥512 时深度 1。N 越大蛇增长越快，深前瞻的意义越小（因为蛇很快会变长改变局面）。"),

      h2("3.7  蛇形顺序引导（ord）"),
      p("ord(r, c) 将 20×20 棋盘映射为 1~324 的蛇形编号：奇数行从左到右递增，偶数行从右到左递增。在 eval 评分中，计算移动前后蛇头 ord 值的变化 gap。gap 接近 0 说明蛇在地图的'下游'（被自己堵住），gap 大说明蛇在'上游'（有更多探索空间）。通过罚分引导蛇在空旷地图中保持前进方向，避免无意义的折返绕圈。"),

      h2("3.8  综合评分（eval）"),
      p("eval(g, d, fd0, depth) 对候选方向 d 计算综合评分，权重根据 N 值动态调整："),
      bp("空间评分：", "flood 面积×sw（N≤4 时 24，N≤32 时 24，N≥512 时 16）+ doors×90。面积不足时强惩罚（每缺少一格扣 650+N×230）。"),
      bp("子节点分析（childS）：", "当 N>2 且面积不足或 doors≤1 时，进一步分析移动后每个合法方向的'最坏情况子节点'——即子方向中 flood 面积最小者。childS 返回 max(N 个子节点面积)。若最坏子节点面积 < 蛇长+安全值，扣 (蛇长+安全值-子节点面积)×230+260。"),
      bp("食物距离：", "若蛇头到食物的 BFS 方向与候选方向 d 一致（d==fd0），通过 safeF 仿真验证安全性——安全则加分 360+safe×170+sp/2−步数×惩罚，不安全则扣分。大 N 时更激进：加分权重大于扣分。"),
      bp("吃到食物：", "eat 直接加分，N≥512 时 +1500（最激进），N≤4 时 +1300（保守）。"),
      bp("ord 引导：", "gap≤蛇长+2 时扣 180 分（蛇在自己的轨迹后方），gap≤45 时加 90−gap 分（鼓励向前）。"),
      p("所有分数加上 fut(next, depth)/3 的前瞻值，取总分最高的方向。"),

      h2("3.9  主决策函数（choose）"),
      p("choose(g) 是最终决策入口："),
      ni("先用 BFS 计算到食物的最短路径 fd0"),
      ni("若 fd0 安全且满足条件，直接返回（快速通道）"),
      ni("否则对四个方向调用 eval 评分，取最高分方向"),
      ni("N≤64 时对次优方向加深前瞻验证，防止评分误判"),
      ni("兜底：选第一个非反方向"),
      p("大 N（≥512）且长时间低分时（探索超过 900 步仍 <300 分），强制 BFS 追食物，打破 AI 的保守策略。"),

      new Paragraph({ children: [new PageBreak()] }),

// ================ 第四章 ================
      h1("第四章  程序流程"),
      h2("4.1  主函数"),
      ...co([
        "int main(void) {",
        "    Snake game = {0};",
        "    readG(&game);      // 读取地图和 N 值",
        "    loopG(&game);      // 主循环：决策→输出→移动→读取反馈",
        "    return 0;",
        "}",
      ]),
      h2("4.2  游戏主循环"),
      p("loopG 是 OJ 交互的核心循环，每回合执行："),
      ni("choose(g) 计算最优方向"),
      ni("打印方向字符和分数，fflush 立即刷新"),
      ni("move(g, d) 执行移动——更新蛇身数组和地图"),
      ni("scanf 读取评测系统反馈的下一食物坐标"),
      ni("若 (100,100) 则跳出循环，打印最终地图和分数"),
      new Paragraph({ children: [new PageBreak()] }),

// ================ 第五章 ================
      h1("第五章  算法深度分析"),
      h2("5.1  为什么数组优于链表"),
      p("本方案使用 sr[]/sc[] 并行数组存储蛇身，本质是'蛇头在索引 0 的单端队列'。优势："),
      li("O(1) 随机访问任意节——碰撞检测和 flood/BFS 中需要频繁检查某坐标是否为蛇身"),
      li("移动时整体后移，仅在增长时多占用一个数组位——均为 O(n) 但 n 最多 400，可忽略"),
      li("无需 malloc/free，全部在 Snake 结构体内联分配，零堆碎片"),
      li("visited 矩阵和 BFS 队列也内联复用，省去反复分配开销"),

      h2("5.2  前瞻深度的动态调整"),
      p("N 值是决定 AI 策略激进程度的关键参数。N 小（≤4）时蛇增长慢，AI 可以深前瞻（depth=4）仔细规划；N 大（≥512）时蛇几乎每步都增长，局面变化快，深前瞻意义不大（depth=1），转而采用更激进的食物追逐策略。这种'自适应前瞻'是本方案的核心创新——不是给所有局面用同样的深度，而是根据 N 值选择的'时间预算'合理分配。"),

      h2("5.3  ord 引导的必要性"),
      p("在空旷地图中（尤其是食物距离远时），纯 BFS+Flood 的 AI 会出现'无限徘徊'——蛇在空旷区域来回游走不确定方向。ord 通过给 20×20 棋盘分配唯一编号，将'蛇是否在前进'编码为可评分量。gap>0 表示蛇头在编号上'向前走'，gap<0 表示'往回走'。这个简单的启发式在大地图场景中显著减少无意义徘徊。"),

      h2("5.4  safeF 的仿真前瞻价值"),
      p("safeF 是评估'追食物是否安全'的关键函数。它模拟蛇沿着 BFS 最短路径追食物的全过程（步数上限随距离动态调整），每几步检查一次 flood 面积。这相当于在一个简化版决策树中做深度优先搜索——只沿食物方向探索，大大减少了分支数量。当仿真通过时，蛇会更有信心地去追食物；不通过时则保守避让。"),

      new Paragraph({ children: [new PageBreak()] }),

// ================ 第六章 ================
      h1("第六章  测试与评测"),
      h2("6.1  评测规则"),
      p("OJ 系统提供 10 组测试数据，每组的地图布局、蛇初始位置、食物位置和 N 值均不同。10 组独立运行，每组原始分除以 N 得到加权分，10 组加权分之和为最终总分。N 越大（蛇增长越快），加权分越低——这意味着大 N 组的难度更高，对 AI 的综合要求也更严苛。"),
      p("OJ 满分 58 分：基础分 50 分（总加权分 ≥500 得满分）+ 排名加分 8 分（全年级前 5% 得 8 分）。"),

      h2("6.2  测试策略"),
      li("本地编译后通过输入重定向模拟 OJ 交互"),
      li("构造边界测试：N=1（每步都增长）、蛇头贴墙、食物在死角等极端情况"),
      li("验证大 N（≥512）长时间运行的稳定性——确保不会因满分失效卡死"),
      li("统计 10 组加权总分，对照评测标准预估最终得分"),

      new Paragraph({ children: [new PageBreak()] }),

// ================ 第七章 ================
      h1("第七章  总结"),
      p("贪吃蛇 OJ 版是一个考验 AI 决策能力的编程题目。核心挑战在于：蛇在持续增长的同时，必须在'吃食物得分'和'留出生存空间'之间实时权衡。本方案通过 BFS 最短路径寻路 + Flood Fill 面积评估 + 递归前瞻 + 蛇形引导的四层递进策略，实现了在 10 组不同 N 值下的自适应决策。"),
      p("算法亮点包括："),
      ni("safeF 食物路径安全仿真——在追食前验证路径可行性"),
      ni("childS 最坏子节点分析——评估移动后'最狭窄'方向的生存空间"),
      ni("ord 蛇形顺序引导——在空旷地图中指引蛇保持前进方向"),
      ni("adaptive depth 自适应前瞻——根据 N 值动态调整前瞻深度"),
      p("数组存储蛇身 + 内联 BFS 复用的设计使得每步决策时间稳定在毫秒级，满足评测的时效要求。"),

    ],
  }],
});

Packer.toBuffer(doc).then(b => {
  const o = "E:/C语言项目/BUPT_Eating_Snake_Graphics/docs/OJ版贪吃蛇概要设计说明书_黄海彬_v2.docx";
  fs.writeFileSync(o, b);
  console.log("Done: " + o);
});
