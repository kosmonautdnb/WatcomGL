gpp -c -o5 -ofast -o_glimpl.obj glimpl.cpp
gpp -c -o5 -ofast -o_nehe.obj _nehe.cpp
gpp -c -o5 -ofast -o_glfwbng.obj _glfwbng.cpp
gpp -c -o5 -ofast -Wno-narrowing -o_yellow.obj _yellow.cpp
gpp -o _nehe.exe _glimpl.obj _nehe.obj
gpp -o _glfwbng.exe _glimpl.obj _glfwbng.obj
gpp -o _yellow.exe _glimpl.obj _yellow.obj
@echo off
del _glimpl.obj
del _nehe.obj
del _glfwbng.obj
del _yellow.obj
_glfwbng.exe
