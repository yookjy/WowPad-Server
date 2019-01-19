Utils\devcon.exe /r remove @HID\vshid*
Utils\devcon.exe /r remove HID\vshid
Utils\DIFxCmd.exe /u x86\vshid.inf
Utils\devcon.exe install %x86\vshid.inf HID\vshid
