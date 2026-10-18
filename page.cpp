//page.cpp
#include <winsock2.h>       // 必须最先（与 windows.h 兼容），见 toolFunction.cpp 顶部说明
#include <graphics.h>
#include "connect.h"
using namespace std;

// ===================== 布局辅助 =====================
// 自适应按钮宽 = 文字宽 + 左右内边距（算法和 drawButton 完全一致，保证居中准确）
static int btnWidth(const char* s, int fontSize)
{
	settextstyle(fontSize, 0, UI_FONT);
	return textwidth(s) + 2 * BTN_PAD_X;
}
// 按钮水平居中时的 x 坐标
static int centerX(int winW, const char* s, int fontSize)
{
	return winW / 2 - btnWidth(s, fontSize) / 2;
}

// ===================== 全局页面 =====================
Page homePage;        // 主页
Page settingPage;     // 棋盘设置页
Page sysPage;         // 系统设置页
Page inputPage;       // 棋盘行列输入页
Page screenInputPage; // 屏幕尺寸输入页
Page play;            // 游戏界面
Page netPage;         // 联机对弈设置页（新）
// 这里才是 6 个页面对象的"真正创建"（connect.h 里的 extern 只是声明）
// 全局变量：程序一启动就存在，所有文件共享同一份

// ===================== 初始化所有页面 =====================
void initPages()                  // 构建所有页面的按钮和输入框
{
	// 每次调用前先清空，保证窗口尺寸变化后可以安全重建布局
	homePage.buttons.clear();          homePage.inputs.clear();
	settingPage.buttons.clear();       settingPage.inputs.clear();
	sysPage.buttons.clear();           sysPage.inputs.clear();
	inputPage.buttons.clear();         inputPage.inputs.clear();
	screenInputPage.buttons.clear();   screenInputPage.inputs.clear();
	play.buttons.clear();              play.inputs.clear();
	netPage.buttons.clear();           netPage.inputs.clear();
	// .clear() = 清空数组。切分辨率时这个函数会被重新调用，不清空会残留旧位置的按钮

	// 各页面标题（顶部居中显示）
	homePage.setTitle("来下棋呀喵喵喵");
	settingPage.setTitle("棋盘设置");
	sysPage.setTitle("系统设置");
	inputPage.setTitle("对局设置");
	screenInputPage.setTitle("屏幕尺寸");
	play.setTitle("对局中");
	netPage.setTitle("联机对弈（五子棋）");

	int w = getwidth(), h = getheight();   // 当前窗口宽高（切分辨率后这里自动变）
	int rowH = h / 12;                     // 行距 = 窗口高的 1/12（按钮高度按文字自适应，小于行距不重叠）

	// --- 主页：4个按钮 ---
	int startY = h / 4;               // 从窗口 1/4 高度开始放按钮
	homePage.addButton(centerX(w, "单机对弈", 40), startY, "单机对弈", 40,RED,LIGHTRED);
	homePage.addButton(centerX(w, "联机对弈", 40), startY + rowH, "联机对弈", 40, RED, LIGHTRED);
	homePage.addButton(centerX(w, "游戏设置", 40), startY + rowH * 3, "游戏设置", 40);
	homePage.addButton(centerX(w, "退出游戏", 40), startY + rowH * 5, "退出游戏", 40);
	// 示例：这个按钮平时红色、悬停亮红——多传了两个颜色参数，其他按钮不受影响
	// centerX = 按钮水平居中；startY + rowH*2 = 往下挪 2 个行距（中间留一个空行，更好看）

	// --- 棋盘设置页：2个输入框 + 1个返回 ---
	settingPage.addInput(w / 2 - 220, h / 3, 200, 50, false);
	settingPage.addInput(w / 2 + 20, h / 3, 200, 50, false);
	// 两个输入框一左一右并排；h / 3 = 窗口 1/3 高度处；200×50 = 框的宽高；false = 初始不聚焦
	settingPage.addButton(centerX(w, "返回主页", 40), h * 2 / 3, "返回主页", 40);   // 2/3 高度处

	// --- 系统设置页：2个按钮 ---
	sysPage.addButton(centerX(w, "屏幕尺寸", 40), h / 3, "屏幕尺寸", 40);
	sysPage.addButton(centerX(w, "返回主页", 40), h / 3 + rowH * 2, "返回主页", 40);

	// --- 棋盘输入页：游戏模式 + 模板/自定义 + 大小/输入 + 先后手 + 确认 + 返回 ---
	int irow = h / 9;                 // 输入页行距（内容多，比 h/12 稍大）
	int iy0 = h / 4 + 30;             // 第一行起点（标题下方留出空间）
	// 行1：游戏模式（五子棋 / 围棋 / 井字棋）三按钮并排，整组居中
	const char* gameTexts[] = { "五子棋", "围棋", "井字棋" };
	int gw[3], gTotal = 0;
	for (int i = 0; i < 3; i++) { gw[i] = btnWidth(gameTexts[i], 24); gTotal += gw[i]; }
	gTotal += 2 * 16;
	int gx = w / 2 - gTotal / 2;
	for (int i = 0; i < 3; i++)
	{
		inputPage.addButton(gx, iy0, gameTexts[i], 24);
		gx += gw[i] + 16;
	}
	// 行2：模板 / 自定义 并排，整组居中（仅五子棋模式显示）
	int mbw1 = btnWidth("模板", 24), mbw2 = btnWidth("自定义", 24);
	int mx1 = w / 2 - (mbw1 + mbw2 + 20) / 2;
	inputPage.addButton(mx1, iy0 + irow, "模板", 24);
	inputPage.addButton(mx1 + mbw1 + 20, iy0 + irow, "自定义", 24);
	// 行3：模板 = 4 个预设大小按钮；自定义 = 2 个输入框（同一行，按模式切换显示）
	const char* sizeTexts[] = { "5 × 5", "9 × 9", "13 × 13", "15 × 15" };
	int sw[4], totalW = 0;
	for (int i = 0; i < 4; i++) { sw[i] = btnWidth(sizeTexts[i], 20); totalW += sw[i]; }
	totalW += 3 * 12;
	int sx = w / 2 - totalW / 2;
	for (int i = 0; i < 4; i++)
	{
		inputPage.addButton(sx, iy0 + irow * 2, sizeTexts[i], 20);
		sx += sw[i] + 12;
	}
	inputPage.addInput(w / 2 - 220, iy0 + irow * 2 - 8, 200, 46, true, "列数(5~30)");
	inputPage.addInput(w / 2 + 20,  iy0 + irow * 2 - 8, 200, 46, false, "行数(5~30)");
	// 行4：先后手 并排，整组居中
	int bw1 = btnWidth("黑方先手", 24), bw2 = btnWidth("白方先手", 24);
	int x1 = w / 2 - (bw1 + bw2 + 20) / 2;
	inputPage.addButton(x1, iy0 + irow * 3, "黑方先手", 24);
	inputPage.addButton(x1 + bw1 + 20, iy0 + irow * 3, "白方先手", 24);
	// 行5：确认 / 行6：返回
	inputPage.addButton(centerX(w, "确认", 24), iy0 + irow * 4, "确认", 24);
	inputPage.addButton(centerX(w, "返回主页", 24), iy0 + irow * 5, "返回主页", 24);

	// --- 屏幕尺寸选择页：7个按钮，整组垂直居中 ---
	const char* scTexts[] = { "全屏", "全屏窗口化", "1680 × 890", "1280 × 720", "1024 × 768", "640 × 480", "返回" };
	// const char* = C 风格字符串；[] 数组 = 7 个按钮文字
	int scNum = (int)(sizeof(scTexts) / sizeof(scTexts[0]));
	// 数组元素个数 = 整个数组占的字节数 ÷ 一个元素占的字节数 → 7
	int scY = h / 2 - scNum * rowH / 2;   // 整组按钮垂直居中：中线 - 整组高度的一半
	for (int i = 0; i < scNum; i++)       // 循环 7 次
		screenInputPage.addButton(centerX(w, scTexts[i], 30), scY + rowH * i, scTexts[i], 30);
	// 第 i 个按钮放在 scY + rowH*i：一个接一个往下排，用循环代替写 7 遍

	// --- 游戏对局页：1个返回按钮（棋盘在 main 循环里画，居中显示） ---
	play.addButton(centerX(w, "返回主页", 30), h - rowH * 2, "返回主页", 30);

	// --- 联机对弈页：创建/加入 + IP/端口输入 + 返回 ---
	int ny0 = h / 4 + 30;              // 第一行起点
	int nrow = h / 9;                  // 行距
	// 行1：创建房间 / 加入房间 并排，整组居中
	int nw1 = btnWidth("创建房间(主机)", 24), nw2 = btnWidth("加入房间(客户端)", 24);
	int nx1 = w / 2 - (nw1 + nw2 + 20) / 2;
	netPage.addButton(nx1, ny0, "创建房间(主机)", 24);
	netPage.addButton(nx1 + nw1 + 20, ny0, "加入房间(客户端)", 24);
	// 行2：IP 输入框（允许小数点，最多 15 位）+ 端口输入框（最多 5 位），整组居中
	// label 提示两种填法：直连填"对方电脑 IP"，中转填"服务器公网 IP"
	netPage.addInput(w / 2 - 300, ny0 + nrow, 300, 46, true, "对方IP或服务器IP", true, 15);
	netPage.addInput(w / 2 + 10,  ny0 + nrow, 200, 46, false, "端口", false, 5);   // 端口框 200 宽，5 位数放得下
	netPage.inputs[1].text = "5000";   // 端口默认填 5000（和 NET_PORT 一致，一般不用改）
	netPage.inputs[1].cursorIndex = 4;
	// 行3：通过服务器联机（异地用，填服务器IP）/ 返回主页 并排
	int nw3 = btnWidth("通过服务器联机", 24), nw4 = btnWidth("返回主页", 24);
	int nx3 = w / 2 - (nw3 + nw4 + 20) / 2;
	netPage.addButton(nx3, ny0 + nrow * 3, "通过服务器联机", 24);
	netPage.addButton(nx3 + nw3 + 20, ny0 + nrow * 3, "返回主页", 24);
}

// ===================== 页面点击分发 =====================
void OnClick(int& pageState, int idx, int& boardX, int& boardY)
{                                    // 引用参数：改 pageState/boardX/boardY = 改 main() 里的原变量
	switch (pageState)                // switch = 按值分路，比一长串 if-else 清晰
	{                                 // 根据当前页面，决定点中按钮后干什么
	case PAGE_HOME:                   // —— 主页
		if (idx == 0)      pageState = PAGE_INPUT_BOARD;    // 开始游戏 → 棋盘输入页（有确认按钮）
		else if (idx == 1) pageState = PAGE_NET;            // 联机对弈 → 联机设置页
		else if (idx == 2) pageState = PAGE_SYS_SETTING;    // 游戏设置 → 系统设置页
		else if (idx == 3) { closegraph(); exit(0); }       // 退出游戏：关窗口、结束进程
		break;                          // break 必须写，否则会"掉进"下一个 case
	case PAGE_GAME_SETTING:            // —— 棋盘设置页
		if (idx == 0) { settingPage.clearInputs(); pageState = PAGE_HOME; }   // 返回时清空输入
		break;
	case PAGE_SYS_SETTING:             // —— 系统设置页
		if (idx == 0)      pageState = PAGE_INPUT_SCREEN;  // 屏幕尺寸 → 屏幕尺寸页
		else if (idx == 1) pageState = PAGE_HOME;          // 返回主页
		break;
	case PAGE_INPUT_BOARD:             // —— 棋盘输入页（对局设置）
		// 按钮下标：0~2=游戏模式 3/4=模板/自定义 5~8=大小 9/10=先后手 11=确认 12=返回
		if (idx == 0) g_game = 1;                       // 五子棋
		else if (idx == 1) { g_game = 2; boardX = boardY = 18; }   // 围棋：18 格 = 19 路交叉点
		else if (idx == 2) { g_game = 3; boardX = boardY = 3; }    // 井字棋：3×3 格
		else if (idx == 3) g_mode = 1;                   // 模板
		else if (idx == 4) g_mode = 2;                   // 自定义
		else if (idx >= 5 && idx <= 8)                   // 模板预设大小（仅五子棋显示）
		{
			static const int sizes[4] = { 5, 9, 13, 15 };
			boardX = boardY = sizes[idx - 5];            // 行数列数 = 预设值
		}
		else if (idx == 9) g_turn = 1;                   // 黑方先手
		else if (idx == 10) g_turn = 2;                  // 白方先手
		else if (idx == 11)                              // 确认
		{
			// 五子棋自定义模式：读输入框（失焦已钳制，这里兜底）
			if (g_game == 1 && g_mode == 2)
			{
				if (!inputPage.inputs[0].text.empty()) boardX = stoi(inputPage.inputs[0].text);
				else boardX = 5;
				if (!inputPage.inputs[1].text.empty()) boardY = stoi(inputPage.inputs[1].text);
				else boardY = 5;
				if (boardX < 5) boardX = 5;              // 兜底钳制
				if (boardX > 30) boardX = 30;
				if (boardY < 5) boardY = 5;
				if (boardY > 30) boardY = 30;
			}
			// 围棋/井字棋：boardX/boardY 已由模式按钮设好（18 或 3）
			resetBoard();               // 新开一局：清空棋盘
			pageState = PAGE_PLAY;      // 进入游戏对局页
		}
		else if (idx == 12)            // 返回主页
		{
			inputPage.clearInputs();    // 返回时清空输入
			pageState = PAGE_HOME;
		}
		break;
	case PAGE_PLAY:                  // —— 游戏对局页
		if (idx == 0)
		{
			if (g_netMode) { netSend(3, 0, 0); netEnd(); }   // 联机中：先通知对方"我走了"再断开
			inputPage.clearInputs();
			pageState = PAGE_HOME;
		}
		break;
	case PAGE_INPUT_SCREEN:            // —— 屏幕尺寸页
		if (idx == 0)  // 全屏：无边框铺满整个屏幕(含任务栏位置)
			SwitchScreen(true, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		// GetSystemMetrics 问系统要屏幕分辨率；true = 无边框全屏
		else if (idx == 1) // 全屏窗口化：铺满工作区(任务栏可见)，带标题栏
		{
			RECT rc;                       // Windows 矩形结构：left/top/right/bottom
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);   // 取"工作区"= 屏幕去掉任务栏
			// &rc = 让系统把结果写进 rc；宽 = 右-左，高 = 下-上
			SwitchScreen(false, rc.right - rc.left, rc.bottom - rc.top);
		}
		else if (idx >= 2 && idx <= 5) // 1680*890 / 1280*720 / 1024*768 / 640*480
		{
			static const int res[4][2] = { {1680,890},{1280,720},{1024,768},{640,480} };
			// 二维数组：4 组(宽,高)。static = 只在第一次调用时创建一次，不反复重建
			SwitchScreen(false, res[idx - 2][0], res[idx - 2][1]);
			// 按钮下标 2~5 → 数组 0~3；[0]=宽 [1]=高
		}
		else if (idx == 6) pageState = PAGE_SYS_SETTING;   // 返回
		break;
	case PAGE_NET:                   // —— 联机对弈页
		if (idx == 0)      startNetHost();   // 创建房间（当主机）：初始化+监听，见 toolFunction.cpp
		else if (idx == 1) { if (startNetClient()) pageState = PAGE_PLAY; }   // 加入房间（成功才进对局页）
		else if (idx == 2) startNetRelay();   // 通过服务器联机：连中转服务器，配对上自动进对局
		else if (idx == 3)           // 返回主页
		{
			if (g_netMode) netEnd();   // 已创建/尝试过 → 清理网络
			netPage.clearInputs();
			g_netStatus = "";
			pageState = PAGE_HOME;
		}
		break;
	}
}
