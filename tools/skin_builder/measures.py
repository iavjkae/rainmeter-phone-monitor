"""Measures and the stable presentation field map."""

from pathlib import Path

from .meters import sec


def render_measures(template: Path) -> str:
    data = template.read_text(encoding="utf-8")
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
    return data
