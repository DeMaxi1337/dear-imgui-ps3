// dear imgui: Platform/Input Backend for PlayStation 3 DualShock 3 / Sixaxis
// Target APIs: cellPad (<cell/pad.h> or PSL1GHT <io/pad.h>)
// Supports: CFW (Evilnat, Rebug) and HFW (PS3HEN)

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_ps3pad.h"
#include <math.h>
#include <string.h>

#if defined(__PSL1GHT__) || defined(PSL1GHT)
#include <io/pad.h>
#else
#include <cell/pad.h>
#endif

#ifndef CELL_PAD_CTRL_SELECT
#define CELL_PAD_CTRL_SELECT    0x0001
#define CELL_PAD_CTRL_L3        0x0002
#define CELL_PAD_CTRL_R3        0x0004
#define CELL_PAD_CTRL_START     0x0008
#define CELL_PAD_CTRL_UP        0x0010
#define CELL_PAD_CTRL_RIGHT     0x0020
#define CELL_PAD_CTRL_DOWN      0x0040
#define CELL_PAD_CTRL_LEFT      0x0080

#define CELL_PAD_CTRL_L2        0x0100
#define CELL_PAD_CTRL_R2        0x0200
#define CELL_PAD_CTRL_L1        0x0400
#define CELL_PAD_CTRL_R1        0x0800
#define CELL_PAD_CTRL_TRIANGLE  0x1000
#define CELL_PAD_CTRL_CIRCLE    0x2000
#define CELL_PAD_CTRL_CROSS     0x4000
#define CELL_PAD_CTRL_SQUARE    0x8000

#define CELL_PAD_BTN_OFFSET_DIGITAL1        2
#define CELL_PAD_BTN_OFFSET_DIGITAL2        3
#define CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X  4
#define CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y  5
#define CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X   6
#define CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y   7
#endif

ImGui_ImplPS3Pad_Config::ImGui_ImplPS3Pad_Config()
{
    EnableVirtualMouse  = true;
    VirtualMouseStick   = IMGUI_PS3PAD_STICK_RIGHT;
    VirtualMouseSpeed   = 850.0f;
    Deadzone            = 0.18f;
    
    ToggleDigital1Mask  = CELL_PAD_CTRL_L3 | CELL_PAD_CTRL_R3;
    ToggleDigital2Mask  = 0;
}

struct ImGui_ImplPS3Pad_Data
{
    ImGui_ImplPS3Pad_Config Config;
    bool                    MenuOpen;
    bool                    PrevComboState;
    ImVec2                  VirtualCursorPos;
    float                   PrevScrollTime;

    ImGui_ImplPS3Pad_Data()
    {
        MenuOpen = true;
        PrevComboState = false;
        VirtualCursorPos = ImVec2(640.0f, 360.0f);
        PrevScrollTime = 0.0f;
    }
};

static ImGui_ImplPS3Pad_Data* ImGui_ImplPS3Pad_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplPS3Pad_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

static void ApplyRadialDeadzone(float in_x, float in_y, float deadzone, float& out_x, float& out_y)
{
    float mag = sqrtf(in_x * in_x + in_y * in_y);
    if (mag <= deadzone)
    {
        out_x = 0.0f;
        out_y = 0.0f;
    }
    else
    {
        float factor = (mag - deadzone) / (1.0f - deadzone);
        factor = factor * factor;
        out_x = (in_x / mag) * factor;
        out_y = (in_y / mag) * factor;
        out_x = ImClamp(out_x, -1.0f, 1.0f);
        out_y = ImClamp(out_y, -1.0f, 1.0f);
    }
}

bool ImGui_ImplPS3Pad_Init(int max_ports)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    ImGui_ImplPS3Pad_Data* bd = IM_NEW(ImGui_ImplPS3Pad_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_ps3pad";
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    io.ConfigFlags  |= ImGuiConfigFlags_NavEnableGamepad;

#if defined(__PSL1GHT__) || defined(PSL1GHT)
    ioPadInit(max_ports);
#else
    cellPadInit(max_ports);
#endif

    return true;
}

void ImGui_ImplPS3Pad_Shutdown()
{
    ImGui_ImplPS3Pad_Data* bd = ImGui_ImplPS3Pad_GetBackendData();
    IM_ASSERT(bd != nullptr && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

#if defined(__PSL1GHT__) || defined(PSL1GHT)
    ioPadEnd();
#else
    cellPadEnd();
#endif

    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    IM_DELETE(bd);
}

ImGui_ImplPS3Pad_Config& ImGui_ImplPS3Pad_GetConfig()
{
    ImGui_ImplPS3Pad_Data* bd = ImGui_ImplPS3Pad_GetBackendData();
    return bd->Config;
}

bool ImGui_ImplPS3Pad_IsMenuOpen()
{
    ImGui_ImplPS3Pad_Data* bd = ImGui_ImplPS3Pad_GetBackendData();
    return bd ? bd->MenuOpen : false;
}

void ImGui_ImplPS3Pad_SetMenuOpen(bool open)
{
    ImGui_ImplPS3Pad_Data* bd = ImGui_ImplPS3Pad_GetBackendData();
    if (bd) bd->MenuOpen = open;
}

void ImGui_ImplPS3Pad_ToggleMenu()
{
    ImGui_ImplPS3Pad_Data* bd = ImGui_ImplPS3Pad_GetBackendData();
    if (bd) bd->MenuOpen = !bd->MenuOpen;
}

void ImGui_ImplPS3Pad_FilterGameInput(CellPadData* pad_data)
{
    ImGui_ImplPS3Pad_Data* bd = ImGui_ImplPS3Pad_GetBackendData();
    if (bd && bd->MenuOpen && pad_data && pad_data->len > 0)
    {
        pad_data->button[CELL_PAD_BTN_OFFSET_DIGITAL1] = 0;
        pad_data->button[CELL_PAD_BTN_OFFSET_DIGITAL2] = 0;
        pad_data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] = 128;
        pad_data->button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] = 128;
        pad_data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X]  = 128;
        pad_data->button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y]  = 128;
    }
}

void ImGui_ImplPS3Pad_NewFrame(float delta_time)
{
    ImGui_ImplPS3Pad_Data* bd = ImGui_ImplPS3Pad_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized!");
    ImGuiIO& io = ImGui::GetIO();

    if (delta_time > 0.0f)
        io.DeltaTime = delta_time;
    else if (io.DeltaTime <= 0.0f)
        io.DeltaTime = 1.0f / 60.0f;

    CellPadData pad;
    memset(&pad, 0, sizeof(pad));

#if defined(__PSL1GHT__) || defined(PSL1GHT)
    int ret = ioPadGetData(0, &pad);
#else
    int ret = cellPadGetData(0, &pad);
#endif

    if (ret != 0 || pad.len == 0)
        return;

    uint16_t d1 = pad.button[CELL_PAD_BTN_OFFSET_DIGITAL1];
    uint16_t d2 = pad.button[CELL_PAD_BTN_OFFSET_DIGITAL2];

    // Toggle Check (L3 + R3)
    bool combo_match = true;
    if (bd->Config.ToggleDigital1Mask != 0 && (d1 & bd->Config.ToggleDigital1Mask) != bd->Config.ToggleDigital1Mask)
        combo_match = false;
    if (bd->Config.ToggleDigital2Mask != 0 && (d2 & bd->Config.ToggleDigital2Mask) != bd->Config.ToggleDigital2Mask)
        combo_match = false;

    if (combo_match && !bd->PrevComboState)
    {
        bd->MenuOpen = !bd->MenuOpen;
    }
    bd->PrevComboState = combo_match;

    if (!bd->MenuOpen)
        return;

    // Keys
    io.AddKeyEvent(ImGuiKey_GamepadDpadUp,        (d1 & CELL_PAD_CTRL_UP) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadDpadDown,      (d1 & CELL_PAD_CTRL_DOWN) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadDpadLeft,      (d1 & CELL_PAD_CTRL_LEFT) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadDpadRight,     (d1 & CELL_PAD_CTRL_RIGHT) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadStart,         (d1 & CELL_PAD_CTRL_START) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadBack,          (d1 & CELL_PAD_CTRL_SELECT) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadL3,            (d1 & CELL_PAD_CTRL_L3) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadR3,            (d1 & CELL_PAD_CTRL_R3) != 0);

    io.AddKeyEvent(ImGuiKey_GamepadFaceDown,      (d2 & CELL_PAD_CTRL_CROSS) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadFaceRight,     (d2 & CELL_PAD_CTRL_CIRCLE) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadFaceLeft,      (d2 & CELL_PAD_CTRL_SQUARE) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadFaceUp,        (d2 & CELL_PAD_CTRL_TRIANGLE) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadL1,            (d2 & CELL_PAD_CTRL_L1) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadR1,            (d2 & CELL_PAD_CTRL_R1) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadL2,            (d2 & CELL_PAD_CTRL_L2) != 0);
    io.AddKeyEvent(ImGuiKey_GamepadR2,            (d2 & CELL_PAD_CTRL_R2) != 0);

    // Left Stick Navigation
    float raw_lx = ((float)pad.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X] - 128.0f) / 127.0f;
    float raw_ly = ((float)pad.button[CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y] - 128.0f) / 127.0f;
    float filt_lx = 0.0f, filt_ly = 0.0f;
    ApplyRadialDeadzone(raw_lx, raw_ly, bd->Config.Deadzone, filt_lx, filt_ly);

    io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickLeft,  filt_lx < 0.0f, -filt_lx);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickRight, filt_lx > 0.0f,  filt_lx);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickUp,    filt_ly < 0.0f, -filt_ly);
    io.AddKeyAnalogEvent(ImGuiKey_GamepadLStickDown,  filt_ly > 0.0f,  filt_ly);

    // Virtual Mouse Cursor
    if (bd->Config.EnableVirtualMouse)
    {
        float stick_x = 0.0f, stick_y = 0.0f;
        if (bd->Config.VirtualMouseStick == IMGUI_PS3PAD_STICK_RIGHT)
        {
            float raw_rx = ((float)pad.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X] - 128.0f) / 127.0f;
            float raw_ry = ((float)pad.button[CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y] - 128.0f) / 127.0f;
            ApplyRadialDeadzone(raw_rx, raw_ry, bd->Config.Deadzone, stick_x, stick_y);
        }
        else
        {
            stick_x = filt_lx;
            stick_y = filt_ly;
        }

        bd->VirtualCursorPos.x += stick_x * bd->Config.VirtualMouseSpeed * io.DeltaTime;
        bd->VirtualCursorPos.y += stick_y * bd->Config.VirtualMouseSpeed * io.DeltaTime;
        bd->VirtualCursorPos.x = ImClamp(bd->VirtualCursorPos.x, 0.0f, io.DisplaySize.x);
        bd->VirtualCursorPos.y = ImClamp(bd->VirtualCursorPos.y, 0.0f, io.DisplaySize.y);

        io.AddMousePosEvent(bd->VirtualCursorPos.x, bd->VirtualCursorPos.y);

        io.AddMouseButtonEvent(0, (d2 & CELL_PAD_CTRL_CROSS) != 0);
        io.AddMouseButtonEvent(1, (d2 & CELL_PAD_CTRL_CIRCLE) != 0);

        float wheel = 0.0f;
        if (d2 & CELL_PAD_CTRL_R1) wheel += 1.0f;
        if (d2 & CELL_PAD_CTRL_L1) wheel -= 1.0f;
        if (wheel != 0.0f)
            io.AddMouseWheelEvent(0.0f, wheel * 5.0f * io.DeltaTime * 60.0f);
    }
}

void ImGui_ImplPS3Pad_RenderMouseCursor()
{
    ImGui_ImplPS3Pad_Data* bd = ImGui_ImplPS3Pad_GetBackendData();
    if (!bd || !bd->MenuOpen || !bd->Config.EnableVirtualMouse)
        return;

    ImDrawList* fg_list = ImGui::GetForegroundDrawList();
    ImVec2 pos = bd->VirtualCursorPos;

    const ImVec2 p1 = pos;
    const ImVec2 p2 = ImVec2(pos.x + 15.0f, pos.y + 15.0f);
    const ImVec2 p3 = ImVec2(pos.x,        pos.y + 20.0f);
    const ImVec2 p4 = ImVec2(pos.x + 6.0f,  pos.y + 13.0f);

    fg_list->AddTriangle(p1, p2, p4, IM_COL32(0, 0, 0, 255), 2.5f);
    fg_list->AddTriangle(p1, p4, p3, IM_COL32(0, 0, 0, 255), 2.5f);

    fg_list->AddTriangleFilled(p1, p2, p4, IM_COL32(255, 255, 255, 255));
    fg_list->AddTriangleFilled(p1, p4, p3, IM_COL32(230, 230, 230, 255));
}

#endif // #ifndef IMGUI_DISABLE
