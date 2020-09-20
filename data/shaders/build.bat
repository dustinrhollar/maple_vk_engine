@echo off

for %%i in (*.vert *.frag *.geom) do "C:\VulkanSDK\1.2.148.1\Bin\glslc.exe"  -flimit-file shaders.conf "%%~i" -o "%%~i.spv"