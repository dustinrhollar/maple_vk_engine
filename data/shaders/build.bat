@echo off

:: for %%i in (*.vert *.frag) do "C:\VulkanSDK\1.2.135.0\Bin\glslc.exe"  -flimit-file shaders.conf "%%~i" -o "%%~i.spv"

fxc /nologo /Od /Zi /T vs_5_0 /Fosimple_vert.cso simple.hlsl
fxc /nologo /Od /Zi /T ps_5_0 /Fosimple_frag.cso simple_frag.hlsl

:: Terrain shaders
fxc /nologo /Od /Zi /T vs_5_0 /Foterrain_vertex.cso terrain.hlsl
fxc /nologo /Od /Zi /T ps_5_0 /Foterrain_pixel.cso terrain_pixel.hlsl

