#pragma once
#include "button.h"

enum class ButtonAction { None, SwitchMode, Recenter, ToggleCompassTheme };
inline ButtonAction buttonAction(DebouncedButton::Event event, bool horizonActive) {
    if (event==DebouncedButton::Event::ShortPress) return ButtonAction::SwitchMode;
    if (event==DebouncedButton::Event::LongPress)
        return horizonActive ? ButtonAction::Recenter : ButtonAction::ToggleCompassTheme;
    return ButtonAction::None;
}
