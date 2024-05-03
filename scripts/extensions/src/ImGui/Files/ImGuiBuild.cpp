#include "swpch.h"

#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"

#define IMGUI_IMPL_VULKAN
#include <backends/imgui_impl_vulkan.cpp>

// Note(Jorben): GLFW is our windowing library
#include <backends/imgui_impl_glfw.cpp>