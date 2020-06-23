@echo off

for %%i in (*.vert *.frag) do "C:\VulkanSDK\1.2.135.0\Bin\glslc.exe"  -flimit-file shaders.conf "%%~i" -o "%%~i.spv"