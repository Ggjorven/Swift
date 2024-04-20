project "Swift"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "On"

	architecture "x86_64"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "swpch.h"
	pchsource "src/Swift/swpch.cpp"

	files
	{
		"src/Swift/*.h",
		"src/Swift/*.hpp",
		"src/Swift/*.cpp",

		"src/Swift/Core/**.h",
		"src/Swift/Core/**.hpp",
		"src/Swift/Core/**.cpp",
		
		"src/Swift/Renderer/**.h",
		"src/Swift/Renderer/**.hpp",
		"src/Swift/Renderer/**.cpp",

		"src/Swift/Utils/**.h",
		"src/Swift/Utils/**.hpp",
		"src/Swift/Utils/**.cpp",

		"src/Swift/Vulkan/**.h",
		"src/Swift/Vulkan/**.hpp",
		"src/Swift/Vulkan/**.cpp",

		"src/Swift/Platforms/" .. FirstToUpper("%{cfg.system}") .. "/**.h",
		"src/Swift/Platforms/" .. FirstToUpper("%{cfg.system}") .. "/**.hpp",
		"src/Swift/Platforms/" .. FirstToUpper("%{cfg.system}") .. "/**.cpp",

		"%{wks.location}/vendor/stb/src/stb_image.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	includedirs
	{
		"src",
		"src/Swift",

		"%{Dependencies.GLFW.IncludeDir}",
		"%{Dependencies.GLM.IncludeDir}",
		"%{Dependencies.Spdlog.IncludeDir}",
		"%{Dependencies.Stb_image.IncludeDir}",
		"%{Dependencies.Assimp.IncludeDir}",
		"%{Dependencies.Tracy.IncludeDir}",
		"%{Dependencies.VMA.IncludeDir}"
	}

	links
	{
		"%{Dependencies.GLFW.LibName}",
		"%{Dependencies.Tracy.LibName}",
		"%{Dependencies.VMA.LibName}"
	}

	disablewarnings
	{
		"4005",
		"4996"
	}

	filter "system:windows"
		systemversion "latest"
		staticruntime "on"

		defines
		{
			"APP_PLATFORM_WINDOWS"
		}

		includedirs
		{
			"%{Dependencies.Vulkan.Windows.IncludeDir}"
		}

		links
		{
			"%{Dependencies.Vulkan.Windows.LibDir}" .. "%{Dependencies.Vulkan.Windows.LibName}"
		}

	-- TODO: Implement Linux

	filter "configurations:Debug"
		defines "APP_DEBUG"
		runtime "Debug"
		symbols "on"
		editandcontinue "Off"

		defines
		{
			"TRACY_ENABLE",
			"NOMINMAX"
		}

	filter "configurations:Release"
		defines "APP_RELEASE"
		runtime "Release"
		optimize "on"

		defines 
		{
			"TRACY_ENABLE",
			"NOMINMAX"
		}

	filter "configurations:Dist"
		defines "APP_DIST"
		runtime "Release"
		optimize "Full"

	filter { "system:windows", "configurations:Debug" }
		links
		{
			"%{Dependencies.Vulkan.Windows.LibDir}" .. "%{Dependencies.ShaderC.DebugLibName}",
			"%{Dependencies.Assimp.Windows.LibDir}" .. "%{Dependencies.Assimp.Windows.DebugLibName}"
		}

	filter { "system:windows", "configurations:Release or configurations:Dist" }
		links
		{
			"%{Dependencies.Vulkan.Windows.LibDir}" .. "%{Dependencies.ShaderC.LibName}",
			"%{Dependencies.Assimp.Windows.LibDir}" .. "%{Dependencies.Assimp.Windows.LibName}"

		}