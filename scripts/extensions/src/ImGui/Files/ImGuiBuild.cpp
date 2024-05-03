#include "swpch.h"

#include "imconfig.h"
#include "imgui.h"
#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_internal.h"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"
#include "imstb_rectpack.h"
#include "imstb_textedit.h"
#include "imstb_truetype.h"
#include "imgui_demo.cpp"

#define IMGUI_IMPL_VULKAN
#include <backends/imgui_impl_vulkan.cpp>

// Note(Jorben): GLFW is our windowing library
#include <backends/imgui_impl_glfw.cpp>