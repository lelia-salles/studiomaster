[Setup]
AppName=StudioMaster
AppVersion=1.0.0
AppPublisher=Master Musica
; Instala o Standalone na pasta Arquivos de Programas
DefaultDirName={autopf}\Master Musica\StudioMaster
DefaultGroupName=StudioMaster
; Define onde o instalador final será salvo
OutputDir=C:\Users\JOAO SALES\studiomaster\build\Installer
OutputBaseFilename=StudioMaster_Win_Installer_v1.0
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64

[Files]
; Copia o aplicativo Standalone
Source: "C:\Users\JOAO SALES\studiomaster\build\StudioMaster_artefacts\Release\Standalone\studiomaster.exe"; DestDir: "{app}"; Flags: ignoreversion

; Copia o plugin VST3 para a pasta padrão do Windows para plugins
Source: "C:\Users\JOAO SALES\studiomaster\build\StudioMaster_artefacts\Release\VST3\StudioMaster.vst3\*"; DestDir: "{commoncf64}\VST3\StudioMaster.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Cria atalhos no Menu Iniciar e na Área de Trabalho
Name: "{group}\StudioMaster"; Filename: "{app}\studiomaster.exe"
Name: "{autodesktop}\StudioMaster"; Filename: "{app}\studiomaster.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Criar um ícone na Área de Trabalho"; GroupDescription: "Ícones Adicionais:"