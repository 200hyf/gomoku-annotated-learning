# 五子棋注释学习版 · 完整学习笔记

> 本笔记覆盖本项目从"游戏开发"到"开源发布"的全链路知识点，按主题组织，每节含 **是什么 / 为什么 / 怎么用 / 踩过的坑**。

---

## 目录

1. [项目架构](#一项目架构)
2. [窗口与界面](#二窗口与界面)
3. [输入与交互](#三输入与交互)
4. [棋盘与棋子](#四棋盘与棋子)
5. [游戏规则（五子棋/围棋/井字棋）](#五游戏规则)
6. [代码质量与精简封装](#六代码质量与精简封装)
7. [Git 与 GitHub 开源](#七git-与-github-开源)
8. [打包安装包（Inno Setup）](#八打包安装包inno-setup)
9. [GitHub Releases 发布](#九github-releases-发布)
10. [踩坑记录汇总](#十踩坑记录汇总)
11. [常用命令速查](#十一常用命令速查)

---

## 一、项目架构

### 1.1 四个源文件各管什么

| 文件 | 职责 |
|---|---|
| `connect.h` | **结构定义 + 全局声明**：按钮、输入框、页面结构、常量、`extern` 全局变量 |
| `main.cpp` | **主循环**：消息处理、页面切换、落子与胜负判定、对局页绘制 |
| `page.cpp` | **页面布局**：六页的排版、按钮点击分发 |
| `toolFunction.cpp` | **对局核心**：落子、围棋规则、胜负判定、窗口模式 |

> 学习点：**头文件放"声明"，源文件放"定义"**。`connect.h` 里用 `extern int g_turn;` 声明，`toolFunction.cpp` 里写 `int g_turn = 1;` 定义。其他文件要用时 `#include "connect.h"` 即可。

### 1.2 全局状态（"游戏的大脑"）

```cpp
int g_board[31][31];  // 棋盘：0=空 1=黑 2=白
int g_turn;           // 当前执棋：1=黑 2=白
int g_game;           // 模式：1=五子棋 2=围棋 3=井字棋
int g_winner;         // 0=进行 1=黑胜 2=白胜 3=平局
int g_blackCap;       // 围棋：黑方提子数
int g_whiteCap;       // 围棋：白方提子数
```

> 学习点：**用全局变量存"跨函数共享的状态"**，游戏逻辑函数（putChess / checkWin）读写它们，界面函数只负责显示。

---

## 二、窗口与界面

### 2.1 三种窗口模式

- **窗口化**：可自由拖动改变大小
- **全屏窗口化**：占满屏幕但不隐藏任务栏（保留状态栏）
- **全屏**：真正的全屏（无边框无任务栏）

切换按钮在界面上点击即可；核心函数写在 `toolFunction.cpp`（窗口三版本），每个版本用不同参数组合调用 Windows API。

### 2.2 窗口大小改变时画面要跟着变

> 踩坑：只改窗口大小不重绘，画面会"歪"（棋盘不居中、按钮错位）。

**Resize + 重绘逻辑**：每次窗口尺寸变化 → 重新计算棋盘原点坐标和按钮位置 → 重画。棋盘居中公式：

```cpp
// 棋盘左上角 = (窗口宽 - 棋盘总宽) / 2
board.x = getwidth() / 2 - boardX * 30 / 2;
board.y = getheight() / 2 - boardY * 30 / 2;
```

> 学习点：**"窗口大小"是运行时才确定的量，一切位置都要用 `getwidth()/getheight()` 现算，不能写死**。

### 2.3 页面结构

```cpp
struct Page {
    std::string title;
    std::vector<TextButton> buttons;   // 一组按钮
    std::vector<TextInputBox> inputs;  // 一组输入框
    // ...
};
```

页面枚举 `PAGE_HOME=0 ... PAGE_PLAY=5`，`main.cpp` 里用指针数组把"枚举值"映射到"页面对象"，切页就是改一个 `pageState` 变量。

> 学习点：**用枚举 + 数组做"页面路由"**——新增页面只需加一个枚举值和一个页面对象，不用改主循环结构。

---

## 三、输入与交互

### 3.1 按钮

- 结构：`TextButton { label, rect, enable }`
- **点击判定**：鼠标点是否落在矩形内（`inRect` 函数），且只响应 `enable=true` 的按钮
- **宽自适应**：按钮宽度 = 文字宽度 + 左右留白，不是写死的像素

```cpp
int btnWidth = textwidth(label) + BTN_PAD_X * 2;  // 文字多宽，按钮多宽
```

> 学习点：**用 `textwidth()` 量文字宽度再算布局**，中英文混排也不会挤。

### 3.2 输入框（自定义棋盘大小）

- 结构：`TextInputBox { label, text, cursorIndex, enable }`
- **只收数字**：键盘输入时过滤非数字字符
- **最多 2 位**：输入第 3 个数字直接忽略
- **失焦自动钳制**：取消选择（失焦）时，把超出范围的值改回合法范围写回输入框

> 踩坑（改了三版才定稿）：
> 1. ❌ 弹窗提示修正 —— 用户不要弹窗
> 2. ❌ 按确认才修正 —— 用户要求在"取消选择"时修正
> 3. ✅ 最终方案：**失焦即修正，只修"刚失焦的那个框"**；空框不修正；另一个还没输入的框不动

失焦有两种触发：**按回车**、**点击另一个输入框**。用一个 `lostFocusIdx` 记录"刚失焦的框是哪个"，处理完置回 -1（防止重复处理）。

```cpp
void clampInputSize(Page& pg) {
    int idx = pg.lostFocusIdx;
    pg.lostFocusIdx = -1;              // 消费掉这次失焦事件
    if (idx < 0) return;               // 没有失焦 → 不动
    if (pg.inputs[idx].text.empty()) return;  // 空框 → 不动
    int val = stoi(pg.inputs[idx].text);
    if (val < 5)  val = 5;             // 钳制到 5~30
    if (val > 30) val = 30;
    pg.inputs[idx].text = to_string(val);  // 写回，用户立刻看到
}
```

> 学习点：**"事件只处理一次"**——处理完把标记清掉（消费事件），防止同一事件被重复响应。这是游戏/UI 编程的常见模式。

---

## 四、棋盘与棋子

### 4.1 用数组建模棋盘

```cpp
int g_board[31][31];  // 值：0空 / 1黑 / 2白
```

- **"落子"就是给数组元素赋值**：`g_board[row][col] = g_turn`
- **"读棋盘"就是遍历数组**

> 关键概念：30 路棋盘有 **31 个交叉点**（0~30）。之前落子代码用 `< boardX` 导致最右/最下路下不了棋，改成 `<=` 并扩数组到 31×31 才修好。
>
> 学习点：**遍历边界和落子边界要一致**——"能下棋的范围"和"绘制遍历的范围"必须是同一个数。

### 4.2 棋子怎么画（两种画法）

```cpp
// 黑子：白底 + 黑心（先画大圆打底，再画小圆盖中心）
setfillcolor(WHITE); solidcircle(x, y, 13);
setfillcolor(BLACK); solidcircle(x, y, 11);

// 白子：白色实心圆
setfillcolor(WHITE); solidcircle(x, y, 13);
```

- **实心圆**：`solidcircle`（填色）
- **空心圆**：`circle`（只画边框）

> 学习点：**"白边黑心" = 两个圆叠加**。EasyX 里画复杂图形常用"先大后小叠加"的思路。

### 4.3 执棋提示"画一个棋子"

对局页顶部提示"黑方执棋"，后面跟一个当前执棋方的棋子：

```cpp
int cx = tipX + textwidth(tip) + 15;      // 棋子 = 文字右缘 + 15
int cy = 150 + textheight(tip) / 2;       // 垂直方向跟文字对齐
if (g_turn == 1) { /* 画黑子 */ } else { /* 画白子 */ }
```

> 学习点：**图形元素的位置靠其他元素的位置推算**（文字右缘 → 棋子），不写死坐标，窗口怎么变都对。

---

## 五、游戏规则

### 5.1 五子棋：四方向连五

方向数组是核心技巧——**一个数组描述 4 种"走法"**：

```cpp
const int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };  // 竖、横、两斜
```

从刚落子的点出发，正反两个方向数同色棋子，`cnt >= 5` 就赢。正反方向合成一个循环（`s` 从 -30 到 30，跳过 0）更简洁。

> 学习点：**方向数组**（dx/dy 数组）是所有棋盘游戏（走迷宫、贪吃蛇、象棋）的通用套路。

### 5.2 井字棋：三连 + 平局

- 行、列、两条对角线各判断一次三连
- **平局**：9 格下满还没有人赢 → 判平

### 5.3 围棋：气 / 吃子 / 提子 / 自杀（本项目精华）

#### (1) 什么是"气"

一颗棋上下左右相邻的**空点**叫它的"气"。气 = 0 时被吃。

#### (2) 算气：递归 + 双标记数组

```cpp
int countAir(int row, int col, bool visited[31][31], bool airMark[31][31])
{
    visited[row][col] = true;    // 标记"这个同色点走过了"
    int air = 0;
    for (int k = 0; k < 4; k++) {
        int nr = row + dr[k], nc = col + dc[k];
        if (!inGo(nr, nc)) continue;              // 棋盘外不算气
        if (g_board[nr][nc] == 0 && !airMark[nr][nc]) {  // 空点=1口气
            airMark[nr][nc] = true;               // 防止同一空点算两次
            air++;
        }
        else if (同色 && !visited[nr][nc])
            air += countAir(nr, nc, visited, airMark);   // 递归进同色邻居
    }
    return air;
}
```

> 为什么需要**两个**标记数组？
> - `visited`：防止**同色棋子**被重复递归（A→B→A 死循环）
> - `airMark`：防止**同一个空点**被多个棋子各算一次（一组棋共用一口"共气"）
>
> 学习点：**递归遍历"连通区域"（洪水填充 Flood Fill）**是图论经典算法，围棋、扫雷、画图工具填充都用它。

#### (3) 吃子：先判定、后统一提（重要规则）

> 踩坑：早期实现"边判定边提"——第一组被提掉后，棋盘多了空位，会让**本来也无气的第二组"起死回生"**而被漏提。

**围棋规则：同时无气的敌方组要一起提。** 正确做法两阶段：

```cpp
int goEat(int row, int col) {
    // 阶段一：先判定（棋盘还没动），标记四个方向哪些敌方组无气
    bool eatDir[4] = {};
    for (int k = 0; k < 4; k++)
        if (inGo(nr, nc) && g_board[nr][nc] == enemy && groupAir(nr, nc) == 0)
            eatDir[k] = true;
    // 阶段二：统一提掉（同组只提一次：提完就不再有 enemy 棋子）
    for (int k = 0; k < 4; k++)
        if (eatDir[k] && g_board[nr][nc] == enemy)
            total += removeGroup(nr, nc, enemy);
    return total;
}
```

> 学习点：**"先收集，再执行"**——判断阶段和修改阶段分开，避免"修改影响判断"。这类 bug 极难肉眼发现，是写规则引擎的大坑。

#### (4) 提子计数：让函数把数量"带回来"

> 踩坑：早期 `removeGroup` 只清空棋盘、不返回数量，外层 `total++` 每提一组只 +1——**一次吃掉 3 颗只算 1**。

修复：`removeGroup` 改成返回 `int`，递归每到一颗 +1：

```cpp
int removeGroup(int row, int col, int enemy) {
    if (g_board[row][col] != enemy) return 0;
    g_board[row][col] = 0;
    int n = 1;                                     // 这颗算一个
    for (int k = 0; k < 4; k++)
        if (inGo(nr, nc)) n += removeGroup(nr, nc, enemy);
    return n;
}
// 调用处：
total += removeGroup(nr, nc, enemy);   // 按真实子数累加
```

> 学习点：**函数能算出来的数据，就从返回值带出来**。用 `++` 猜数量 = 让外层重复算一遍已经算过的东西。

#### (5) 自杀禁手

落子后：没吃到对方 且 自己这组气 = 0 → 自杀，**撤销落子**、不换手：

```cpp
int eaten = goEat(row, col);
if (eaten == 0 && groupAir(row, col) == 0) {
    g_board[row][col] = 0;      // 撤销
    return false;
}
```

#### (6) 边缘提子：游戏边界 ≠ 存储边界（经典坑）

> 踩坑：`g_board` 是 31×31（为五子棋 30 路预留），围棋只用 0~18。早期 `countAir` 用 `31` 当边界，边缘棋子的邻居探到棋盘外（数组里是 0），**0 被当成"空点=气"** → 贴边的棋子永远有气、永远提不掉。

修复：定义**游戏自己的边界常量**：

```cpp
static const int GO_EDGE = 18;   // 围棋 18 格 = 19 路交叉点（0~18）

bool inGo(int r, int c) { return r >= 0 && r <= GO_EDGE && c >= 0 && c <= GO_EDGE; }
```

气、提子、吃子、落子**全部**用它判断。

> 学习点：**逻辑边界必须用"游戏的尺寸"，不是"存储的尺寸"**。数组开大是为了兼容别的模式，多出来的区域全是 0，会伪装成"空气"。

#### (7) 胜负：先提满 10 子胜（简化规则）

```cpp
if (g_blackCap >= GO_WIN) return 1;   // GO_WIN = 10
if (g_whiteCap >= GO_WIN) return 2;
```

### 5.4 统一落子入口

```cpp
bool putChess(int col, int row) {
    // 越界/占用/模式边界检查 → 放子
    if (g_game == 2 && !putGo(row, col)) return false;  // 围棋分支
    g_turn = (g_turn == 1) ? 2 : 1;   // 换手
    return true;
}
```

> 学习点：**把"模式差异"收敛到一个小函数里**，主流程（落子→判定→换手）三种模式共用一条。

---

## 六、代码质量与精简封装

### 6.1 重复表达式收进命名函数

```cpp
// 边界判断出现 3 次 → 收成一个函数，改一处全生效
bool inGo(int r, int c) { return r >= 0 && r <= GO_EDGE && c >= 0 && c <= GO_EDGE; }
```

> 学习点：**"一处定义，处处引用"**。改逻辑时只改一个地方，不会漏。

### 6.2 公共常量数组

```cpp
const int dr[4] = { -1, 1, 0, 0 };   // 上下左右
const int dc[4] = { 0, 0, -1, 1 };
```

几个函数共用，不再各自重复定义。

### 6.3 便捷封装：给复杂函数套一层"好用的壳"

```cpp
// 底层函数要外部传两个 31×31 标记数组，调用方不关心这些
int groupAir(int r, int c) {
    bool visited[31][31] = {}, airMark[31][31] = {};
    return countAir(r, c, visited, airMark);
}
// 调用方只需要问："这组几口气？" 一行搞定
if (groupAir(nr, nc) == 0) ...
```

### 6.4 职责拆分

把"落子函数"里的围棋分支抽成独立函数 `putGo`，`putChess` 恢复成一行调用。

> 学习点：**一个函数只做一件事**。`putChess` 管"落子总流程"，`putGo` 管"围棋特殊规则"，各自能独立看懂、独立测试。

### 6.5 精简的原则

- 精简的是**写法**（重复、啰嗦），不是**逻辑**（规则正确性不能砍）
- 两阶段吃子（eatDir）不能精简掉——那是围棋规则的要求

---

## 七、Git 与 GitHub 开源

### 7.1 一次完整的发布流程

```bash
# 1. 初始化（-b main 指定主分支名，和 GitHub 默认一致）
git init -b main

# 2. 暂存 → 查看 → 提交
git add .
git status          # 红色 = 未暂存；绿色 = 已暂存
git commit -m "说明改了什么"

# 3. 关联远程仓库并推送
git remote add origin https://github.com/用户名/仓库名.git
git push -u origin main
```

- `-u`：记住远程分支，以后直接 `git push`
- `main`（不是 master）：GitHub 新仓库默认分支

### 7.2 .gitignore：哪些文件不该进仓库

**该忽略的**：编译产物（x64/、Debug/、Release/、.obj、.pdb）、IDE 缓存（.vs/）、本机配置（*.user）、打包产物（安装包/）。

```gitignore
.vs/
*.user
Debug/
Release/
x64/
*.obj
*.pdb
安装包/
```

> ⚠️ **重要坑：gitignore 不支持行尾注释！**
> ```gitignore
> *.user   # 这样写整行都变成"模式"，# 不是注释！规则失效！
> ```
> `#` 只有**在行首**才是注释。注释要单独占一行：
> ```gitignore
> # 本机配置，不入库
> *.user
> ```

> 学习点：**提交前先 `git status` 看一眼**有没有不该进的文件（构建产物、密钥、本机路径）。

### 7.3 git rm --cached：后悔药（只移出仓库，保留本地文件）

误提交了文件怎么办：

```bash
git rm --cached "五子棋注释学习版.vcxproj.user"
git commit -m "移除本机配置"
```

文件还在磁盘上，只是不再被版本控制。

### 7.4 开源仓库三件套

- **README.md**：项目门面——是什么、怎么编译、怎么用
- **LICENSE**：许可证（法律约定）。MIT 最宽松：任何人可自由使用/修改/商用，只需保留版权声明
- **.gitignore**：保证仓库干净

> 注意：**不加 LICENSE 时法律默认"保留所有权利"**，别人不能合法使用你的代码。

### 7.5 用 GitHub 网页/工具创建仓库

- 网页：New repository → 填名字 → Public → Create
- 命令行工具 `gh`：`gh repo create 仓库名 --public --source . --push`

---

## 八、打包安装包（Inno Setup）

### 8.1 为什么用 Inno Setup

- 免费、轻量、最主流；一个脚本（.iss）编译出一个 .exe 安装程序
- 支持中文界面（需下载 `ChineseSimplified.isl` 语言文件放入 Languages 目录）

### 8.2 安装脚本的六大段（每个段管一类事）

```ini
[Setup]     # 安装包自身属性：名字/版本/目录/压缩/AppId
[Languages] # 界面语言
[Tasks]     # 安装时的可选项（如"创建桌面快捷方式"，默认不打勾）
[Files]     # 往安装目录放哪些文件（{app} = 用户选的安装目录）
[Icons]     # 快捷方式（开始菜单 / 桌面）
[Run]       # 安装完成后做什么（"立即运行"）
```

### 8.3 AppId：安装程序的"身份证"

```ini
AppId={{6AF43B00-8A06-4959-A92C-396432C2322D}
```

- **定下后不要改**：卸载、升级靠它识别
- 升级版本（改 AppVersion）保持 AppId 不变 → 旧版本上直接覆盖升级

### 8.4 打包前检查依赖

```bash
dumpbin /dependents 你的程序.exe
```

本项目只依赖 `KERNEL32/USER32/SHELL32`（系统自带）→ **零外部依赖**，打包只需一个 exe。

### 8.5 静默测试安装包（交付前验证）

```bash
# 静默安装到临时目录 → 检查文件 → 运行 → 卸载
setup.exe /VERYSILENT /DIR="C:\Temp\Test"
& "$dir\unins000.exe" /VERYSILENT   # 用卸载程序清理
```

> 学习点：**交付前用"可能失败的动作"验证**——安装、启动、卸载全跑一遍，而不是只看"文件生成了"。

### 8.6 打包命令

```bash
ISCC setup.iss    # 输出到 OutputDir 指定的目录
```

---

## 九、GitHub Releases 发布

### 9.1 一条命令发 Release（含安装包）

```bash
gh release create v1.0.0 "安装包\Gomoku-Setup-1.0.0.exe" \
  --title "五子棋注释学习版 v1.0.0" \
  --notes "更新说明..."
```

它会：打标签 v1.0.0 → 创建 Release → 上传安装包。别人在仓库 **Releases** 页就能下载。

### 9.2 验证与修复

```bash
gh release view v1.0.0 --json assets        # 看资产
gh release delete-asset v1.0.0 "坏文件" -y   # 删资产
gh api -X DELETE "repos/用户/仓库/releases/assets/资产ID"  # API 强删
gh release upload v1.0.0 "新文件"            # 重新上传
```

### 9.3 中文文件名坑（Windows 通病）

> ⚠️ `gh release create` 上传**中文文件名**时，Windows 控制台编码会把中文剥离，资产名变成 `-Setup-1.0.0.exe`。
> **对策：给别人下载的文件一律用英文名**（`Gomoku-Setup-1.0.0.exe`）。

---

## 十、踩坑记录汇总

| # | 坑 | 原因 | 修复 |
|---|---|---|---|
| 1 | 窗口拖拽后切回全屏画面歪 | 没做 Resize 重绘 | 尺寸变化时重算棋盘/按钮位置并重绘 |
| 2 | 最右/最下行无法落子 | 落子边界 `< boardX` | 改 `<=`，数组扩 31×31 |
| 3 | 开始自动落一子 | 切页时把确认点击当落子 | 加 `hitIdx < 0` 条件 |
| 4 | 输入超限弹窗/确认才钳制 | 时机不对 | 失焦即钳制，只修刚失焦的框，空框不修 |
| 5 | 一次提子多个只算一个 | removeGroup 不返回数量 | 改返回 int，`total += removeGroup(...)` |
| 6 | 提子数仍不对（多组漏提） | 边判定边提，先提的组影响后判的组 | 两阶段：先判定后统一提 |
| 7 | 边缘棋子永远提不掉 | 用数组大小 31 当边界，棋盘外 0 被当气 | 加 GO_EDGE=18，全链路用 inGo |
| 8 | .gitignore 规则失效 | 行尾写了注释（gitignore 不支持） | 注释独立成行 |
| 9 | vcxproj.user 误提交 | 没排除 *.user | git rm --cached 移出 |
| 10 | Release 资产中文名被剥离 | Windows 控制台编码 | 资产用英文文件名 |

---

## 十一、常用命令速查

### Git
```bash
git init -b main          # 初始化
git add .                 # 暂存全部
git status                # 查看状态（红=未暂存 绿=已暂存）
git commit -m "说明"      # 提交
git push                  # 推送
git log --oneline         # 提交历史
git rm --cached 文件      # 移出仓库保留本地
```

### 构建
```bash
MSBuild.exe 工程.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64
```

### 打包
```bash
ISCC setup.iss
```

### 发布
```bash
git add . && git commit -m "v1.1.0 ..." && git push
gh release create v1.1.0 "安装包\Gomoku-Setup-1.1.0.exe" --title "..." --notes "..."
```

---

*笔记随项目更新。改完代码/踩了新坑，记得回来补充。*
