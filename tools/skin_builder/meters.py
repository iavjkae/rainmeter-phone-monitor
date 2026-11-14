"""Small composable Rainmeter meter primitives."""


def sec(name, **v):
    return "\n[" + name + "]\n" + "".join(str(k) + "=" + str(val) + "\n" for k, val in v.items())


def shape(name, x, y, w, h, color="#Card#", radius=22, stroke=0, background=False):
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
