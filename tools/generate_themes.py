from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
S = ROOT / "skins" / "PhoneMonitor"
A = S / "@Resources"

themes = {
    "Blue White": {
        "Bg": "E6EDF6",
        "Card": "F9FBFF",
        "Text": "162137",
        "Mute": "748198",
        "Accent": "268BEB",
        "Soft": "DCE6F3",
        "Second": "8B9FC3",
        "Edge": "D5DEEA",
        "Mode": "BLUE",
    },
    "Cloud": {
        "Bg": "EEEDE8",
        "Card": "FCFCF8",
        "Text": "242B2A",
        "Mute": "808A84",
        "Accent": "608875",
        "Soft": "E4E8E1",
        "Second": "A2B39D",
        "Edge": "E0E2DA",
        "Mode": "CLOUD",
    },
    "Graphite": {
        "Bg": "08090B",
        "Card": "15171B",
        "Text": "F1F1EF",
        "Mute": "858A95",
        "Accent": "F25263",
        "Soft": "282B32",
        "Second": "828B9F",
        "Edge": "202329",
        "Mode": "OLED",
    },
}


def sec(name, **v):
    return "\n[" + name + "]\n" + "".join(str(k) + "=" + str(val) + "\n" for k, val in v.items())


def shape(name, x, y, w, h, color="#Card#", radius=22, stroke=0):
    background = name == "Background" or name.endswith("Card") or name.startswith("ThemePill")
    fill = color + "#BackgroundHex#" if background else color
    edge = "#Edge##BackgroundHex#" if background else "#Edge#"
    return sec(
        name,
        Meter="Shape",
        X=f"({x}*#S#)",
        Y=f"({y}*#S#)",
        Shape=f"Rectangle 0,0,({w}*#S#),({h}*#S#),({radius}*#S#) | Fill Color {fill} | StrokeWidth {stroke} | Stroke Color {edge}",
        AntiAlias=1,
        DynamicVariables=1,
        Group="Backgrounds" if background else "Foreground",
    )


def txt(
    name,
    text,
    x,
    y,
    size=14,
    color="#Text#",
    face="Segoe UI Variable",
    width=None,
    align="Left",
    **kw,
):
    v = {
        "Meter": "String",
        "Text": text,
        "X": f"({x}*#S#)",
        "Y": f"({y}*#S#)",
        "FontFace": face,
        "FontSize": f"({size}*#S#)",
        "FontColor": color,
        "StringAlign": align,
        "AntiAlias": 1,
        "DynamicVariables": 1,
    }
    if width:
        v.update(W=f"({width}*#S#)", H=f"({size * 2.1}*#S#)", ClipString=1)
    v.update(kw)
    return sec(name, **v)


def graph(name, measure, x, y, w, h, color="#Accent#", **kw):
    v = {
        "Meter": "Line",
        "MeasureName": measure,
        "X": f"({x}*#S#)",
        "Y": f"({y}*#S#)",
        "W": f"({w}*#S#)",
        "H": f"({h}*#S#)",
        "LineColor": color,
        "LineWidth": 2,
        "AntiAlias": 1,
        "AutoScale": 1,
        "SolidColor": "0,0,0,0",
        "DynamicVariables": 1,
    }
    v.update(kw)
    return sec(name, **v)


def bar(name, measure, x, y, w):
    return shape(name + "Base", x, y, w, 5, "#Soft#", 2) + sec(
        name,
        Meter="Bar",
        MeasureName=measure,
        X=f"({x}*#S#)",
        Y=f"({y}*#S#)",
        W=f"({w}*#S#)",
        H="(5*#S#)",
        BarColor="#Accent#",
        BarOrientation="Horizontal",
        SolidColor="0,0,0,0",
        DynamicVariables=1,
    )


data = """[Rainmeter]
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
"""
fields = {
    "GPUName": 1,
    "GPUPercent": 2,
    "GPUPower": 3,
    "VRAMRaw": 4,
    "VRAMTotal": 5,
    "GPUTemp": 6,
    "FPSRaw": 7,
    "LowRaw": 8,
    "FrameRaw": 9,
    "GameRaw": 10,
    "FPSLive": 11,
    "GPULive": 12,
    "CPUName": 13,
}
for name, i in fields.items():
    data += sec(
        name,
        Measure="String",
        String="[Telemetry]",
        DynamicVariables=1,
        RegExpSubstitute=1,
        Substitute=f'"(?s)^(?:[^|]*\\|){{{i - 1}}}([^|]*).*$":"\\1","^$":"0"',
    )
data += sec(
    "FPS", Measure="Calc", Formula="0+[FPSRaw]", DynamicVariables=1, MinValue=0, MaxValue=240
)
data += sec("VRAMUsed", Measure="Calc", Formula="0+[VRAMRaw]", DynamicVariables=1)
data += sec(
    "VRAMPercent",
    Measure="Calc",
    Formula="100 * [VRAMRaw] / Max([VRAMTotal],1)",
    DynamicVariables=1,
    MinValue=0,
    MaxValue=100,
)
for name, raw in [("FPSText", "FPSRaw"), ("LowText", "LowRaw")]:
    data += sec(
        name,
        Measure="String",
        String=f"[{raw}]",
        DynamicVariables=1,
        RegExpSubstitute=1,
        Substitute='"\\.\\d+$":"","^0$":"--"',
    )
data += sec(
    "FrameText",
    Measure="String",
    String="[FrameRaw]",
    DynamicVariables=1,
    RegExpSubstitute=1,
    Substitute='"^0\\.0*$":"--"',
)
data += sec(
    "GameText",
    Measure="String",
    String="[GameRaw]",
    DynamicVariables=1,
    RegExpSubstitute=1,
    Substitute='"\\.exe$":"","^0$":"等待游戏","^$":"等待游戏"',
)
data += sec(
    "FPSState",
    Measure="Calc",
    Formula="0+[FPSLive]",
    DynamicVariables=1,
    IfCondition="FPSState < 1",
    IfTrueAction='[!SetOption FPSStateLabel Text "启用帧率采集 →"][!SetOption FPSStateLabel FontColor "#Accent#"]',
    IfFalseAction='[!SetOption FPSStateLabel Text "帧率采集中"][!SetOption FPSStateLabel FontColor "#Mute#"]',
)
(A / "Measures.inc").write_text(data, encoding="utf-16")
for name, palette in themes.items():
    s = (
        "[Variables]\nS=1\nBackgroundHex=FF\nBlackAlpha=0\nOpacityLabel=背景\nBlackLabel=纯黑底\nDimLabel=低亮\nOnLabel=开\nOffLabel=关\n"
        + "".join(
            k + "=" + v + "\n" + (k + "Base=" + v + "\n" if k != "Mode" else "")
            for k, v in palette.items()
        )
        + "\n@IncludeSettings=#@#DisplaySettings.inc\n@Include=#@#Measures.inc\n"
    )
    s += shape("BlackBacking", 0, 0, 1600, 720, "0,0,0,#BlackAlpha#", 0)
    s += shape("Background", 0, 0, 1600, 720, "#Bg#", 0)
    s += sec(
        "LiveDot",
        Meter="Shape",
        X="(42*#S#)",
        Y="(40*#S#)",
        Shape="Ellipse 0,0,(4*#S#) | Fill Color #Accent# | StrokeWidth 0",
        AntiAlias=1,
        DynamicVariables=1,
    )
    s += txt("Header", "PERFORMANCE", 60, 23, 19, face="Bahnschrift", CharacterSpacing=3)
    s += txt("Model", "[GPUName]", 310, 29, 12, "#Mute#")
    s += txt(
        "OpacityControl",
        "背景 0%",
        740,
        28,
        11,
        "#Mute#",
        LeftMouseUpAction='[!CommandMeasure DisplayControl "CycleOpacity()"]',
        MouseScrollUpAction='[!CommandMeasure DisplayControl "AdjustOpacity(10)"]',
        MouseScrollDownAction='[!CommandMeasure DisplayControl "AdjustOpacity(-10)"]',
        ToolTipText="点击切换背景不透明度；滚轮每次调节 10%。文字不受影响。",
    )
    s += txt(
        "BlackControl",
        "纯黑底 开",
        895,
        28,
        11,
        "#Mute#",
        LeftMouseUpAction='[!CommandMeasure DisplayControl "ToggleBlack()"]',
        ToolTipText="开：透明区域显示纯黑；关：透明区域露出桌面。",
    )
    s += txt(
        "DimControl",
        "低亮 开",
        1050,
        28,
        11,
        "#Mute#",
        LeftMouseUpAction='[!CommandMeasure DisplayControl "ToggleDim()"]',
        ToolTipText="降低文字、曲线及面板颜色亮度；手机屏幕亮度仍需在手机上调节。",
    )
    for j, (other, label) in enumerate(
        [("Blue White", "BLUE"), ("Cloud", "CLOUD"), ("Graphite", "OLED")]
    ):
        x = 1200 + j * 120
        s += shape("ThemePill" + str(j), x, 20, 108, 38, "#Card#" if other == name else "#Bg#", 19)
        s += txt(
            "Theme" + str(j),
            label,
            x + 54,
            29,
            10,
            "#Accent#" if other == name else "#Mute#",
            align="Center",
            LeftMouseUpAction=f'[!ActivateConfig "PhoneMonitor" "{other}.ini"]',
            ToolTipText=other,
        )
    for card, x, y, w, h in [
        ("FPSCard", 40, 94, 460, 552),
        ("GPUCard", 520, 94, 620, 316),
        ("CPUCard", 1160, 94, 400, 316),
        ("RAMCard", 520, 430, 300, 216),
        ("VRAMCard", 840, 430, 300, 216),
        ("NetCard", 1160, 430, 400, 216),
    ]:
        s += shape(card, x, y, w, h, stroke=1 if name == "Graphite" else 0)
    # Hero FPS: primary value, live history and two secondary metrics.
    s += txt("FPSLabel", "GAME FPS", 72, 124, 13, "#Mute#", face="Bahnschrift", CharacterSpacing=2)
    s += txt("Game", "[GameText]", 72, 158, 15, width=390)
    s += txt("FPSValue", "[FPSText]", 64, 202, 118, "#Text#", face="Bahnschrift", width=405)
    s += txt("FPSUnit", "FRAMES / SECOND", 74, 354, 10, "#Mute#", CharacterSpacing=2)
    s += graph("FPSHistory", "FPS", 74, 403, 390, 66)
    s += shape("FPSDivider", 74, 493, 390, 1, "#Soft#", 0)
    s += txt("LowLabel", "1% LOW · 30s", 74, 520, 10, "#Mute#")
    s += txt("LowValue", "[LowText]", 74, 551, 31, face="Bahnschrift")
    s += txt("FrameLabel", "FRAME TIME", 292, 520, 10, "#Mute#")
    s += txt("FrameValue", "[FrameText] ms", 292, 555, 25, face="Bahnschrift")
    # GPU card: usage, board power, temperature, and history.
    s += txt("GPUHeader", "GPU", 552, 124, 13, "#Mute#", face="Bahnschrift", CharacterSpacing=2)
    s += txt("GPUValue", "[GPUPercent]%", 548, 159, 65, face="Bahnschrift")
    s += txt("PowerLabel", "POWER", 836, 160, 10, "#Mute#")
    s += txt("PowerValue", "[GPUPower] W", 836, 185, 29, face="Bahnschrift")
    s += txt("TempLabel", "TEMPERATURE", 994, 160, 9, "#Mute#")
    s += txt("TempValue", "[GPUTemp]°C", 994, 187, 25, face="Bahnschrift")
    s += bar("GPUBar", "Telemetry", 554, 266, 550)
    s += graph("GPUHistory", "Telemetry", 554, 305, 550, 69)
    # CPU card: load and frequency.
    s += txt("CPUHeader", "CPU", 1192, 124, 13, "#Mute#", face="Bahnschrift", CharacterSpacing=2)
    s += txt(
        "CPUValue", "%1%", 1188, 161, 65, face="Bahnschrift", MeasureName="CPU", NumOfDecimals=0
    )
    s += txt(
        "CPUClock",
        "%1 GHz",
        1530,
        218,
        15,
        "#Mute#",
        align="Right",
        MeasureName="ClockGHz",
        NumOfDecimals=2,
    )
    s += txt("CPUModel", "[CPUName]", 1192, 266, 11, "#Mute#", width=337)
    s += graph("CPUHistory", "CPU", 1194, 307, 332, 67)
    # Memory and dedicated graphics memory.
    for prefix, x, label, measure, total, ratio in [
        ("RAM", 520, "MEMORY", "RAMGB", "RAMTotalGB", "RAMPercent"),
        ("VRAM", 840, "VRAM", "VRAMUsed", "VRAMTotal", "VRAMPercent"),
    ]:
        s += txt(
            prefix + "Label",
            label,
            x + 30,
            458,
            12,
            "#Mute#",
            face="Bahnschrift",
            CharacterSpacing=2,
        )
        s += txt(
            prefix + "Value",
            "%1 GB",
            x + 27,
            495,
            36,
            face="Bahnschrift",
            MeasureName=measure,
            NumOfDecimals=1,
        )
        if prefix == "RAM":
            s += txt(
                prefix + "TotalLabel",
                "of %1 GB",
                x + 31,
                554,
                12,
                "#Mute#",
                MeasureName=total,
                NumOfDecimals=0,
            )
        else:
            s += txt(prefix + "TotalLabel", "of [VRAMTotal] GB", x + 31, 554, 12, "#Mute#")
        s += bar(prefix + "Bar", ratio, x + 31, 602, 238)
    # Network card.
    s += txt("NetLabel", "NETWORK", 1192, 458, 12, "#Mute#", face="Bahnschrift", CharacterSpacing=2)
    s += txt("DownIcon", "↓", 1190, 496, 24, "#Accent#")
    s += txt(
        "DownValue",
        "%1B/s",
        1233,
        501,
        25,
        face="Bahnschrift",
        MeasureName="NetIn",
        AutoScale=1,
        NumOfDecimals=1,
    )
    s += txt("UpIcon", "↑", 1190, 544, 24, "#Second#")
    s += txt(
        "UpValue",
        "%1B/s",
        1233,
        549,
        25,
        face="Bahnschrift",
        MeasureName="NetOut",
        AutoScale=1,
        NumOfDecimals=1,
    )
    s += graph(
        "NetHistory",
        "NetIn",
        1193,
        601,
        333,
        20,
        LineCount=2,
        MeasureName2="NetOut",
        LineColor2="#Second#",
        LineWidth=1,
    )
    s += txt(
        "FPSStateLabel",
        "启用帧率采集 →",
        42,
        676,
        11,
        "#Accent#",
        LeftMouseUpAction='["powershell.exe" "-NoProfile" "-ExecutionPolicy" "RemoteSigned" "-WindowStyle" "Hidden" "-File" "#@#start_fps.ps1"]',
    )
    s += txt("Footer", "LIVE / 1s", 800, 678, 10, "#Mute#", align="Center")
    s += txt("FooterTime", "[Time]", 1558, 672, 17, "#Mute#", face="Bahnschrift", align="Right")
    (S / (name + ".ini")).write_text(s, encoding="utf-16")
print("Generated three 1600x720 Rainmeter themes.")
