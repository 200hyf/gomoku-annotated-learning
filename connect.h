//connect.h
#pragma once                    // "只包含一次"：防止这个头文件被重复 #include 导致重复定义
#include <winsock2.h>           // socket 网络编程头（放最前：与 windows.h 里的老版 socket 头兼容）
#include <vector>               // 引入标准库 vector：自动扩容的数组
#include <string>               // 引入 string：字符串类型
#include <graphics.h>           // EasyX 图形库：画图、建窗口、收鼠标键盘消息
#include <windows.h>            // Windows 系统 API：窗口样式、屏幕尺寸、RECT 等
using namespace std;            // 声明用 std 命名空间，不用写 std::vector、std::string

// ===================== 页面状态 =====================
enum PageState {                // enum = 枚举：给一组整数起名字，比记数字好读
	PAGE_HOME = 0,              // 主页 = 0
	PAGE_GAME_SETTING = 1,      // 棋盘设置页 = 1
	PAGE_SYS_SETTING = 2,       // 系统设置页 = 2
	PAGE_INPUT_BOARD = 3,       // 棋盘行列输入页 = 3
	PAGE_INPUT_SCREEN = 4,      // 屏幕尺寸输入页 = 4
	PAGE_PLAY = 5,              // 游戏对局页 = 5
	PAGE_NET = 6                // 联机对弈设置页 = 6（新）
};

// ===================== UI 默认样式（改这里 = 全局生效） =====================
static const char* UI_FONT = "宋体";    // 默认字体：所有文字用它，想换字体改这一处
static const int  BTN_PAD_X = 14;       // 按钮左右内边距：按钮宽 = 文字宽 + 2×这个值
static const int  BTN_PAD_Y = 6;        // 按钮上下内边距：按钮高 = 文字高 + 2×这个值
static const int  GO_WIN = 10;          // 围棋提子数达到这个值即胜（简化胜负规则）
static const int  GO_EDGE = 18;         // 围棋棋盘 18 格 = 19 路交叉点（编号 0~18）
static const int  NET_PORT = 5000;      // 联机默认端口（局域网内两台电脑都要用同一个端口）
// 气/提子计算只认这个边界：棋盘外(19~30)的位置虽然数组里是 0，但不能当"气"，否则边缘棋子永远提不掉

// ===================== 联机消息 =====================
// 网络上发的每一条消息 = 固定 4 字节：操作码 + 落子坐标 + 占位
struct NetMsg
{
	char op;    // 1=落子（后面 col,row 是要落的子） 3=对方离开
	char col;   // 落子的列
	char row;   // 落子的行
	char pad;   // 占位符（凑够 4 字节，保持消息格式固定好解析）
};

// ===================== 工具函数（声明） =====================
// 注意：这两个声明必须放在 Page 结构体之前——Page::draw 里要调用 zh，而 C++ 规定"调用前必须先见到声明"
string zh(const string& s);   // 中文编码自适应：把源码里的 UTF-8 字符串转成当前系统能正确显示的编码
string getLocalIP();          // 获取本机 IPv4 地址（联机页右下角显示，方便对方填写）

// ===================== 结构体 =====================
// 普通文字按钮
struct TextButton               // struct = 结构体：把一组相关数据打包成一种"类型"
{
	int x, y, w, h;             // x,y = 左上角坐标；w,h 在绘制时按文字自动算（见 drawButton）
	string text;                // 按钮上显示的文字
	int fontSize;               // 字号（像素）
	COLORREF normalColor;       // 平时颜色（COLORREF = Windows 颜色类型，WHITE/YELLOW 都是它）
	COLORREF hoverColor;        // 鼠标悬停时的高亮颜色
	bool enable;                // 是否可交互（true=能点）
	const char* font;           // 这个按钮自己的字体（默认填 UI_FONT = 跟随全局）
};

// 文本输入框
struct TextInputBox
{
	int x, y, w, h;             // 位置与大小（宽高固定；字号、偏移按它自适应）
	string text;                // 输入框里的文字
	COLORREF bg, border, textColor;         // 普通态颜色：背景 / 边框 / 文字
	bool isFocus;               // 是否被选中（有光标在闪）
	int cursorIndex;            // 光标停在"第几个字符"的位置
	bool enable;                // 是否显示/可交互（自定义模式才显示输入框）
	string label;               // 占位提示文字（框为空时显示，如"列数(5~30)"）
	bool allowDot = false;      // 是否允许输入小数点（联机的 IP 框需要：192.168.1.5）
	int  maxLen = 2;            // 最多能输入几个字符（棋盘框 2 位；IP 框 15 位；端口框 5 位）
};
// 精简说明：删掉了 enable(永远 true)、blinkState(改为函数参数)、3 个"高亮色"字段
// （聚焦高亮色统一写在 drawTextBox 里，原来也没法单独配，删了无损失）

// 鼠标位置
struct coordinates { int x, y; };   // 极简结构体：鼠标的 x、y

// 棋盘
struct mup { int x, y, size, x_num, y_num; };   // 左上角(x,y)、格子边长、列数、行数
// 小提醒：标准库有个 std::map，这里自己定义了同名 map，目前没用到 std::map 所以能编译，以后慎用这个命名 (已改)

// ===================== 函数声明 =====================
bool inRect(int x, int y, int rx, int ry, int rw, int rh);
// 上面是"函数声明"：只写长什么样（返回类型、名字、参数），不写实现。实现写在 toolFunction.cpp
void drawButton(coordinates judp, TextButton& btn);
// void = 没有返回值；TextButton& 的 & 是"引用"：传的是原按钮本身，不是复制品 → 函数里改它，外面跟着变
void drawTextBox(coordinates judp, TextInputBox& box, bool blink);  // 画输入框（blink=光标亮灭）
void handleInput(TextInputBox& box, char ch);           // 处理键盘输入
void drawMap(mup d);                                    // 画棋盘
bool putChess(int col, int row);                        // 在(col,row)落子，成功返回 true
void drawChesses(mup d);                                // 在棋盘上画已落的棋子
void resetBoard();                                      // 清空棋盘（新开一局）
int checkWin(int row, int col);                         // 落子后判定胜负：1黑胜/2白胜/3平/0未分
void Renderscreen();                                    // 建窗口（全屏版）
void Renderscreen(int w, int h);                        // 建窗口（窗口化版）
// 同名函数、参数不同 = "函数重载"，调用时编译器按参数自动选哪个
void RenderscreenNoBorder(int w, int h);                // 无边框窗口
void SwitchScreen(bool borderless, int w, int h);       // 统一切换窗口模式
void CheckWindowSize();                                 // 每帧检查窗口尺寸
void initPages();                                       // 构建所有页面
void OnClick(int& pageState, int idx, int& boardX, int& boardY);
// 参数里的 & 也是引用：OnClick 里改 pageState，main() 里的 pageState 会跟着变 —— 让函数"把结果带出来"

// ===================== 联机网络函数声明 =====================
// 实现都在 toolFunction.cpp（网络章节），这里只是告诉其他文件"有这些函数"
bool netInit();                 // 初始化网络（WSAStartup，程序里第一次联网前调用一次）
void netEnd();                  // 结束网络：关 socket、清理（回到本地模式）
bool netHostStart(short port);  // 当主机：创建监听 socket，等别人连进来
bool netHostAccept();           // 主机每帧调用：有人连就接上，返回 true
bool netClientConnect(const char* ip, short port);   // 当客户端：连主机
bool netRecv(NetMsg& msg);      // 收一条消息（非阻塞，没消息返回 false）
bool netSend(char op, char col, char row);           // 发一条消息
// 联机流程封装（page.cpp 的 OnClick 调用，实现见 toolFunction.cpp）
void startNetHost();            // 点"创建房间"：初始化+监听，只改状态文字
bool startNetClient();          // 点"加入房间"：读IP端口+拨号，成功返回 true（调用方负责跳对局页）

// ===================== 页面 =====================
struct Page                     // 页面：除了数据，还能有函数（叫"成员函数/方法"）
{
	string title;               // 页面标题（顶部居中显示，空 = 不显示）
	vector<TextButton> buttons; // 这个页面有哪些按钮
	vector<TextInputBox> inputs;// 这个页面有哪些输入框
	int lostFocusIdx = -1;      // 记录"刚失焦的输入框"下标（-1 = 没有）；失焦钳制要用

	// 设置页面标题
	void setTitle(const string& t) { title = t; }

	// 添加按钮：前 4 个参数必填；后面 3 个"样式参数"带默认值，不传 = 用全局默认样式
	// 想让某个按钮与众不同：多传颜色/字体即可，
	// 例：addButton(x, y, "退出游戏", 40, RED, LIGHTRED);   ← 平时红色、悬停亮红
	void addButton(int x, int y, const string& text, int fontSize,
		COLORREF normalColor = WHITE, COLORREF hoverColor = YELLOW, const char* font = UI_FONT)
	{
		buttons.push_back({ x, y, 0, 0, text, fontSize, normalColor, hoverColor, true, font });
		// 10 个值对应 TextButton 的 10 个字段；宽高先填 0，绘制时按文字自动算
		// push_back = 往数组末尾加一个元素；大括号 {…} = 按字段顺序依次赋值（"聚合初始化"）
	}
	// 添加输入框(默认样式)；label 是框内占位提示，不传就没有
	// allowDot：是否允许输入小数点（联机 IP 框需要）；maxLen：最多几位字符
	void addInput(int x, int y, int w, int h, bool focus, const string& label = "",
		bool allowDot = false, int maxLen = 2)
	{
		inputs.push_back({ x, y, w, h, "", BLACK, WHITE, WHITE, focus, 0, true, label, allowDot, maxLen });
		// 14 个值对应 TextInputBox 的 14 个字段
	}
	// 取消所有输入框焦点（先记下"谁正在失焦"，再全部取消）
	void clearFocus()
	{
		for (int i = 0; i < (int)inputs.size(); i++)
			if (inputs[i].isFocus) { lostFocusIdx = i; break; }   // 记录刚失焦的框
		for (auto& i : inputs) i.isFocus = false;
		// 范围 for："遍历 inputs 里每一个元素，起名叫 i"
		// auto = 让编译器自动猜类型（这里猜出 TextInputBox&）；& = 直接改原件
	}
	// 清空所有输入框（返回/确认后调用），并让第一个框重新聚焦
	void clearInputs()
	{
		for (auto& i : inputs)
		{
			i.text = "";          // 清空文字
			i.cursorIndex = 0;    // 光标归零
			i.isFocus = false;    // 先全部取消焦点
		}
		if (!inputs.empty()) inputs[0].isFocus = true;   // 聚焦第一个框，可直接输入
	}
	// 点击输入框切换焦点
	void focusInput(coordinates c)
	{
		for (auto& i : inputs)
			if (i.enable && inRect(c.x, c.y, i.x, i.y, i.w, i.h))   // 框可用且点中它
			{
				clearFocus();       // 先让所有框失去焦点
				i.isFocus = true;   // 再把点中的这个设成焦点
			}
	}
	// 统一画整个页面：先画标题，再画按钮和输入框，并处理键盘输入与光标闪烁
	void draw(coordinates judp, bool blink, char key)
	{
		// ===== 画页面标题（顶部居中） =====
		if (!title.empty())           // 标题非空才画
		{
			settextstyle(40, 0, UI_FONT);            // 标题字号固定 40
			setbkmode(TRANSPARENT);
			settextcolor(WHITE);
			string t = zh(title);                    // 中文编码自适应（UTF-8→GBK 等）
			outtextxy(getwidth() / 2 - textwidth(t.c_str()) / 2, 100, t.c_str());
			// 水平居中：(窗口宽 - 标题宽) / 2，顶部留 45 像素
		}
		for (auto& b : buttons) if (b.enable) drawButton(judp, b);   // 画按钮（隐藏的跳过）
		for (auto& i : inputs) if (i.enable)                         // 处理输入框（隐藏的跳过）
		{
			drawTextBox(judp, i, blink);   // 画输入框（顺带把光标亮灭传进去）
			handleInput(i, key);          // 处理这个框的键盘输入
		}
	}
	// 检测点中了哪个按钮，返回下标(-1=没点中)
	int hitButton(coordinates mouse) const
	{
		for (int i = 0; i < (int)buttons.size(); i++)   // 从头到尾遍历按钮数组
			if (buttons[i].enable && inRect(mouse.x, mouse.y, buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h))
				return i;   // 按钮要"可用"且点在它里面 → 立刻返回下标
		return -1;                                      // 一个都没点中 → 返回 -1
	}
	// 函数名后面加 const = 这个函数保证不改动页面数据，只是"查"
	// buttons.size() 返回无符号数，(int) 强转成有符号，避免和 i 比较时报警告
};

// ===================== 全局页面 =====================
extern Page homePage;        // 主页
extern Page settingPage;     // 棋盘设置页
extern Page sysPage;         // 系统设置页
extern Page inputPage;       // 棋盘行列输入页
extern Page screenInputPage; // 屏幕尺寸输入页
extern Page play;            // 游戏对局页
extern Page netPage;         // 联机对弈设置页（新）
// ===================== 对局数据（声明） =====================
extern int g_board[31][31];  // 棋盘状态：0=空 1=黑子 2=白子（31 = 30 路棋盘有 31 个交叉点）
extern int boardX, boardY;   // 棋盘行列数（联机固定 15×15；全局化后 main 和网络层都能改）
extern int g_turn;           // 当前该谁下：1=黑 2=白
extern int g_mode;           // 输入页模式：1=模板（预设大小）2=自定义（手动输入）
extern int g_game;           // 游戏模式：1=五子棋 2=围棋 3=井字棋
extern int g_winner;         // 胜负：0=进行中 1=黑胜 2=白胜 3=平局
extern int g_blackCap;       // 黑方提子数（围棋：黑提掉的白子数）
extern int g_whiteCap;       // 白方提子数（围棋：白提掉的黑子数）

// ===================== 联机状态（声明） =====================
extern bool   g_netMode;      // 是否联机对局（true = 当前这盘是联机）
extern bool   g_netHost;      // 我是不是主机（服务器）
extern bool   g_netConnected; // 双方是否已连上
extern int    g_netTurn;      // 我方执棋：主机=1黑先 客户端=2白后
extern SOCKET g_netSock;      // 网络 socket（Windows 的"网络句柄"）
extern string g_netStatus;    // 联机页状态文字（"等待对方加入…"/"连接失败…"）
// extern = "声明这些变量在其他文件里已经定义好了"
// 真正的定义在 page.cpp，那里才是"创建"；这里只是告诉大家"有这么几个对象"
// clampInputSize 声明放在这里：它用到 Page 类型，必须等 Page 定义完之后才能声明
void clampInputSize(Page& pg);
// 输入框"取消选择"(失焦)时调用：只修正刚失焦的那个框，越界值钳制到 5~30 并写回
