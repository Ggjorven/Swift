------------------------------------------------------------------------------
-- Utilities
------------------------------------------------------------------------------
function FirstToUpper(str)
	return (str:gsub("^%l", string.upper))
end
------------------------------------------------------------------------------

------------------------------------------------------------------------------
-- Dependencies
------------------------------------------------------------------------------
VULKAN_SDK = os.getenv("VULKAN_SDK")

Dependencies = 
{
	-- Vulkan Related
	Vulkan = 
	{
		Windows = 
		{
			LibName = "vulkan-1",
			IncludeDir = "%{VULKAN_SDK}/Include/",
			LibDir = "%{VULKAN_SDK}/Lib/"
		},
		Linux =  
		{
			LibName = "vulkan",
			IncludeDir = "%{VULKAN_SDK}/include/",
			LibDir = "%{VULKAN_SDK}/lib/"
		}
	},
	ShaderC = 
	{
		LibName = "shaderc_shared",
		DebugLibName = "shaderc_sharedd"
	},
	VMA = 
	{
		LibName = "VMA",
		IncludeDir = "%{wks.location}/vendor/vma/include"
	},

	-- Libs
	GLFW = 
	{
		LibName = "GLFW",
		IncludeDir = "%{wks.location}/vendor/GLFW/include"
	},
	Tracy = 
	{
		LibName = "Tracy",
		IncludeDir = "%{wks.location}/vendor/tracy/tracy/public"
	},
	Assimp = 
	{
		Windows = 
		{
			LibName = "assimp-vc143-mt",
			DebugLibName = "assimp-vc143-mtd",
			IncludeDir = "%{wks.location}/vendor/assimp/include",
			LibDir = "%{wks.location}/vendor/assimp/bin/windows/",
			DynamicLib = "%{Dependencies.Assimp.Windows.LibDir}" .. "assimp-vc143-mt.dll",
			DebugDynamicLib = "%{Dependencies.Assimp.Windows.LibDir}" .. "assimp-vc143-mtd.dll",
		},
		Linux =  
		{
			LibName = "assimp-vc143-mt", -- TODO
			DebugDynamicLibName = "assimp-vc143-mtd.dll", -- TODO
			IncludeDir = "%{wks.location}/vendor/assimp/include",
			LibDir = "%{wks.location}/vendor/assimp/bin/", -- TODO
			DebugLib = "%{Dependencies.Assimp.Linux.LibDir}" .. "assimp-vc143-mtd", -- TODO
			DynamicLib = "%{Dependencies.Assimp.Linux.LibDir}" .. "assimp-vc143-mt.dll", -- TODO
		}
	},

	-- Includes
	GLM = 
	{
		IncludeDir = "%{wks.location}/vendor/glm"
	},
	Spdlog = 
	{
		IncludeDir = "%{wks.location}/vendor/spdlog/include"
	},
	Stb_image =
	{
		IncludeDir = "%{wks.location}/vendor/stb/include"
	}
}
------------------------------------------------------------------------------

------------------------------------------------------------------------------
-- Solution
------------------------------------------------------------------------------
outputdir = "%{cfg.buildcfg}-" .. FirstToUpper("%{cfg.system}")

workspace "Outline"
	architecture "x86_64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

group "Dependencies"
	include "vendor/glfw"
	include "vendor/tracy"
	include "vendor/vma"
group ""

group "Outline"
	include "Outline"
group ""

include "Sandbox"
------------------------------------------------------------------------------