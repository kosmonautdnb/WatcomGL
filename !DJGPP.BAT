gcc -c -O3 -o_glimpl.obj glimpl.cpp
gcc -c -O3 -o_nehe.obj _nehe.cpp
gcc -c -O3 -o_glfwbng.obj _glfwbng.cpp
gcc -c -O3 -Wno-narrowing -o_yellow.obj _yellow.cpp
gcc -o _nehe.exe _glimpl.obj _nehe.obj
gcc -o _glfwbng.exe _glimpl.obj _glfwbng.obj
gcc -o _yellow.exe _glimpl.obj _yellow.obj
@echo off
del _glimpl.obj
del _nehe.obj
del _glfwbng.obj
del _yellow.obj
_glfwbng.exe
