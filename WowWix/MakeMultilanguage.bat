cls 
@echo off
setlocal 
echo ==========================
echo 국제화 인스톨러 제작
echo ==========================

SET TARGET_DIR=Release
SET INSTALL_FILE=WowPad.msi

IF EXIST %1 SET TARGET_DIR=%1
IF EXIST %2 SET INSTALL_FILE=%2
IF NOT EXIST .\%TARGET_DIR%\multilanguage mkdir .\%TARGET_DIR%\multilanguage
REM ECHO 작업모드 : %TARGET_DIR%
path=%path%;"C:\Program Files (x86)\WiX Toolset v3.7\bin";

ECHO 1. WowPad.msi 복사중...
copy .\%TARGET_DIR%\en-us\%INSTALL_FILE% .\%TARGET_DIR%\multilanguage\%INSTALL_FILE%
ECHO 2. 각 인스톨러에서 mst파일 추출중...
torch.exe -p -t language .\%TARGET_DIR%\multilanguage\%INSTALL_FILE% .\%TARGET_DIR%\ja-jp\%INSTALL_FILE% -out .\%TARGET_DIR%\multilanguage\ja-jp.mst
torch.exe -p -t language .\%TARGET_DIR%\multilanguage\%INSTALL_FILE% .\%TARGET_DIR%\ko-kr\%INSTALL_FILE% -out .\%TARGET_DIR%\multilanguage\ko-kr.mst
ECHO 3. mst파일 추출완료

ECHO 4. 메인 인스톨러에 mst파일 머지중...
EmbedTransform.exe .\%TARGET_DIR%\multilanguage\%INSTALL_FILE% .\%TARGET_DIR%\multilanguage\ja-jp.mst
EmbedTransform.exe .\%TARGET_DIR%\multilanguage\%INSTALL_FILE% .\%TARGET_DIR%\multilanguage\ko-kr.mst
ECHO 5. mst머지 완료

REM DELETE .\%TARGET_DIR%\multilanguage\ja-jp.mst
REM DELETE .\%TARGET_DIR%\multilanguage\ko-kr.mst

REM msiexec /i .\%TARGET_DIR%\multilanguage\%INSTALL_FILE% TRANSFORMS=":ko-kr.mst"

echo ==========================
echo 국제화 인스톨러 제작 완료
echo ==========================