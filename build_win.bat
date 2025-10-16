@echo off

setlocal

set VULKAN_SDK=C:\VulkanSDK\1.4.321.1

:: Compiler flags
set CFLAGS=-I"%VULKAN_SDK%\Include" -DDEBUG -g -Wall

:: Linker flags
set LDFLAGS=-L"%VULKAN_SDK%\Lib" ^
            -lvulkan-1 ^
            -luser32 ^
            -lkernel32

:: compile shaders
glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv

clang++ main.cpp glass_windows.cpp glass_render_vulkan.cpp geometry.cpp -o main.exe -std=c++17 %CFLAGS% %LDFLAGS% -D_CRT_SECURE_NO_WARNINGS -DVulkan -DVK_USE_PLATFORM_WIN32_KHR