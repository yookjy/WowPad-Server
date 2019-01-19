Utils\devcon64.exe /r remove @HID\vshid*
Utils\devcon64.exe /r remove HID\vshid
Utils\DIFxCmd64.exe /u x64\vshid.inf
Utils\devcon64.exe install x64\vshid.inf HID\vshid

