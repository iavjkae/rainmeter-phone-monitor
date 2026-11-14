local function number(name, fallback)
    return tonumber(SKIN:GetVariable(name)) or fallback
end

local function save(name, value)
    SKIN:Bang('!SetVariable', name, tostring(value))
    SKIN:Bang('!WriteKeyValue', 'Variables', name, tostring(value), SKIN:ReplaceVariables('#@#DisplaySettings.inc'))
end

function Apply()
    local opacity = math.max(0, math.min(100, number('BackgroundOpacity', 0)))
    local black = number('PureBlack', 1) == 1
    local dim = number('LowBrightness', 1) == 1
    SKIN:Bang('!SetVariable', 'BackgroundHex', string.format('%02X', math.floor(opacity * 255 / 100 + 0.5)))
    SKIN:Bang('!SetVariable', 'BlackAlpha', black and '255' or '0')
    local onBlack = black and opacity < 70 and SKIN:GetVariable('Mode') ~= 'OLED'
    local darkForeground = {Text='F1F1EF', Mute='9A9FA8', Soft='30343B', Edge='202329'}
    for _, name in ipairs({'Bg','Card','Text','Mute','Accent','Soft','Second','Edge'}) do
        local base = (onBlack and darkForeground[name]) or SKIN:GetVariable(name .. 'Base')
        local factor = dim and 0.65 or 1
        local r = math.floor(tonumber(base:sub(1,2),16) * factor + 0.5)
        local g = math.floor(tonumber(base:sub(3,4),16) * factor + 0.5)
        local b = math.floor(tonumber(base:sub(5,6),16) * factor + 0.5)
        SKIN:Bang('!SetVariable', name, string.format('%02X%02X%02X', r,g,b))
    end
    SKIN:Bang('!SetOption', 'OpacityControl', 'Text', '#OpacityLabel# ' .. opacity .. '%')
    SKIN:Bang('!SetOption', 'BlackControl', 'Text', black and '#BlackLabel# #OnLabel#' or '#BlackLabel# #OffLabel#')
    SKIN:Bang('!SetOption', 'DimControl', 'Text', dim and '#DimLabel# #OnLabel#' or '#DimLabel# #OffLabel#')
    SKIN:Bang('!UpdateMeter', '*')
    SKIN:Bang('!Redraw')
end

function CycleOpacity()
    save('BackgroundOpacity', (math.floor(number('BackgroundOpacity',0)/25)*25 + 25) % 125)
    Apply()
end

function AdjustOpacity(delta)
    save('BackgroundOpacity', math.max(0, math.min(100, number('BackgroundOpacity',0) + delta)))
    Apply()
end

function ToggleBlack()
    save('PureBlack', number('PureBlack',1)==1 and 0 or 1)
    Apply()
end

function ToggleDim()
    save('LowBrightness', number('LowBrightness',1)==1 and 0 or 1)
    Apply()
end
