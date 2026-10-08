# Dear ImGui — PlayStation 3 (RSX / DualShock 3)

[![Build PS3 Dear ImGui PKG](https://github.com/DeMaxi1337/dear-imgui-ps3/actions/workflows/build.yml/badge.svg)](https://github.com/DeMaxi1337/dear-imgui-ps3/actions/workflows/build.yml)

Full adaptation of **Dear ImGui (1.9x)** for the **Sony PlayStation 3** architecture (Cell Broadband Engine + RSX Reality Synthesizer + DualShock 3).

Designed for **homebrew applications**, **debugging tools**, and **in-game overlays / trainers (SPRX)** on both **CFW (Evilnat, Rebug)** and **HFW (PS3HEN)**.

---

## 🎮 DualShock 3 Controls

| Button | ImGui Action | Description |
|---|---|---|
| **L3 + R3** (Both stick clicks) | **Toggle Menu** | Open / close overlay in-game |
| **Right Analog Stick** | **Virtual Mouse Cursor** | Smooth pointer with radial deadzone and acceleration curve |
| **Cross ($\times$)** | **Left Click (LMB)** | Activate buttons, drag windows, interact with widgets |
| **Circle ($\bigcirc$)** | **Right Click (RMB)** | Cancel, close dropdowns |
| **L1 / R1** | **Vertical Scroll** | Fast scrolling up/down |
| **D-Pad** | **Native Navigation** | `ImGuiConfigFlags_NavEnableGamepad` navigation |

---

## ⚡ PlayStation 3 Architecture Features

* **Big-Endian Color Packing (`imconfig.h`)**: Corrects PowerPC 64-bit Big-Endian color layout (`IM_COL32_R_SHIFT 24`, `G_SHIFT 16`, `B_SHIFT 8`, `A_SHIFT 0`) for hardware `[R, G, B, A]` memory representation. Zero CPU overhead, no blue/red color swap.
* **16-Bit Indices (`ImDrawIdx`)**: Configured as `unsigned short`, natively matching RSX hardware `CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16`.
* **Triple-Buffered Dynamic Geometry**: Vertices and indices are streamed through mapped Host Memory (`CELL_GCM_LOCATION_MAIN`), preventing GPU/CPU stalls.
* **Hardware Scissor Clipping**: Top-Left RSX coordinate system matches ImGui screen space directly without Y-inversion.
* **In-Game Input Filtering**: `ImGui_ImplPS3Pad_FilterGameInput` prevents game character actions while navigating the menu.
* **Render State Preservation**: `SaveRenderState` / `RestoreRenderState` allows safe in-game hooking without corrupting host game graphics.

---

## 🚀 Quick Start in C++

```cpp
#include "imgui.h"
#include "backends/imgui_impl_ps3rsx.h"
#include "backends/imgui_impl_ps3pad.h"

// 1. Initialization
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGui::GetIO().DisplaySize = ImVec2(1280.0f, 720.0f);

ImGui_ImplPS3Pad_Init(1);
ImGui_ImplPS3RSX_Init(gcmContext);

// 2. Per-Frame Loop
ImGui_ImplPS3Pad_NewFrame();
ImGui_ImplPS3RSX_NewFrame();
ImGui::NewFrame();

// Render your UI
ImGui::Begin("PS3 Developer Menu");
ImGui::Text("Hello PS3!");
static bool godMode = false;
ImGui::Checkbox("God Mode", &godMode);
ImGui::End();

// Draw cursor & geometry
ImGui_ImplPS3Pad_RenderMouseCursor();
ImGui::Render();
ImGui_ImplPS3RSX_RenderDrawData(ImGui::GetDrawData());

// 3. Shutdown
ImGui_ImplPS3RSX_Shutdown();
ImGui_ImplPS3Pad_Shutdown();
ImGui::DestroyContext();
```

---

## 📦 Automated Builds

Automated compilation and packaging into `.pkg` and `.sprx` are handled via [GitHub Actions](.github/workflows/build.yml). Every push to `main` produces downloadable artifacts.
