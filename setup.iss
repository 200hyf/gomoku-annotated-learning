; ============================================================
;  五子棋注释学习版 安装包脚本（Inno Setup 6）
;  编译方式：ISCC.exe setup.iss
;  或者：右键本文件 → 用 Inno Setup 编译
; ============================================================
#define MyAppName "五子棋注释学习版"
#define MyAppVersion "1.1.0"
#define MyAppExeName "五子棋注释学习版.exe"
#define MyAppPublisher "200hyf"

[Setup]
; 安装程序唯一标识（卸载/升级靠它识别，定下后别改）
AppId={{6AF43B00-8A06-4959-A92C-396432C2322D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
; 默认安装目录：Program Files 下
DefaultDirName={autopf}\{#MyAppName}
; 开始菜单组名
DefaultGroupName={#MyAppName}
; 卸载程序使用的图标（指向主程序）
UninstallDisplayIcon={app}\{#MyAppExeName}
; 输出到工程目录\安装包\，文件名带版本号
OutputDir=安装包
OutputBaseFilename=五子棋注释学习版-Setup-{#MyAppVersion}
; 高压缩率
Compression=lzma2
SolidCompression=yes
; 现代风格向导
WizardStyle=modern
; 只允许 64 位系统（我们的 exe 是 x64 构建）
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
; 简体中文界面（使用下载的 ChineseSimplified.isl）
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Tasks]
; 可选项：桌面快捷方式（默认不打勾）
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加任务："; Flags: unchecked

[Files]
; {app} = 用户选择的安装目录。三个文件都会装进去
Source: "x64\Release\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; 开始菜单快捷方式
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
; 桌面快捷方式（仅当用户勾选了 desktopicon 任务）
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; 安装完成后可勾选"立即运行"
Filename: "{app}\{#MyAppExeName}"; Description: "立即运行 {#MyAppName}"; Flags: nowait postinstall skipifsilent
