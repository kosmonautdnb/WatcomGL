@echo off
wpp386 /mf glimpl.cpp  -fo_glimpl.obj
wpp386 /mf _nehe.cpp -fo_nehe.obj
wpp386 /mf _glfwbng.cpp -fo_glfwbng.obj
wpp386 /mf _yellow.cpp -fo_yellow.obj
wlink FILE _nehe.obj FILE _glimpl.obj  OPTION QUIET OPTION STack = 65536 NAME _nehe.exe
wlink FILE _glfwbng.obj FILE _glimpl.obj  OPTION QUIET OPTION STack = 65536 NAME _glfwbng.exe
wlink FILE _yellow.obj FILE _glimpl.obj  OPTION QUIET OPTION STack = 65536 NAME _yellow.exe
del _glimpl.obj
del _nehe.obj
del _glfwbng.obj
del _yellow.obj
_glfwbng.exe
