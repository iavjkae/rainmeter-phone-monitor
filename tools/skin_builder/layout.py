"""Compose layout regions from meter primitives."""

from .meters import bar, graph, sec, shape, txt


def render_header(name, palette, palettes) -> str:
    s = (
        "[Variables]\nS=1\nBackgroundHex=FF\nBlackAlpha=0\nOpacityLabel=背景\nBlackLabel=纯黑底\nDimLabel=低亮\nOnLabel=开\nOffLabel=关\n"
        + "".join(
            (
                k + "=" + v + "\n" + (k + "Base=" + v + "\n" if k != "Mode" else "")
                for k, v in palette.items()
            )
        )
        + "\n@IncludeSettings=#@#DisplaySettings.inc\n@IncludeTelemetry=#@#TelemetrySettings.inc\n@Include=#@#Measures.inc\n"
    )
    s += shape("BlackBacking", 0, 0, 1600, 720, "0,0,0,#BlackAlpha#", 0)
    s += shape("Background", 0, 0, 1600, 720, "#Bg#", 0, background=True)
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
        s += shape(
            "ThemePill" + str(j),
            x,
            20,
            108,
            38,
            "#Card#" if other == name else "#Bg#",
            19,
            background=True,
        )
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
        s += shape(card, x, y, w, h, stroke=1 if name == "Graphite" else 0, background=True)
    return s


def render_fps() -> str:
    s = ""
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
    return s


def render_gpu() -> str:
    s = ""
    s += txt("GPUHeader", "GPU", 552, 124, 13, "#Mute#", face="Bahnschrift", CharacterSpacing=2)
    s += txt("GPUValue", "[GPUPercent]%", 548, 159, 65, face="Bahnschrift")
    s += txt("PowerLabel", "POWER", 836, 160, 10, "#Mute#")
    s += txt("PowerValue", "[GPUPower] W", 836, 185, 29, face="Bahnschrift")
    s += txt("TempLabel", "TEMPERATURE", 994, 160, 9, "#Mute#")
    s += txt("TempValue", "[GPUTemp]°C", 994, 187, 25, face="Bahnschrift")
    s += bar("GPUBar", "Telemetry", 554, 266, 550)
    s += graph("GPUHistory", "Telemetry", 554, 305, 550, 69)
    return s


def render_cpu() -> str:
    s = ""
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
    return s


def render_memory() -> str:
    s = ""
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
    return s


def render_network() -> str:
    s = ""
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
    return s


def render_footer() -> str:
    s = ""
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
    return s


def render_layout(name, palette, palettes) -> str:
    return (
        render_header(name, palette, palettes)
        + render_fps()
        + render_gpu()
        + render_cpu()
        + render_memory()
        + render_network()
        + render_footer()
    )
