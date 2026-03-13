; ==============================================================================
; INSTALLATION SCRIPT - STUDIOMASTER (MASTER MUSICA)
; ==============================================================================

#define MyAppName "StudioMaster"
#define MyAppVersion "1.7.2"
#define MyAppPublisher "Master Musica"
#define MyAppURL "https://mastermusica.com.br/"
#define MyAppExeName "StudioMaster.exe"

[Setup]
; General Installer Settings
AppId={{12345678-ABCD-1234-ABCD-STUDIOMASTER}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; Default installation directory for the Standalone app
DefaultDirName={autopf}\{#MyAppPublisher}\{#MyAppName}
DefaultGroupName={#MyAppPublisher}\{#MyAppName}

; Allows the installer to run in 64-bit mode (required for VST3 plugins)
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

; Visual and output settings
OutputDir=.\Installers
OutputBaseFilename=StudioMaster_Win_Installer_v{#MyAppVersion}
SetupIconFile=images\icon.ico
Compression=lzma2/ultra
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; 1. Copies the Standalone (.exe) to the Program Files folder
Source: "build\StudioMaster_artefacts\Release\Standalone\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

; 2. Copies the VST3 folder/file to the default Windows folder (Common Files\VST3)
; Note: JUCE generates the VST3 as a "package" (folder) on Windows, hence the recursesubdirs flag
Source: "build\StudioMaster_artefacts\Release\VST3\StudioMaster.vst3\*"; DestDir: "{commoncf64}\VST3\StudioMaster.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Creates shortcuts in the Start Menu and on the Desktop
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
; Offers the option to launch the Standalone app right after the installation finishes
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent