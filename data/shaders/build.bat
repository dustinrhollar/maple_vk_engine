@echo off

for %%i in (*.vert *.frag *.geom) do "C:\VulkanSDK\1.2.135.0\Bin\glslc.exe"  -flimit-file shaders.conf "%%~i" -o "%%~i.spv"