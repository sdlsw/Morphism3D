set "glslc=C:\VulkanSDK\1.4.321.1\Bin\glslc.exe"

%glslc% shader\unlit.vert -o unlit_vert.spv
%glslc% shader\unlit.frag -o unlit_frag.spv
mv unlit_vert.spv build\
mv unlit_frag.spv build\

%glslc% shader\lit.vert -o lit_vert.spv
%glslc% shader\lit.frag -o lit_frag.spv
mv lit_vert.spv build\
mv lit_frag.spv build\

pause
