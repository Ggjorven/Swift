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
		IncludeDir = "%{wks.location}/vendor/assimp/include",

		Windows = 
		{
			LibName = "assimp-vc143-mt",
			DebugLibName = "assimp-vc143-mtd",
			LibDir = "%{wks.location}/vendor/assimp/bin/windows/",
			DynamicLib = "%{Dependencies.Assimp.Windows.LibDir}" .. "assimp-vc143-mt.dll",
			DebugDynamicLib = "%{Dependencies.Assimp.Windows.LibDir}" .. "assimp-vc143-mtd.dll",
		},
		Linux =  
		{
			LibName = "libassimp.so",				-- TODO: Check this out
			DebugDynamicLibName = "libassimp.so.5",	-- TODO: Check this out
			LibDir = "%{wks.location}/vendor/assimp/bin/linux",
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

workspace "SwiftRenderer"
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

group "SwiftRenderer"
	include "Core"
group ""

include "Sandbox"
------------------------------------------------------------------------------