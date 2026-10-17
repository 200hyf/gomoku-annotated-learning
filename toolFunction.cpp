//toolFunction.cpp
#include <graphics.h>
#include <windows.h>
#include <string>
#include "connect.h"
using namespace std;

// ===================== 对局数据（定义） =====================
int g_board[31][31] = { 0 };  // 棋盘状态：0=空 1=黑子 2=白子（31×31：30 路棋盘有 31 个交叉点）
int g_turn = 1;               // 当前该谁下：1=黑先 2=白
int g_mode = 1;               // 输入页模式：1=模板（预设大小）2=自定义（手动输入）
int g_game = 1;               // 游戏模式：1=五子棋 2=围棋 3=井字棋
int g_winner = 0;             // 胜负：0=进行中 1=黑胜 2=白胜 3=平局
int g_blackCap = 0;           // 黑方提子数（围棋：黑提掉的白子数）
int g_whiteCap = 0;           // 白方提子数（围棋：白提掉的黑子数）

// 围棋四方向偏移（上下左右），下面几个函数共用
const int dr[4] = { -1, 1, 0, 0 };
const int dc[4] = { 0, 0, -1, 1 };

// 点 (r,c) 是否在围棋棋盘内（19 路交叉点编号 0~GO_EDGE）
bool inGo(int r, int c) { return r >= 0 && r <= GO_EDGE && c >= 0 && c <= GO_EDGE; }

// 计算 (row,col) 所在"同色连通组"的气数（相邻空点数）
// visited = 已走过的同色点；airMark = 已计过数的空点（防止一个空点被算两次）
int countAir(int row, int col, bool visited[31][31], bool airMark[31][31])
{
	visited[row][col] = true;
	int air = 0;
	for (int k = 0; k < 4; k++)
	{
		int nr = row + dr[k], nc = col + dc[k];
		if (!inGo(nr, nc)) continue;                       // 棋盘外不算气
		if (g_board[nr][nc] == 0 && !airMark[nr][nc])      // 空点 = 一口气
		{
			airMark[nr][nc] = true;    // 标记已计入，防重复
			air++;
		}
		else if (g_board[nr][nc] == g_board[row][col] && !visited[nr][nc])
			air += countAir(nr, nc, visited, airMark);     // 同色邻居：递归统计
	}
	return air;
}

// 便捷入口：只问"这一组有几口气"（内部自动开标记数组）
int groupAir(int r, int c)
{
	bool visited[31][31] = {}, airMark[31][31] = {};
	return countAir(r, c, visited, airMark);
}

// 提掉 (row,col) 所在 enemy 色整组棋子，返回提掉的数量
int removeGroup(int row, int col, int enemy)
{
	if (g_board[row][col] != enemy) return 0;   // 不是敌人就停
	g_board[row][col] = 0;                      // 吃掉：置为空
	int n = 1;                                  // 这一颗算一个
	for (int k = 0; k < 4; k++)                 // 四方向循环递归，清掉整组并数颗数
	{
		int nr = row + dr[k], nc = col + dc[k];
		if (inGo(nr, nc)) n += removeGroup(nr, nc, enemy);
	}
	return n;                                   // 返回这一组总共几颗
}

// 围棋吃子：把落子四邻"无气"的敌方组提掉，返回提子数
// 先判定、后统一提：同时无气的组必须一起提（边判边提会让第二组"起死回生"而漏提）
int goEat(int row, int col)
{
	int enemy = (g_board[row][col] == 1) ? 2 : 1;   // 落的是黑就吃白，落白就吃黑
	bool eatDir[4] = {};                            // 阶段一：标记哪个方向要提
	for (int k = 0; k < 4; k++)
	{
		int nr = row + dr[k], nc = col + dc[k];
		if (inGo(nr, nc) && g_board[nr][nc] == enemy && groupAir(nr, nc) == 0)
			eatDir[k] = true;
	}
	int total = 0;
	for (int k = 0; k < 4; k++)                     // 阶段二：统一提（同组只提一次）
	{
		int nr = row + dr[k], nc = col + dc[k];
		if (eatDir[k] && g_board[nr][nc] == enemy)
			total += removeGroup(nr, nc, enemy);
	}
	return total;                                   // 返回这手吃掉的子数
}

// 围棋落子后的处理：吃子 + 防自杀，返回这手是否有效
bool putGo(int row, int col)
{
	int eaten = goEat(row, col);                    // 先看能不能吃对方
	if (eaten == 0 && groupAir(row, col) == 0)      // 没吃到且自己无气 = 自杀禁手
	{
		g_board[row][col] = 0;                      // 撤销落子
		return false;
	}
	if (g_turn == 1) g_blackCap += eaten;           // 黑方吃子数累计
	else             g_whiteCap += eaten;           // 白方吃子数累计
	return true;
}

// 落子：在 (col,row) 放当前该下的棋子，成功后换手
bool putChess(int col, int row)
{
	if (col < 0 || row < 0 || col >= 31 || row >= 31) return false;   // 越界：拒绝
	if (g_board[row][col] != 0) return false;                         // 已占用：拒绝
	if (g_game == 3 && (col >= 3 || row >= 3)) return false;          // 井字棋只有 3×3 格
	if (g_game == 2 && (col > GO_EDGE || row > GO_EDGE)) return false; // 围棋只到 19 路
	g_board[row][col] = g_turn;              // 放当前颜色的子（1=黑 2=白）
	if (g_game == 2 && !putGo(row, col)) return false;   // 围棋：吃子/自杀处理失败则整手作废
	g_turn = (g_turn == 1) ? 2 : 1;          // 换手：三目运算符，真取 2、假取 1
	return true;
}

// 胜负判定：落子在 (row,col) 后检查，返回 1黑胜/2白胜/3平/0未分
int checkWin(int row, int col)
{
	int player = g_board[row][col];          // 刚落的子是谁下的
	if (g_game == 1)                         // —— 五子棋：四条线数连续 5 个
	{
		const int dirs[4][2] = { {1,0},{0,1},{1,1},{1,-1} };  // 竖、横、两斜
		for (int k = 0; k < 4; k++)
		{
			int cnt = 1;                     // 已经有一颗（刚落的这颗）
			for (int s = -30; s <= 30; s++)  // 正反两个方向一起数（跳过自己）
			{
				if (s == 0) continue;
				int nr = row + dirs[k][0] * s, nc = col + dirs[k][1] * s;
				if (nr < 0 || nr >= 31 || nc < 0 || nc >= 31 || g_board[nr][nc] != player) continue;
				cnt++;
			}
			if (cnt >= 5) return player;     // 连续 5 个 → 这方赢
		}
	}
	else if (g_game == 3)                    // —— 井字棋：所在行/列/两对角线三连
	{
		if (g_board[row][0] == player && g_board[row][1] == player && g_board[row][2] == player) return player;
		if (g_board[0][col] == player && g_board[1][col] == player && g_board[2][col] == player) return player;
		if (g_board[0][0] == player && g_board[1][1] == player && g_board[2][2] == player) return player;
		if (g_board[0][2] == player && g_board[1][1] == player && g_board[2][0] == player) return player;
	}
	// —— 围棋：提子数先到目标即胜（简化规则）
	if (g_game == 2 && g_blackCap >= GO_WIN) return 1;
	if (g_game == 2 && g_whiteCap >= GO_WIN) return 2;
	return 0;                                // 还没分出胜负
}

// 输入框"取消选择"(失焦)时调用：只修正"刚失焦的那个框"，越界值钳制到 5~30 并写回
// 失焦有两种：按回车、点击另一个输入框。空框不修正；刚聚焦的框（可能还没输入）不动
void clampInputSize(Page& pg)
{
	int idx = pg.lostFocusIdx;   // 取出"刚失焦的框"下标
	pg.lostFocusIdx = -1;        // 消费掉这次失焦事件（防止点按钮时重复处理）
	if (g_mode != 2) return;     // 模板模式没有输入框，跳过
	if (idx < 0 || idx >= (int)pg.inputs.size()) return;   // 没有失焦的框 → 什么都不做
	if (pg.inputs[idx].text.empty()) return;   // 空框不修正（没有输入 = 不需要修正）
	// 读这个框的值，越界钳制到 5~30，再写回（用户立刻看到修正）
	int val = stoi(pg.inputs[idx].text);
	if (val < 5) val = 5;
	if (val > 30) val = 30;
	pg.inputs[idx].text = to_string(val);
	pg.inputs[idx].cursorIndex = (int)pg.inputs[idx].text.size();
}

// 清空棋盘（新开一局）
void resetBoard()
{
	for (int i = 0; i < 31; i++)
		for (int j = 0; j < 31; j++)
			g_board[i][j] = 0;
	g_winner = 0;                // 胜负状态清零
	g_blackCap = g_whiteCap = 0; // 提子数清零
}

// 判断点是否在矩形内(通用)
bool inRect(int x, int y, int rx, int ry, int rw, int rh)
{                                     // 6 个参数：点坐标 + 矩形左上角 + 宽高
	return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
	// && = 并且。判断：x 在 [rx, rx+rw] 之间 且 y 在 [ry, ry+rh] 之间
	// 满足就返回 true（在矩形内），否则 false
}

// 绘制文字按钮，悬浮高亮
void drawButton(coordinates judp, TextButton& btn)
{
	const char* pText = btn.text.c_str();      // string → C 风格字符串（EasyX 画字要这种）
	settextstyle(btn.fontSize, 0, btn.font);   // 用按钮自己的字体（addButton 没传就=UI_FONT）
	setbkmode(TRANSPARENT);                    // 文字背景透明，不盖住底色
	btn.w = textwidth(pText) + 2 * BTN_PAD_X;  // 按钮宽自适应 = 文字宽 + 左右内边距
	btn.h = textheight(pText) + 2 * BTN_PAD_Y; // 按钮高自适应 = 文字高 + 上下内边距
	if (btn.enable && inRect(judp.x, judp.y, btn.x, btn.y, btn.w, btn.h))   // 悬停 → 高亮
		settextcolor(btn.hoverColor);
	else
		settextcolor(btn.normalColor);
	outtextxy(btn.x, btn.y, pText);             // 在按钮左上角画出文字
}

// 绘制文本输入框 + 闪烁光标（blink = 这一帧光标该不该亮）
void drawTextBox(coordinates judp, TextInputBox& box, bool blink)
{
	// 聚焦态的高亮颜色集中在这里：想改高亮样式就改这两行
	if (box.isFocus) { setcolor(LIGHTGRAY); setfillcolor(BLACK); }
	else             { setcolor(box.border); setfillcolor(box.bg); }
	fillrectangle(box.x, box.y, box.x + box.w, box.y + box.h);   // 画实心矩形（左上角→右下角）

	// ===== 字号与偏移自适应 =====
	int fontSize = box.h / 2;                    // 字号 = 框高的一半（框变大字自动变大）
	settextstyle(fontSize, 0, UI_FONT);
	setbkmode(TRANSPARENT);
	int padX = fontSize / 2;                     // 水平内边距 = 字号一半（随字号变）
	int textH = textheight("0");                 // 测当前字号下"文字高度"
	int padY = (box.h - textH) / 2;              // 垂直内边距 = 让文字在框内垂直居中
	if (box.text.empty() && !box.label.empty())   // 框是空的且有占位提示 → 显示浅灰提示
	{
		settextcolor(LIGHTGRAY);
		outtextxy(box.x + padX, box.y + padY, box.label.c_str());
	}
	else                                          // 否则显示实际输入的文字
	{
		settextcolor(box.textColor);
		outtextxy(box.x + padX, box.y + padY, box.text.c_str());
	}

	// ===== 闪烁光标：位置和长度都自适应 =====
	if (box.isFocus && blink)
	{
		int curX = box.x + padX + textwidth(box.text.substr(0, box.cursorIndex).c_str());
		// substr(0, cursorIndex) = 截取"光标前"的子串，测它的宽度 → 光标跟着文字末尾走
		line(curX, box.y + padY, curX, box.y + padY + textH);   // 光标长度 = 文字高度
	}
}

// 键盘输入处理
void handleInput(TextInputBox& box, char ch)
{
	if (!box.isFocus) return;           // 没选中这个框 → 直接返回，什么都不做

	// 退格
	if (ch == VK_BACK)                  // VK_BACK = 退格键的虚拟键码
	{
		if (box.cursorIndex > 0)          // 光标前面还有字才能删
		{
			box.text.erase((size_t)(box.cursorIndex) - 1, 1);
			// erase(位置, 个数)：删掉光标前 1 个字符；(size_t) 转成无符号防警告
			box.cursorIndex--;            // 光标往前挪一位
		}
		return;
	}
	// 左右方向键移动光标
	if (ch == VK_LEFT)
	{
		if (box.cursorIndex > 0) box.cursorIndex--;    // 左移，但不能小于 0
		return;
	}
	if (ch == VK_RIGHT)
	{
		if (box.cursorIndex < (int)box.text.size()) box.cursorIndex++;  // 右移，不能超过长度
		return;
	}
	// 只接收数字字符，且最多 2 位（棋盘大小范围 5~30，最大是两位数）
	if (ch >= '0' && ch <= '9')          // 字符比较：'0'~'9' 就是数字键
	{
		if (box.text.size() >= 2) return;   // 已经 2 位 → 忽略新输入（比如 100 的第 3 位输不进去）
		box.text.insert(box.cursorIndex, 1, ch);   // 在光标位置插入 1 个字符
		box.cursorIndex++;                         // 光标后移
	}
}

// 棋盘绘制
void drawMap(mup d)
{
	for (int i = 0; i <= d.x_num; i++)        // 竖线：列数+1 条
		line(d.x + i * d.size, d.y, d.x + i * d.size, d.y + d.y_num * d.size);
	// line(起点x, 起点y, 终点x, 终点y)：第 i 条竖线从 (棋盘x + i*边长, 上边) 画到 (同x, 下边)
	for (int j = 0; j <= d.y_num; j++)        // 横线：行数+1 条
		line(d.x, d.y + j * d.size, d.x + d.x_num * d.size, d.y + j * d.size);
	// 第 j 条横线从 (左边, y + j*边长) 画到 (右边, 同y)
}

// 画棋子：黑子 = 实心圆(白边黑心)，白子 = 空心圆
void drawChesses(mup d)
{
	int r = d.size / 2 - 2;                 // 棋子半径：比格子一半略小
	for (int row = 0; row <= d.y_num; row++) // 遍历所有行（含最下行交叉点）
		for (int col = 0; col <= d.x_num; col++)   // 遍历所有列（含最右列交叉点）
		{
			if (g_board[row][col] == 0) continue;     // 空位置跳过
			int cx = d.x + col * d.size;              // 交叉点 x（五子棋/围棋落在交叉点）
			int cy = d.y + row * d.size;              // 交叉点 y
			if (g_game == 3) { cx += d.size / 2; cy += d.size / 2; }   // 井字棋落在格子中心
			if (g_board[row][col] == 1)               // 黑子：实心圆
			{
				setfillcolor(WHITE);  solidcircle(cx, cy, r);     // 先画白底（黑背景上才看得见）
				setfillcolor(BLACK);  solidcircle(cx, cy, r - 2); // 再画黑心 → 黑子带一圈白边
			}
			else                                     // 白子：填充白色实心圆
			{
				setfillcolor(WHITE);  solidcircle(cx, cy, r);
			}
		}
}

// 无边框窗口(全屏/全屏窗口化)：去掉标题栏与缩放边框，固定到屏幕左上角铺满
void RenderscreenNoBorder(int screen_x, int screen_y)
{
	HWND hWnd = initgraph(screen_x, screen_y);
	// initgraph = EasyX 建窗口，返回 HWND（句柄 = Windows 给窗口发的"身份证号"）
	LONG style = static_cast<LONG>(GetWindowLongPtr(hWnd, GWL_STYLE));
	// 取出窗口当前样式。样式 = 一堆开关标志位。LONG = Windows 的整型
	// static_cast<LONG> = 显式类型转换（把 64 位转成 32 位整型）
	style &= ~(WS_CAPTION | WS_THICKFRAME);
	// ~ = 按位取反；| = 按位或；&= 按位与
	// 意思：把"标题栏"和"缩放边框"两个标志关掉 → 变成无边框窗口
	SetWindowLongPtr(hWnd, GWL_STYLE, (LONG_PTR)style);
	// 把改好的样式写回窗口
	SetWindowPos(hWnd, NULL, 0, 0, screen_x, screen_y,
		SWP_NOZORDER | SWP_FRAMECHANGED);
	// 把窗口移到屏幕左上角(0,0)并设成指定大小 → 铺满全屏
	// SWP_NOZORDER=不动层级；SWP_FRAMECHANGED=让边框按新样式重绘
}

// 屏幕初始化(全屏)
void Renderscreen()
{
	RenderscreenNoBorder(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	// 无参版 = 问系统要屏幕分辨率，然后调用无边框版 → 启动时就是全屏
}

// 屏幕初始化重载(分屏/窗口化)：补全窗口样式，标题栏按钮与自由缩放全部可用
void Renderscreen(int x, int y)
{
	HWND hWnd = initgraph(x, y);
	LONG style = static_cast<LONG>(GetWindowLongPtr(hWnd, GWL_STYLE));
	style |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
	// |= 按位或 = 把标题栏、系统菜单、最小化、最大化、缩放边框这些标志"补上"
	// —— EasyX 默认窗口缺"最大化"和"缩放"，所以之前按钮点了没反应，这里就是修复
	SetWindowLongPtr(hWnd, GWL_STYLE, (LONG_PTR)style);
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	// 尺寸不动的 SetWindowPos：只是强制刷新边框，让标题栏按钮立刻生效
}

// ===================== 窗口切换封装 =====================
static bool g_windowed = false;   // static = 只在本文件内可见的全局变量
// g_windowed：当前是不是"带标题栏的窗口化模式"
static int  g_winW = 0, g_winH = 0; // 当前绘图缓冲(窗口客户区)尺寸
static int  g_preW = -1, g_preH = -1; // 上一帧观测到的尺寸（消抖：防止拖拽中反复重建）

// 统一切换窗口：borderless=true 无边框全屏，false 带标题栏窗口化
void SwitchScreen(bool borderless, int w, int h)
{
	closegraph();                     // ① 销毁旧窗口（EasyX 内部状态也一起清掉）
	if (borderless) RenderscreenNoBorder(w, h);   // ② 按模式建新窗口
	else            Renderscreen(w, h);
	g_windowed = !borderless;         // ③ 记录当前模式
	g_winW = w; g_winH = h;           // ④ 记录新尺寸（CheckWindowSize 靠它判断变化）
	flushmessage();      // ⑤ 清掉窗口残留的旧消息，防止旧点击误触发
	BeginBatchDraw();    // ⑥ closegraph 会销毁批量绘图状态 → 必须重新开启
	initPages();         // ⑦ 按新窗口尺寸重建所有页面按钮位置
}

// 每帧检测：窗口被最大化/拖拽缩放后重建窗口，让画面跟着铺满
void CheckWindowSize()
{
	if (!g_windowed) return;            // 全屏模式缓冲=屏幕大小，不需要跟随
	HWND hwnd = GetHWnd();              // 取当前窗口句柄
	RECT rc;
	GetClientRect(hwnd, &rc);           // 取窗口"客户区"矩形（不含标题栏边框的区域）
	int cw = rc.right - rc.left, ch = rc.bottom - rc.top;   // 算出客户区宽高
	if (cw == g_winW && ch == g_winH)   // 尺寸没变 → 重置消抖记录，返回
	{
		g_preW = -1; g_preH = -1; return;
	}
	// 连续两帧尺寸相同(拖拽松手/最大化完成)才重建，避免拖拽过程中反复重建闪烁
	if (cw != g_preW || ch != g_preH)   // 尺寸变了但和上一帧不同 → 还在拖动中，先只记录
	{
		g_preW = cw; g_preH = ch; return;
	}
	bool zoomed = IsZoomed(hwnd);       // 记住窗口当前是否处于"最大化"状态
	closegraph();                       // 尺寸已稳定 → 销毁旧窗口
	Renderscreen(cw, ch);               // 按新尺寸重建（画面铺满新窗口）
	if (zoomed) ShowWindow(GetHWnd(), SW_MAXIMIZE);  // 之前最大化 → 重建后继续保持最大化
	g_winW = cw; g_winH = ch;           // 更新记录
	g_preW = -1; g_preH = -1;
	flushmessage();                     // 清残留消息
	BeginBatchDraw();                   // 重开批量绘图
	initPages();                        // 重建页面布局
}
