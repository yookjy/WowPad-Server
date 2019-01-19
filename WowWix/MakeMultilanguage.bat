cls 
@echo off
setlocal 
echo ==========================
echo ����ȭ �ν��緯 ����
echo ==========================

SET TARGET_DIR=Release
SET INSTALL_FILE=WowPad.msi

IF EXIST %1 SET TARGET_DIR=%1
IF EXIST %2 SET INSTALL_FILE=%2
IF NOT EXIST .\%TARGET_DIR%\multilanguage mkdir .\%TARGET_DIR%\multilanguage
REM ECHO �۾���� : %TARGET_DIR%
path=%path%;"C:\Program Files (x86)\WiX Toolset v3.7\bin";

ECHO 1. WowPad.msi ������...
copy .\%TARGET_DIR%\en-us\%INSTALL_FILE% .\%TARGET_DIR%\multilanguage\%INSTALL_FILE%
ECHO 2. �� �ν��緯���� mst���� ������...
torch.exe -p -t language .\%TARGET_DIR%\multilanguage\%INSTALL_FILE% .\%TARGET_DIR%\ja-jp\%INSTALL_FILE% -out .\%TARGET_DIR%\multilanguage\ja-jp.mst
torch.exe -p -t language .\%TARGET_DIR%\multilanguage\%INSTALL_FILE% .\%TARGET_DIR%\ko-kr\%INSTALL_FILE% -out .\%TARGET_DIR%\multilanguage\ko-kr.mst
ECHO 3. mst���� ����Ϸ�

ECHO 4. ���� �ν��緯�� mst���� ������...
EmbedTransform.exe .\%TARGET_DIR%\multilanguage\%INSTALL_FILE% .\%TARGET_DIR%\multilanguage\ja-jp.mst
EmbedTransform.exe .\%TARGET_DIR%\multilanguage\%INSTALL_FILE% .\%TARGET_DIR%\multilanguage\ko-kr.mst
ECHO 5. mst���� �Ϸ�

REM DELETE .\%TARGET_DIR%\multilanguage\ja-jp.mst
REM DELETE .\%TARGET_DIR%\multilanguage\ko-kr.mst

REM msiexec /i .\%TARGET_DIR%\multilanguage\%INSTALL_FILE% TRANSFORMS=":ko-kr.mst"

echo ==========================
echo ����ȭ �ν��緯 ���� �Ϸ�
echo ==========================