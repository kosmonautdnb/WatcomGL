gcc -c -O3 -o_glimpl.obj glimpl.cpp
gcc -c -O3 -o_nehe.obj _nehe.cpp
gcc -c -O3 -o_glfwbng.obj _glfwbng.cpp
gcc -c -O3 -Wno-narrowing -o_yellow.obj _yellow.cpp
gcc -c -O3 -D__GLDISABLEDOSFUNCTIONS__=1 -o_glimpld.obj glimpl.cpp
gcc -c -O3 -o_direct.obj _direct.cpp
gcc -o _nehe.exe _glimpl.obj _nehe.obj
gcc -o _glfwbng.exe _glimpl.obj _glfwbng.obj
gcc -o _yellow.exe _glimpl.obj _yellow.obj
gcc -o _direct.exe _glimpld.obj _direct.obj
@echo off
del _glimpl.obj
del _nehe.obj
del _glfwbng.obj
del _yellow.obj
del _glimpld.obj
del _direct.obj
_glfwbng.exe
