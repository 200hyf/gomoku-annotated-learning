//main.cpp
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
	int boardX = 5, boardY = 5;     // 棋盘行列数，默认 5×5
	ExMessage msg;                  // EasyX 的消息结构：一次鼠标/键盘事件的数据
	coordinates mouse = { 0, 0 }, click = { 0, 0 };   // 鼠标当前位置、点击位置
	bool mouseClick = false;        // 这一帧有没有发生左键点击
	char key = 0;                   // 这一帧按下的键
	bool blink = false;             // 光标当前亮不亮
	int frame = 0;                  // 帧计数器（数到 15 就翻一次光标）
	Page* pages[] = { &homePage, &settingPage, &sysPage, &inputPage, &screenInputPage, &play };
	// 指针数组：把 6 个页面的"地址"排成一列。& = 取地址
	// 数组下标正好 = 枚举值：pages[0] 主页、pages[4] 屏幕尺寸页、pages[5] 游戏对局页
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
		while (peekmessage(&msg, EX_MOUSE | EX_KEY))
			// peekmessage = 从消息队列取一条消息（非阻塞，没有就返回假）
			// EX_MOUSE | EX_KEY：按位或 = 只要"鼠标"或"键盘"这两类消息
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

		// 画当前页面
		curPage->draw(mouse, blink, key);
		// 这一行干了三件事：画所有按钮、画所有输入框、把键盘输入填进选中的输入框

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
		// 前提：没点中任何按钮，且还没分出胜负
		if (mouseClick && pageState == PAGE_PLAY && hitIdx < 0 && g_winner == 0)
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
			else                    { tip = (g_turn == 1) ? "黑方执棋" : "白方执棋"; tipColor = WHITE; }
			settextcolor(tipColor);
			int tipX = getwidth() / 2 - textwidth(tip) / 2;   // 提示文字左缘（水平居中）
			outtextxy(tipX, 150, tip);
			if (g_winner == 0)   // 进行中：在提示文字后面画当前执棋方的棋子
			{
				int cx = tipX + textwidth(tip) + 15;   // 文字右缘再往右 15px
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
				outtextxy(getwidth() / 2 - textwidth(cap.c_str()) / 2, 185, cap.c_str());
			}
		}

		FlushBatchDraw();             // 把内存里画好的这一帧，一次性显示到屏幕
		Sleep(10);                    // 睡 10 毫秒再进下一帧：限速 ≈ 100 帧/秒，不让 CPU 空转
	}

	EndBatchDraw();                 // 收尾（正常流程走不到，程序退出时才执行）
	closegraph();                   // 关掉绘图窗口
	return 0;                       // 返回 0 告诉系统"程序正常结束"
}
