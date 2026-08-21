[Setup]
AppId={{E4F14619-EDF4-4CCB-BBF9-251730C00B4A}
AppName=cseq
AppVersion=1.0
AppPublisher=geltz
DefaultDirName={autopf}\cseq
UsePreviousAppDir=no
DefaultGroupName=cseq
SetupIconFile=cseq.ico
UninstallDisplayIcon={app}\cseq.ico
Compression=lzma2/ultra64
SolidCompression=yes
OutputDir=.
OutputBaseFilename=cseq_setup_1.0
WizardStyle=modern
ChangesAssociations=no
DirExistsWarning=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "cseq.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "cseq.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\cseq"; Filename: "{app}\cseq.exe"; IconFilename: "{app}\cseq.ico"
Name: "{group}\{cm:UninstallProgram,cseq}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\cseq"; Filename: "{app}\cseq.exe"; IconFilename: "{app}\cseq.ico"; Tasks: desktopicon

[UninstallDelete]
Type: files; Name: "{app}\cseq.exe"
Type: files; Name: "{app}\cseq.ico"
Type: files; Name: "{app}\unins*.exe"
Type: files; Name: "{app}\unins*.dat"

[Run]
Filename: "{app}\cseq.exe"; Description: "{cm:LaunchProgram,cseq}"; Flags: nowait postinstall skipifsilent