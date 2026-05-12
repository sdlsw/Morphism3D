C:\VulkanSDK\1.4.321.1\Bin\glslc.exe shader_unlit.vert -o unlit_vert.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe shader_unlit.frag -o unlit_frag.spv
mv unlit_vert.spv build\Debug\
mv unlit_frag.spv build\Debug\

C:\VulkanSDK\1.4.321.1\Bin\glslc.exe shader_lit.vert -o lit_vert.spv
C:\VulkanSDK\1.4.321.1\Bin\glslc.exe shader_lit.frag -o lit_frag.spv
mv lit_vert.spv build\Debug\
mv lit_frag.spv build\Debug\

pause
