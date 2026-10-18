//main.cpp
#include <winsock2.h>       // socket 网络编程头（必须最先，与 windows.h 兼容）
#include <graphics.h>       // EasyX 图形库
#include <windows.h>        // Windows API
#include <string>           // string 类型
#include "connect.h"        // 自己写的头文件用双引号：数据结构和函数声明都在里面
using namespace std;

// ===================== 主循环 =====================
int main()                  // 程序入口：操作系统从这里开始执行你的代码
{
	Renderscreen();           // ① 建窗口（全屏无边框）
	initPages();              // ② 把 5 个页面的按钮、输入框全部建好
	BeginBatchDraw();         // ③ 开启"批量绘图"：先画在内存里，攒一帧再整屏刷新 → 不闪烁

	int pageState = PAGE_HOME;      // 当前在哪个页面，初始 = 主页
	// boardX/boardY（棋盘行列数）已全局化，定义在 page.cpp，main 和网络层共用
	ExMessage msg;                  // EasyX 的消息结构：一次鼠标/键盘事件的数据
	coordinates mouse = { 0, 0 }, click = { 0, 0 };   // 鼠标当前位置、点击位置
	bool mouseClick = false;        // 这一帧有没有发生左键点击
	char key = 0;                   // 这一帧按下的键
	bool blink = false;             // 光标当前亮不亮
	int frame = 0;                  // 帧计数器（数到 15 就翻一次光标）
	Page* pages[] = { &homePage, &settingPage, &sysPage, &inputPage, &screenInputPage, &play, &netPage };
	// 指针数组：把 7 个页面的"地址"排成一列。& = 取地址
	// 数组下标正好 = 枚举值：pages[0] 主页、pages[5] 游戏对局页、pages[6] 联机设置页
	// Page* 意思是"指向 Page 的指针"，用指针访问成员要写 ->

	while (true)                    // 死循环：游戏一直跑，直到主动退出
	{                               // 一次循环 = 一帧（一张画面）
		CheckWindowSize();            // 检查窗口是否被最大化/拖拽缩放，变了就重建让画面铺满
		cleardevice();                // 清空画布，把上一帧的画面擦掉
		mouseClick = false;           // 重置这一帧的临时状态
		key = 0;
		click = mouse;                // 没新点击时，click 沿用当前鼠标位置
		// 对局页棋盘位置（每帧按当前窗口尺寸重算，居中）
		mup board = { getwidth() / 2 - boardX * 30 / 2, getheight() / 2 - boardY * 30 / 2, 30, boardX, boardY };

		// 光标闪烁计时
		if (++frame >= 30) { blink = !blink; frame = 0; }
		// ++frame = 先把 frame 加 1 再比较；每 15 帧把 blink 翻转一次 → 光标一闪一闪

		Page* curPage = pages[pageState];   // 取出"当前页面"的指针
		// 消息循环
		while (peekmessage(&msg, EX_MOUSE | EX_KEY | EX_CHAR))
			// peekmessage = 从消息队列取一条消息（非阻塞，没有就返回假）
			// EX_MOUSE | EX_KEY | EX_CHAR：按位或 = 要"鼠标"或"键盘"或"字符"这三类消息
			// EX_CHAR 单独一类：字符消息（WM_CHAR），句号键的字符码在这里最可靠
		{
			mouse.x = msg.x;          // 每条消息都带鼠标坐标 → 实时更新鼠标位置
			mouse.y = msg.y;
			if (msg.message == WM_LBUTTONDOWN)   // 如果是"鼠标左键按下"
			{
				mouseClick = true;    // 标记：这一帧发生了点击
				click = mouse;        // 记录点击位置
			}
			if (msg.message == WM_KEYDOWN)       // 如果是"键盘按下"
			{
				key = msg.vkcode;     // 记录键的虚拟键码（VK_RETURN=回车、'0'~'9' 数字）
				// 按 Enter = 取消选中（数字自动保留在输入框里）
				if (msg.vkcode == VK_RETURN)
				{
					curPage->clearFocus();   // -> 指针访问成员
					// 失焦方式①：按回车 → 当场钳制刚失焦的框
					if (pageState == PAGE_INPUT_BOARD) clampInputSize(inputPage);
				}
			}
			// 字符消息（WM_CHAR）：句号键的字符码比虚拟键码更可靠（布局无关）。
			// 只接受数字/句号/退格，其他（如输入法产生的全角符号）一律忽略
			if (msg.message == WM_CHAR)
			{
				char c = msg.ch;                       // 字符消息带的是"字符"本身（TCHAR=char）
				if ((c >= '0' && c <= '9') || c == '.' || c == 8)
					key = c;   // 覆盖掉虚拟键码：主键盘句号 → '.'(0x2E)，正好被 handleInput 接收
			}
		}

		// 输入页：游戏模式/模板/大小/先后手的点亮与显示切换
		if (pageState == PAGE_INPUT_BOARD)
		{
			inputPage.buttons[0].normalColor = (g_game == 1) ? YELLOW : WHITE;   // 五子棋
			inputPage.buttons[1].normalColor = (g_game == 2) ? YELLOW : WHITE;   // 围棋
			inputPage.buttons[2].normalColor = (g_game == 3) ? YELLOW : WHITE;   // 井字棋
			// 模板/自定义只在五子棋模式显示；模板模式下再细分"模板/自定义"
			bool showSize = (g_game == 1);
			inputPage.buttons[3].enable = showSize;   // 模板
			inputPage.buttons[4].enable = showSize;   // 自定义
			inputPage.buttons[3].normalColor = (showSize && g_mode == 1) ? YELLOW : WHITE;
			inputPage.buttons[4].normalColor = (showSize && g_mode == 2) ? YELLOW : WHITE;
			bool tpl = showSize && (g_mode == 1);     // 模板模式才显示大小按钮
			for (int i = 5; i <= 8; i++) inputPage.buttons[i].enable = tpl;
			inputPage.inputs[0].enable = showSize && !tpl;   // 自定义模式才显示输入框
			inputPage.inputs[1].enable = showSize && !tpl;
			static const int sizes[4] = { 5, 9, 13, 15 };
			for (int i = 0; i < 4; i++)           // 大小按钮点亮（下标 5~8）
				inputPage.buttons[i + 5].normalColor =
					(boardX == sizes[i] && boardY == sizes[i]) ? YELLOW : WHITE;
			inputPage.buttons[9].normalColor = (g_turn == 1) ? YELLOW : WHITE;   // 黑先
			inputPage.buttons[10].normalColor = (g_turn == 2) ? YELLOW : WHITE;  // 白先
		}

		// 联机页：创建/加入进行中 → 锁死按钮和输入框（防止重复创建/连两次）
		if (pageState == PAGE_NET)
		{
			bool busy = g_netMode;   // 已经开始等待/连接了
			netPage.buttons[0].enable = !busy;   // 创建房间
			netPage.buttons[1].enable = !busy;   // 加入房间
			netPage.buttons[2].enable = !busy;   // 通过服务器联机（连服务器也算"开始连接"）
			netPage.inputs[0].enable = !busy;    // IP 框
			netPage.inputs[1].enable = !busy;    // 端口框
			// buttons[3] = 返回主页：不锁，等待中也能随时撤（netEnd 会清理）
		}

		// 画当前页面
		curPage->draw(mouse, blink, key);
		// 这一行干了三件事：画所有按钮、画所有输入框、把键盘输入填进选中的输入框

		// 联机页：把网络状态文字画在输入框下面（比如"等待对方加入…"）
		if (pageState == PAGE_NET && !g_netStatus.empty())
		{
			int ny0 = getheight() / 4 + 30, nrow = getheight() / 9;
			settextstyle(20, 0, UI_FONT);
			setbkmode(TRANSPARENT);
			settextcolor(LIGHTGRAY);
			string st = zh(g_netStatus);   // 中文编码自适应
			outtextxy(getwidth() / 2 - textwidth(st.c_str()) / 2, ny0 + nrow * 2 + nrow / 2 - 15, st.c_str());
		}
		// 联机页：右下角显示本机 IP（直连时让对方填这个；中转联机时输入框应填服务器IP，不是这个）
		if (pageState == PAGE_NET)
		{
			static string localIP = getLocalIP();
			if (localIP.empty()) localIP = "127.0.0.1";   // 兜底：取不到就显示回环地址
			settextstyle(18, 0, UI_FONT);
			settextcolor(LIGHTGRAY);
			string ipTip = "本机IP: " + localIP + "（直连时让对方填这个）";   // 防止把本机IP误当成对方IP
			string ipDisp = zh(ipTip);
			outtextxy(getwidth() - textwidth(ipDisp.c_str()) - 20, getheight() - 35, ipDisp.c_str());
		}

		int hitIdx = -1;                  // 本帧点中的按钮下标（-1 = 没点中按钮）
		if (mouseClick)               // 只有这一帧有点击才处理
		{
			curPage->focusInput(click);   // 先看点击是否落在输入框上 → 切换输入焦点
			// 失焦方式②：点击另一个输入框 → 刚失焦的框当场钳制
			if (pageState == PAGE_INPUT_BOARD) clampInputSize(inputPage);
			hitIdx = curPage->hitButton(click);   // 记下点中的按钮（后面落子判断要用）
			OnClick(pageState, hitIdx, boardX, boardY);  // 页面分发
			// hitButton 返回点中的按钮下标；OnClick 根据"当前页面+下标"决定跳转/切分辨率/退出
			// pageState、boardX、boardY 是引用传入，函数里可以直接改它们
		}

		// 对局页：点击落子（五子棋/围棋落交叉点，井字棋落格子中心）
		// 前提：没点中任何按钮，且还没分出胜负；联机时还要求"轮到我"（g_turn == g_netTurn）
		if (mouseClick && pageState == PAGE_PLAY && hitIdx < 0 && g_winner == 0
			&& (!g_netMode || g_turn == g_netTurn))
		{
			int col = 0, row = 0;          // 落子位置（列,行）
			if (g_game == 3)               // 井字棋：按格子索引（3×3 格）
			{
				col = (click.x - board.x) / 30;
				row = (click.y - board.y) / 30;
			}
			else                           // 五子棋/围棋：最近的交叉点
			{
				col = (click.x - board.x + 15) / 30;   // +15 再除 = 四舍五入
				row = (click.y - board.y + 15) / 30;
			}
			bool ok = (g_game == 3) ? (col >= 0 && col < 3 && row >= 0 && row < 3)
			                        : (col >= 0 && col <= boardX && row >= 0 && row <= boardY);
			if (ok && putChess(col, row))     // 落在范围内且落子成功
			{
				if (g_netMode) netSend(1, (char)col, (char)row);   // 联机：把这步棋告诉对方
				int w = checkWin(row, col);   // 判定胜负（参数顺序：行,列）
				if (w) g_winner = w;          // 有人赢（或围棋提子达标）
				else if (g_game == 3)         // 井字棋：查平局（9 格全满）
				{
					int filled = 0;
					for (int i = 0; i < 3; i++)
						for (int j = 0; j < 3; j++)
							if (g_board[i][j] != 0) filled++;
					if (filled == 9) g_winner = 3;   // 3 = 平局
				}
			}
		}

		// ===== 联机网络：每帧轮询 =====
		if (g_netMode)
		{
			// 直连主机：还没人连进来 → 每帧试着接一次
			if (g_netHost && !g_netConnected)
			{
				if (netHostAccept())           // 有人连进来了！
				{
					g_netConnected = true;
					g_game = 1; boardX = boardY = 15;   // 联机固定五子棋 15×15
					g_turn = 1;                // 主机黑先
					resetBoard();
					g_netStatus = "";
					pageState = PAGE_PLAY;     // 双方就位 → 进对局页
				}
			}
			// 中转模式：已连上服务器、还没配对 → 等服务器发身份包（op=7，col=1主机黑/2客户端白）
			else if (g_netRelay && !g_netConnected)
			{
				NetMsg msg;
				if (netRecv(msg) && msg.op == 7)   // 收到身份包 = 服务器已把两人配对
				{
					g_netTurn = msg.col;           // 身份：1=主机(黑先) 2=客户端(白后)
					g_netConnected = true;         // 配对完成
					g_game = 1; boardX = boardY = 15;   // 联机固定五子棋 15×15
					g_turn = 1;                    // 黑先：主机(黑)先手，客户端要等收到落子才能点
					resetBoard();
					g_netStatus = "";
					pageState = PAGE_PLAY;         // 双方就位 → 进对局页
				}
			}
			// 已连上 → 收对方的消息（落子 / 离开）
			if (g_netConnected)
			{
				NetMsg msg;
				while (netRecv(msg))
				{
					if (msg.op == 1 && g_winner == 0)   // 对方落子 → 我这边放上
					{
						putChess(msg.col, msg.row);     // 放对方的子（自动换手）
						int w = checkWin(msg.row, msg.col);
						if (w) g_winner = w;
					}
					else if (msg.op == 3)               // 对方离开
					{
						netEnd();                       // 清理网络
						g_netStatus = "";
						pageState = PAGE_HOME;          // 回主页
						break;
					}
				}
			}
		}

		// 对局页：先画棋盘网格，再画棋子，顶部提示当前执棋/胜负
		if (pageState == PAGE_PLAY)
		{
			drawMap(board);       // 画网格线
			drawChesses(board);   // 画已落的棋子
			// 提示文字：进行中显示"当前执棋"，结束后显示胜负结果
			settextstyle(28, 0, UI_FONT);
			setbkmode(TRANSPARENT);
			const char* tip;
			COLORREF tipColor;
			if (g_winner == 1)      { tip = "黑方胜！";   tipColor = YELLOW; }
			else if (g_winner == 2) { tip = "白方胜！";   tipColor = YELLOW; }
			else if (g_winner == 3) { tip = "平局";       tipColor = YELLOW; }
			else if (g_netMode)     { tip = (g_turn == 1) ? "联机·黑方执棋" : "联机·白方执棋"; tipColor = WHITE; }
			else                    { tip = (g_turn == 1) ? "黑方执棋" : "白方执棋"; tipColor = WHITE; }
			settextcolor(tipColor);
			string tipDisp = zh(tip);                       // 中文编码自适应（提示文字是 UTF-8 源码）
			int tipX = getwidth() / 2 - textwidth(tipDisp.c_str()) / 2;   // 提示文字左缘（水平居中）
			outtextxy(tipX, 150, tipDisp.c_str());
			if (g_winner == 0)   // 进行中：在提示文字后面画当前执棋方的棋子
			{
				int cx = tipX + textwidth(tipDisp.c_str()) + 15;   // 文字右缘再往右 15px
				int cy = 150 + textheight(tip) / 2;    // 棋子中心 = 文字高度中部
				if (g_turn == 1)                       // 黑子：白底 + 黑心（和棋盘画法一致）
				{
					setfillcolor(WHITE); solidcircle(cx, cy, 13);
					setfillcolor(BLACK); solidcircle(cx, cy, 11);
				}
				else                                   // 白子：白色实心
				{
					setfillcolor(WHITE); solidcircle(cx, cy, 13);
				}
			}
			if (g_game == 2)   // 围棋：额外显示双方提子数，能看到胜负进度
			{
				settextstyle(20, 0, UI_FONT);
				string cap = "黑提 " + to_string(g_blackCap) + "  白提 " + to_string(g_whiteCap);
				settextcolor(WHITE);
				outtextxy(getwidth() / 2 - textwidth(zh(cap).c_str()) / 2, 185, zh(cap).c_str());
			}
		}

		FlushBatchDraw();             // 把内存里画好的这一帧，一次性显示到屏幕
		Sleep(10);                    // 睡 10 毫秒再进下一帧：限速 ≈ 100 帧/秒，不让 CPU 空转
	}

	EndBatchDraw();                 // 收尾（正常流程走不到，程序退出时才执行）
	closegraph();                   // 关掉绘图窗口
	return 0;                       // 返回 0 告诉系统"程序正常结束"
}
