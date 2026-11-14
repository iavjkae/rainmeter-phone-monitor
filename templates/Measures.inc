[Rainmeter]
Update=1000
AccurateText=1
DynamicWindowSize=0
SkinWidth=(1600*#S#)
SkinHeight=(720*#S#)
ContextTitle=Blue White
ContextAction=[!ActivateConfig "PhoneMonitor" "Blue White.ini"]
ContextTitle2=Cloud
ContextAction2=[!ActivateConfig "PhoneMonitor" "Cloud.ini"]
ContextTitle3=Graphite
ContextAction3=[!ActivateConfig "PhoneMonitor" "Graphite.ini"]
ContextTitle4=启用游戏帧率
ContextAction4=["powershell.exe" "-NoProfile" "-ExecutionPolicy" "RemoteSigned" "-WindowStyle" "Hidden" "-File" "#@#start_fps.ps1"]
ContextTitle5=恢复此前桌面
ContextAction5=[!Manage "Layouts"]
OnRefreshAction=[!CommandMeasure DisplayControl "Apply()"]

[DisplayControl]
Measure=Script
ScriptFile=#@#DisplayControl.lua
UpdateDivider=-1

[Telemetry]
Measure=Plugin
Plugin=PhoneTelemetry
FPSStateFile=#@#fps-state.txt
GPUIndex=#GPUIndex#
FPSFreshnessMs=#FPSFreshnessMs#
MinValue=0
MaxValue=100

[CPU]
Measure=CPU

[ClockMHz]
Measure=Plugin
Plugin=PowerPlugin
PowerState=MHZ

[ClockGHz]
Measure=Calc
Formula=ClockMHz / 1000

[RAMUsed]
Measure=PhysicalMemory

[RAMTotal]
Measure=PhysicalMemory
Total=1
UpdateDivider=-1

[RAMGB]
Measure=Calc
Formula=RAMUsed / 1073741824

[RAMTotalGB]
Measure=Calc
Formula=RAMTotal / 1073741824

[RAMPercent]
Measure=Calc
Formula=100 * RAMUsed / Max(RAMTotal,1)
MinValue=0
MaxValue=100

[NetIn]
Measure=NetIn

[NetOut]
Measure=NetOut

[Time]
Measure=Time
Format=%H:%M
