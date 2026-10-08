// Dear ImGui: In-Game Hook / Trainer Framework Template for PS3 (SPRX Plugin)
// Compatible with: CFW (Cobra) and HFW (PS3HEN)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prx.h>
#include <sys/ppu_thread.h>

#include "imgui.h"
#include "imgui_impl_ps3rsx.h"
#include "imgui_impl_ps3pad.h"

#include <cell/gcm.h>
#include <cell/pad.h>

SYS_MODULE_INFO(ImGuiPS3Hook, 0, 1, 1);
SYS_MODULE_START(prx_start);
SYS_MODULE_STOP(prx_stop);

static bool g_GodMode         = false;
static bool g_InfiniteAmmo    = false;
static float g_PlayerSpeed    = 1.0f;
static float g_MenuColor[4]   = { 0.45f, 0.35f, 0.85f, 1.00f };

static void DrawUserCheatMenu()
{
    if (!ImGui_ImplPS3Pad_IsMenuOpen())
        return;

    ImGui::SetNextWindowSize(ImVec2(500, 360), ImGuiCond_FirstUseEver);
    ImGui::Begin("PS3 In-Game Trainer / Debugger", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "SPRX In-Game Hook Active!");
    ImGui::Text("Press L3 + R3 on DualShock 3 to toggle");
    ImGui::Separator();

    if (ImGui::BeginTabBar("Tabs"))
    {
        if (ImGui::BeginTabItem("Cheats"))
        {
            if (ImGui::Checkbox("God Mode", &g_GodMode))
            {
                // Patch game memory
            }

            if (ImGui::Checkbox("Infinite Ammo", &g_InfiniteAmmo))
            {
                // Patch game memory
            }

            ImGui::SliderFloat("Player Speed", &g_PlayerSpeed, 0.5f, 5.0f, "%.2fx");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Settings"))
        {
            ImGui::ColorEdit4("Accent Color", g_MenuColor);

            ImGui_ImplPS3Pad_Config& cfg = ImGui_ImplPS3Pad_GetConfig();
            ImGui::SliderFloat("Cursor Speed", &cfg.VirtualMouseSpeed, 300.0f, 1500.0f);
            ImGui::SliderFloat("Stick Deadzone", &cfg.Deadzone, 0.05f, 0.40f);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();

    ImGui_ImplPS3Pad_RenderMouseCursor();
}

static int32_t (*orig_cellGcmSetFlip)(gcmContextData* context, uint8_t id) = nullptr;

static int32_t Hooked_cellGcmSetFlip(gcmContextData* context, uint8_t id)
{
    static bool imgui_initialized = false;
    if (!imgui_initialized)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1280.0f, 720.0f);

        ImGui_ImplPS3Pad_Init(1);
        ImGui_ImplPS3RSX_Init(context);

        ImGui_ImplPS3Pad_SetMenuOpen(false);
        imgui_initialized = true;
    }

    ImGui_ImplPS3Pad_NewFrame();
    ImGui_ImplPS3RSX_NewFrame();
    ImGui::NewFrame();

    DrawUserCheatMenu();

    ImGui::Render();

    if (ImGui_ImplPS3Pad_IsMenuOpen())
    {
        ImGui_ImplPS3RSX_RenderState state;
        ImGui_ImplPS3RSX_SaveRenderState(&state);

        ImGui_ImplPS3RSX_RenderDrawData(ImGui::GetDrawData());

        ImGui_ImplPS3RSX_RestoreRenderState(&state);
    }

    return orig_cellGcmSetFlip ? orig_cellGcmSetFlip(context, id) : 0;
}

static int32_t (*orig_cellPadGetData)(uint32_t port, CellPadData* data) = nullptr;

static int32_t Hooked_cellPadGetData(uint32_t port, CellPadData* data)
{
    int32_t ret = orig_cellPadGetData ? orig_cellPadGetData(port, data) : 0;
    if (ret == 0 && data)
    {
        ImGui_ImplPS3Pad_FilterGameInput(data);
    }
    return ret;
}

extern "C" int prx_start(size_t args, void *argp)
{
    return SYS_PRX_RESIDENT;
}

extern "C" int prx_stop(void)
{
    ImGui_ImplPS3RSX_Shutdown();
    ImGui_ImplPS3Pad_Shutdown();
    ImGui::DestroyContext();
    return SYS_PRX_STOP_OK;
}
