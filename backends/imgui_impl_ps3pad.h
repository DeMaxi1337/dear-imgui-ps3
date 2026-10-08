// dear imgui: Platform/Input Backend for PlayStation 3 DualShock 3 / Sixaxis
// Target APIs: cellPad (<cell/pad.h> or PSL1GHT <io/pad.h>)
// Supports: CFW (Evilnat, Rebug) and HFW (PS3HEN)

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

#ifndef IMGUI_DISABLE

#if defined(__PSL1GHT__) || defined(PSL1GHT)
#include <io/pad.h>
typedef padData CellPadData;
#else
struct CellPadData;
#endif

enum ImGuiPS3Pad_Stick
{
    IMGUI_PS3PAD_STICK_LEFT = 0,
    IMGUI_PS3PAD_STICK_RIGHT = 1
};

struct ImGui_ImplPS3Pad_Config
{
    bool                EnableVirtualMouse;
    ImGuiPS3Pad_Stick   VirtualMouseStick;
    float               VirtualMouseSpeed;
    float               Deadzone;
    
    uint16_t            ToggleDigital1Mask;
    uint16_t            ToggleDigital2Mask;
    
    ImGui_ImplPS3Pad_Config();
};

IMGUI_IMPL_API bool     ImGui_ImplPS3Pad_Init(int max_ports = 1);
IMGUI_IMPL_API void     ImGui_ImplPS3Pad_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplPS3Pad_NewFrame(float delta_time = 0.0f);

IMGUI_IMPL_API ImGui_ImplPS3Pad_Config& ImGui_ImplPS3Pad_GetConfig();
IMGUI_IMPL_API bool     ImGui_ImplPS3Pad_IsMenuOpen();
IMGUI_IMPL_API void     ImGui_ImplPS3Pad_SetMenuOpen(bool open);
IMGUI_IMPL_API void     ImGui_ImplPS3Pad_ToggleMenu();

IMGUI_IMPL_API void     ImGui_ImplPS3Pad_FilterGameInput(CellPadData* pad_data);
IMGUI_IMPL_API void     ImGui_ImplPS3Pad_RenderMouseCursor();

#endif // #ifndef IMGUI_DISABLE
