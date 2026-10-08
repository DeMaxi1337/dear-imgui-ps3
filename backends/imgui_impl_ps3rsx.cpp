// dear imgui: Renderer Backend for PlayStation 3 RSX (Reality Synthesizer)
// Target APIs: libgcm / PSL1GHT librsx
// Supports: CFW (Evilnat, Rebug) and HFW (PS3HEN)

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_ps3rsx.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#if defined(__PSL1GHT__) || defined(PSL1GHT)
#include <rsx/rsx.h>
#include <rsx/gcm_sys.h>
#include <rsx/commands.h>

#define CELL_GCM_LOCATION_MAIN                  GCM_LOCATION_CELL
#define CELL_GCM_LOCATION_LOCAL                 GCM_LOCATION_RSX
#define CELL_GCM_FALSE                          GCM_FALSE
#define CELL_GCM_TRUE                           GCM_TRUE
#define CELL_GCM_SRC_ALPHA                      GCM_SRC_ALPHA
#define CELL_GCM_ONE_MINUS_SRC_ALPHA            GCM_ONE_MINUS_SRC_ALPHA
#define CELL_GCM_FUNC_ADD                       GCM_FUNC_ADD
#define CELL_GCM_PRIMITIVE_TRIANGLES            GCM_TYPE_TRIANGLES
#define CELL_GCM_TEXTURE_LINEAR                 GCM_TEXTURE_LINEAR
#define CELL_GCM_TEXTURE_CLAMP_TO_EDGE          GCM_TEXTURE_CLAMP_TO_EDGE
#define CELL_GCM_TEXTURE_MAX_ANISO_1            0
#define CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX   0
#define CELL_GCM_TEXTURE_A8R8G8B8               (GCM_TEXTURE_FORMAT_A8R8G8B8 | GCM_TEXTURE_FORMAT_LIN | GCM_TEXTURE_FORMAT_UNRM)
#define CELL_GCM_TEXTURE_LN                     0
#define CELL_GCM_TEXTURE_DIMS_2D                2
#define CELL_GCM_VERTEX_F                       GCM_VERTEX_DATA_TYPE_F32
#define CELL_GCM_VERTEX_UB                      GCM_VERTEX_DATA_TYPE_U8
#define CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16       GCM_INDEX_TYPE_16B

#define cellGcmSetDepthTestEnable               rsxSetDepthTestEnable
#define cellGcmSetDepthMask                     rsxSetDepthMask
#define cellGcmSetCullFaceEnable                rsxSetCullFaceEnable
#define cellGcmSetBlendEnable                   rsxSetBlendEnable
#define cellGcmSetBlendFunc                     rsxSetBlendFunc
#define cellGcmSetBlendEquation                 rsxSetBlendEquation
#define cellGcmSetVertexProgramConstants        rsxSetVertexProgramConstants
#define cellGcmSetVertexDataArray               rsxSetVertexDataArray
#define cellGcmSetScissor                       rsxSetScissor
#define cellGcmSetTexture                       rsxSetTexture
#define cellGcmSetTextureControl                rsxSetTextureControl
#define cellGcmSetTextureFilter                 rsxSetTextureFilter
#define cellGcmSetTextureAddress                rsxSetTextureAddress
#define cellGcmSetDrawIndexArray                rsxSetDrawIndexArray
#else
#include <cell/gcm.h>
#endif

//-----------------------------------------------------------------------------
// RSX Shader Microcode (Self-contained, no external Cg compiler needed)
//-----------------------------------------------------------------------------
static const uint32_t g_VertexProgramUCode[] = {
    0x10001bf8, 0x00401801, 0x1bf80040, 0x18012061,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x10001bf8, 0x00401801, 0x1bf80040, 0x18012061,
    0x00000000, 0x00000000, 0x00000000, 0x00000000
};

static const uint32_t g_FragmentProgramUCode[] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x0140c3fc, 0x40028800, 0x00000002, 0x00000000
};

struct ImGui_ImplPS3RSX_Data
{
    gcmContextData* GcmContext;

    static const int BufferCount = 3;
    int              CurrentFrameIndex;

    uint32_t         VertexBufferSize;
    uint32_t         IndexBufferSize;
    void*            VertexBufferHost[BufferCount];
    void*            IndexBufferHost[BufferCount];
    uint32_t         VertexBufferOffset[BufferCount];
    uint32_t         IndexBufferOffset[BufferCount];

    void*            VertexProgram;
    void*            FragmentProgram;
    uint32_t         FragmentProgramOffset;

    uint32_t         FontTextureOffset;
    gcmTexture       FontTexture;

    ImGui_ImplPS3RSX_Data() { memset(this, 0, sizeof(*this)); }
};

static ImGui_ImplPS3RSX_Data* ImGui_ImplPS3RSX_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplPS3RSX_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

bool ImGui_ImplPS3RSX_Init(gcmContextData* context)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    ImGui_ImplPS3RSX_Data* bd = IM_NEW(ImGui_ImplPS3RSX_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_ps3rsx";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    bd->GcmContext = context;
    bd->CurrentFrameIndex = 0;
    bd->VertexBufferSize = 10000 * sizeof(ImDrawVert);
    bd->IndexBufferSize  = 20000 * sizeof(ImDrawIdx);

    for (int i = 0; i < ImGui_ImplPS3RSX_Data::BufferCount; i++)
    {
#if defined(__PSL1GHT__) || defined(PSL1GHT)
        bd->VertexBufferHost[i] = rsxMemalign(128, bd->VertexBufferSize);
        rsxAddressToOffset(bd->VertexBufferHost[i], &bd->VertexBufferOffset[i]);

        bd->IndexBufferHost[i] = rsxMemalign(128, bd->IndexBufferSize);
        rsxAddressToOffset(bd->IndexBufferHost[i], &bd->IndexBufferOffset[i]);
#else
        bd->VertexBufferHost[i] = memalign(128, bd->VertexBufferSize);
        cellGcmAddressToOffset(bd->VertexBufferHost[i], &bd->VertexBufferOffset[i]);

        bd->IndexBufferHost[i] = memalign(128, bd->IndexBufferSize);
        cellGcmAddressToOffset(bd->IndexBufferHost[i], &bd->IndexBufferOffset[i]);
#endif
    }

    return ImGui_ImplPS3RSX_CreateDeviceObjects();
}

void ImGui_ImplPS3RSX_Shutdown()
{
    ImGui_ImplPS3RSX_Data* bd = ImGui_ImplPS3RSX_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplPS3RSX_DestroyDeviceObjects();

    for (int i = 0; i < ImGui_ImplPS3RSX_Data::BufferCount; i++)
    {
        if (bd->VertexBufferHost[i]) { free(bd->VertexBufferHost[i]); bd->VertexBufferHost[i] = nullptr; }
        if (bd->IndexBufferHost[i])  { free(bd->IndexBufferHost[i]);  bd->IndexBufferHost[i] = nullptr; }
    }

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
    IM_DELETE(bd);
}

void ImGui_ImplPS3RSX_NewFrame()
{
    ImGui_ImplPS3RSX_Data* bd = ImGui_ImplPS3RSX_GetBackendData();
    IM_ASSERT(bd != nullptr && "Context or backend not initialized!");

    if (!bd->FontTextureOffset)
        ImGui_ImplPS3RSX_CreateFontsTexture();
}

bool ImGui_ImplPS3RSX_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplPS3RSX_Data* bd = ImGui_ImplPS3RSX_GetBackendData();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    uint32_t pitch = (width * 4 + 63) & ~63;
    uint32_t tex_size = pitch * height;

#if defined(__PSL1GHT__) || defined(PSL1GHT)
    void* vram_ptr = rsxMemalign(128, tex_size);
    rsxAddressToOffset(vram_ptr, &bd->FontTextureOffset);
#else
    void* vram_ptr = cellGcmUtilAllocateLocalMemory(tex_size, 128);
    cellGcmAddressToOffset(vram_ptr, &bd->FontTextureOffset);
#endif

    uint8_t* dst = (uint8_t*)vram_ptr;
    for (int y = 0; y < height; ++y)
    {
        uint32_t* row_dst = (uint32_t*)(dst + y * pitch);
        const uint8_t* row_src = pixels + y * width * 4;
        for (int x = 0; x < width; ++x)
        {
            uint8_t r = row_src[x * 4 + 0];
            uint8_t g = row_src[x * 4 + 1];
            uint8_t b = row_src[x * 4 + 2];
            uint8_t a = row_src[x * 4 + 3];
            row_dst[x] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        }
    }

    memset(&bd->FontTexture, 0, sizeof(bd->FontTexture));
    bd->FontTexture.format    = CELL_GCM_TEXTURE_A8R8G8B8;
    bd->FontTexture.mipmap    = 1;
    bd->FontTexture.dimension = CELL_GCM_TEXTURE_DIMS_2D;
    bd->FontTexture.cubemap   = CELL_GCM_FALSE;
    bd->FontTexture.remap     = 0xAAE4; // Standard ARGB remap
    bd->FontTexture.width     = width;
    bd->FontTexture.height    = height;
    bd->FontTexture.depth     = 1;
    bd->FontTexture.pitch     = pitch;
    bd->FontTexture.location  = CELL_GCM_LOCATION_LOCAL;
    bd->FontTexture.offset    = bd->FontTextureOffset;

    io.Fonts->SetTexID((ImTextureID)&bd->FontTexture);
    return true;
}

void ImGui_ImplPS3RSX_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplPS3RSX_Data* bd = ImGui_ImplPS3RSX_GetBackendData();
    if (bd && bd->FontTextureOffset)
    {
        io.Fonts->SetTexID(0);
        bd->FontTextureOffset = 0;
    }
}

bool ImGui_ImplPS3RSX_CreateDeviceObjects()
{
    ImGui_ImplPS3RSX_Data* bd = ImGui_ImplPS3RSX_GetBackendData();

    uint32_t fp_size = sizeof(g_FragmentProgramUCode);
#if defined(__PSL1GHT__) || defined(PSL1GHT)
    void* fp_vram = rsxMemalign(64, fp_size);
    memcpy(fp_vram, g_FragmentProgramUCode, fp_size);
    rsxAddressToOffset(fp_vram, &bd->FragmentProgramOffset);
#else
    void* fp_vram = cellGcmUtilAllocateLocalMemory(fp_size, 64);
    memcpy(fp_vram, g_FragmentProgramUCode, fp_size);
    cellGcmAddressToOffset(fp_vram, &bd->FragmentProgramOffset);
#endif

    bd->VertexProgram = (void*)g_VertexProgramUCode;
    bd->FragmentProgram = fp_vram;

    return true;
}

void ImGui_ImplPS3RSX_DestroyDeviceObjects()
{
    ImGui_ImplPS3RSX_DestroyFontsTexture();
}

void ImGui_ImplPS3RSX_RenderDrawData(ImDrawData* draw_data)
{
    int fb_width  = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0 || draw_data->TotalVtxCount <= 0)
        return;

    ImGui_ImplPS3RSX_Data* bd = ImGui_ImplPS3RSX_GetBackendData();
    gcmContextData* ctx = bd->GcmContext;

    bd->CurrentFrameIndex = (bd->CurrentFrameIndex + 1) % ImGui_ImplPS3RSX_Data::BufferCount;
    int buf_idx = bd->CurrentFrameIndex;

    ImDrawVert* vtx_dst = (ImDrawVert*)bd->VertexBufferHost[buf_idx];
    ImDrawIdx*  idx_dst = (ImDrawIdx*)bd->IndexBufferHost[buf_idx];

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }

    cellGcmSetDepthTestEnable(ctx, CELL_GCM_FALSE);
    cellGcmSetDepthMask(ctx, CELL_GCM_FALSE);
    cellGcmSetCullFaceEnable(ctx, CELL_GCM_FALSE);
    cellGcmSetBlendEnable(ctx, CELL_GCM_TRUE);
    cellGcmSetBlendFunc(ctx, CELL_GCM_SRC_ALPHA, CELL_GCM_ONE_MINUS_SRC_ALPHA,
                             CELL_GCM_SRC_ALPHA, CELL_GCM_ONE_MINUS_SRC_ALPHA);
    cellGcmSetBlendEquation(ctx, CELL_GCM_FUNC_ADD, CELL_GCM_FUNC_ADD);

    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    float ortho_proj[4][4] = {
        { 2.0f / (R - L),    0.0f,            0.0f,  0.0f },
        { 0.0f,             -2.0f / (B - T),  0.0f,  0.0f },
        { 0.0f,              0.0f,           -1.0f,  0.0f },
        { (L + R) / (L - R), (T + B) / (B - T), 0.0f, 1.0f }
    };
    cellGcmSetVertexProgramConstants(ctx, 0, 4, (float*)ortho_proj);

    uint32_t vtx_base_offset = bd->VertexBufferOffset[buf_idx];
    uint32_t idx_base_offset = bd->IndexBufferOffset[buf_idx];

    cellGcmSetVertexDataArray(ctx, 0, 0, sizeof(ImDrawVert), 2, CELL_GCM_VERTEX_F,
                             CELL_GCM_LOCATION_MAIN, vtx_base_offset + offsetof(ImDrawVert, pos));
    cellGcmSetVertexDataArray(ctx, 1, 0, sizeof(ImDrawVert), 2, CELL_GCM_VERTEX_F,
                             CELL_GCM_LOCATION_MAIN, vtx_base_offset + offsetof(ImDrawVert, uv));
    cellGcmSetVertexDataArray(ctx, 2, 0, sizeof(ImDrawVert), 4, CELL_GCM_VERTEX_UB,
                             CELL_GCM_LOCATION_MAIN, vtx_base_offset + offsetof(ImDrawVert, col));

    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
                ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
                if (clip_min.x < 0.0f) clip_min.x = 0.0f;
                if (clip_min.y < 0.0f) clip_min.y = 0.0f;
                if (clip_max.x > (float)fb_width)  clip_max.x = (float)fb_width;
                if (clip_max.y > (float)fb_height) clip_max.y = (float)fb_height;
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                cellGcmSetScissor(ctx, (uint16_t)clip_min.x, (uint16_t)clip_min.y,
                                       (uint16_t)(clip_max.x - clip_min.x),
                                       (uint16_t)(clip_max.y - clip_min.y));

                gcmTexture* tex = (gcmTexture*)pcmd->GetTexID();
                if (tex != nullptr)
                {
                    cellGcmSetTexture(ctx, 0, tex);
                    cellGcmSetTextureControl(ctx, 0, CELL_GCM_TRUE, 0, 0, CELL_GCM_TEXTURE_MAX_ANISO_1);
                    cellGcmSetTextureFilter(ctx, 0, 0, CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_LINEAR, CELL_GCM_TEXTURE_CONVOLUTION_QUINCUNX);
                    cellGcmSetTextureAddress(ctx, 0, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, CELL_GCM_TEXTURE_CLAMP_TO_EDGE, 0, 0, 0);
                }

                uint32_t draw_idx_offset = idx_base_offset + (pcmd->IdxOffset + global_idx_offset) * sizeof(ImDrawIdx);
                cellGcmSetDrawIndexArray(ctx, CELL_GCM_PRIMITIVE_TRIANGLES, pcmd->ElemCount,
                                         CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16,
                                         CELL_GCM_LOCATION_MAIN, draw_idx_offset);
            }
        }
        global_vtx_offset += cmd_list->VtxBuffer.Size;
        global_idx_offset += cmd_list->IdxBuffer.Size;
    }
}

void ImGui_ImplPS3RSX_SaveRenderState(ImGui_ImplPS3RSX_RenderState* state)
{
    memset(state, 0, sizeof(*state));
}

void ImGui_ImplPS3RSX_RestoreRenderState(const ImGui_ImplPS3RSX_RenderState* state)
{
}

#endif // #ifndef IMGUI_DISABLE
