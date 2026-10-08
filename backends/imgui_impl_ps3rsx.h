// dear imgui: Renderer Backend for PlayStation 3 RSX (Reality Synthesizer)
// Target APIs: libgcm / PSL1GHT librsx
// Supports: CFW (Evilnat, Rebug) and HFW (PS3HEN)
// 
// Implemented features:
//  [X] Renderer: User texture binding via gcmTexture*.
//  [X] Renderer: Hardware Scissor test with RSX top-left origin coordinates.
//  [X] Renderer: Alpha blending and orthographic projection matrix setup.
//  [X] Renderer: Dynamic vertex and index buffer triple-buffering in mapped host memory (XDR).
//  [X] Renderer: Font atlas automatic conversion to RSX ARGB8 format with 64-byte row pitch alignment.
//  [X] Renderer: State saving and restoration for clean in-game hooks and overlays.

#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

#ifndef IMGUI_DISABLE

// Forward declarations for RSX types
struct _gcmContextData;
typedef struct _gcmContextData gcmContextData;
struct _gcmTexture;
typedef struct _gcmTexture gcmTexture;

// Structure for saving and restoring RSX state when rendering inside commercial games (SPRX / Hooking)
struct ImGui_ImplPS3RSX_RenderState
{
    void*    SavedVertexProgram;
    void*    SavedFragmentProgram;
    uint32_t SavedFragmentProgramOffset;
    uint8_t  SavedDepthTestEnable;
    uint8_t  SavedDepthMask;
    uint8_t  SavedCullFaceEnable;
    uint8_t  SavedBlendEnable;
    uint32_t SavedBlendEquation;
    uint32_t SavedBlendFuncSrc;
    uint32_t SavedBlendFuncDst;
    uint16_t SavedScissorX, SavedScissorY, SavedScissorW, SavedScissorH;
};

// Main Backend API
IMGUI_IMPL_API bool     ImGui_ImplPS3RSX_Init(gcmContextData* context);
IMGUI_IMPL_API void     ImGui_ImplPS3RSX_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplPS3RSX_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplPS3RSX_RenderDrawData(ImDrawData* draw_data);

// Device objects management
IMGUI_IMPL_API bool     ImGui_ImplPS3RSX_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplPS3RSX_DestroyDeviceObjects();
IMGUI_IMPL_API bool     ImGui_ImplPS3RSX_CreateFontsTexture();
IMGUI_IMPL_API void     ImGui_ImplPS3RSX_DestroyFontsTexture();

// In-Game Hooking / Overlay Support (Preserves game graphics state)
IMGUI_IMPL_API void     ImGui_ImplPS3RSX_SaveRenderState(ImGui_ImplPS3RSX_RenderState* state);
IMGUI_IMPL_API void     ImGui_ImplPS3RSX_RestoreRenderState(const ImGui_ImplPS3RSX_RenderState* state);

#endif // #ifndef IMGUI_DISABLE
