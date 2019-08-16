workspace "g3GXT"
	configurations { "Release", "Debug" }
	location "project"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	targetdir "output/%{cfg.buildcfg}"
	characterset "MBCS"
	defines { "WIN32_LEAN_AND_MEAN", "_CRT_SECURE_NO_WARNINGS" }
	
	filter "*Release"
		optimize "On"
		symbols "Off"
	
	filter "*Debug"
		symbols "On"
	
project "g3GXT_vc"
	targetname "g3GXT_vc"
	files "g3GXT_vc.cpp"

project "g3GXT_iii"
	targetname "g3GXT_iii"
	files "g3GXT_iii.cpp"

project "gxttotxt"
	links "shlwapi"
	targetname "gxttotxt"
	files "gxttotxt.cpp"
	
	