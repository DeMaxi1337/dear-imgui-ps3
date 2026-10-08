// Dear ImGui: Standalone Example for PlayStation 3
// Compatible with: CFW (Evilnat, Rebug) and HFW (PS3HEN)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "imgui.h"
#include "imgui_impl_ps3rsx.h"
#include "imgui_impl_ps3pad.h"

#if defined(__PSL1GHT__) || defined(PSL1GHT)
#include <rsx/rsx.h>
#include <rsx/gcm_sys.h>
#include <io/pad.h>
#include <sysutil/video.h>
#else
#include <cell/gcm.h>
#include <cell/pad.h>
#include <cell/video_out.h>
#endif

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720
#define CB_SIZE (0x80000)

static gcmContextData*  g_GcmContext = nullptr;
static void*            g_CommandBuffer = nullptr;
static uint32_t         g_DisplayBuffers[2];
static int              g_CurrentBuffer = 0;

static bool InitGraphics()
{
    g_CommandBuffer = memalign(1024 * 1024, CB_SIZE);
    if (!g_CommandBuffer)
        return false;

#if defined(__PSL1GHT__) || defined(PSL1GHT)
    g_GcmContext = rsxInit(CB_SIZE, g_CommandBuffer);
    if (!g_GcmContext)
        return false;

    videoConfiguration video_cfg;
    memset(&video_cfg, 0, sizeof(video_cfg));
    video_cfg.resolution = VIDEO_RESOLUTION_720;
    video_cfg.format     = VIDEO_BUFFER_FORMAT_XRGB;
    video_cfg.pitch      = SCREEN_WIDTH * 4;
    videoConfigure(&video_cfg);

    for (int i = 0; i < 2; i++)
    {
        uint32_t buf_size = SCREEN_WIDTH * SCREEN_HEIGHT * 4;
        void* mem = rsxMemalign(64, buf_size);
        rsxAddressToOffset(mem, &g_DisplayBuffers[i]);
        gcmSetDisplayBuffer(i, g_DisplayBuffers[i], video_cfg.pitch, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    gcmResetFlipStatus();
#endif

    return true;
}

static void SwapBuffers()
{
#if defined(__PSL1GHT__) || defined(PSL1GHT)
    gcmSetFlip(g_GcmContext, g_CurrentBuffer);
    rsxFlushBuffer(g_GcmContext);
    gcmSetWaitFlip(g_GcmContext);

    g_CurrentBuffer = !g_CurrentBuffer;
#endif
}

int main(int argc, char* argv[])
{
    if (!InitGraphics())
        return -1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui_ImplPS3Pad_Init(1);
    ImGui_ImplPS3RSX_Init(g_GcmContext);

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 4.0f;

    bool running = true;
    while (running)
    {
        ImGui_ImplPS3Pad_NewFrame();
        ImGui_ImplPS3RSX_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        {
            ImGui::Begin("PlayStation 3 Dear ImGui Port", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Running on PS3 (Cell BE + RSX)!");
            ImGui::Separator();

            ImGui::Text("Framerate: %.1f FPS (%.2f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
            ImGui::Text("Display: %d x %d", SCREEN_WIDTH, SCREEN_HEIGHT);
            ImGui::Separator();

            ImGui::Text("Controls:");
            ImGui::BulletText("Right Stick: Move Virtual Mouse Cursor");
            ImGui::BulletText("Cross (X): Left Click");
            ImGui::BulletText("Circle (O): Right Click");
            ImGui::BulletText("L3 + R3: Toggle Menu Open/Close");
            ImGui::BulletText("D-Pad: Native Gamepad Navigation");
            ImGui::Separator();

            static float clear_color[4] = { 0.1f, 0.1f, 0.12f, 1.0f };
            ImGui::ColorEdit3("Background Clear Color", clear_color);

            static int counter = 0;
            if (ImGui::Button("Click Me!"))
                counter++;
            ImGui::SameLine();
            ImGui::Text("Count = %d", counter);

            if (ImGui::Button("Exit Application"))
                running = false;

            ImGui::End();
        }

        ImGui_ImplPS3Pad_RenderMouseCursor();

        ImGui::Render();
        ImGui_ImplPS3RSX_RenderDrawData(ImGui::GetDrawData());

        SwapBuffers();
    }

    ImGui_ImplPS3RSX_Shutdown();
    ImGui_ImplPS3Pad_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
