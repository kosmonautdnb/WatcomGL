wcl386.exe /c /mf -fospk.obj app_spkt.cpp
wcl386.exe /c /mf -fokeys.obj app_keys.cpp
wcl386.exe /c /mf -fovid.obj app_vid.cpp
wcl386.exe /c /mf -fogl.obj glimpl.cpp
wcl386.exe /c /mf -foi1.obj imgui.cpp
wcl386.exe /c /mf -foi2.obj imguigl.cpp
wcl386.exe /c /mf -foo.obj object.cpp
wcl386.exe /xr /c /mf -fos.obj string.cpp
wcl386.exe /c /mf -fosp.obj speaker.cpp
wcl386.exe /c /mf -fod.obj dos.cpp
wcl386.exe /c /mf -fof.obj filedlg.cpp
wcl386.exe /c /mf -foe.obj glega.cpp
wcl.exe APP_SPPL.C
wlink @appkeys.lnk OPTION QUIET OPTION STack = 65536 NAME app_keys.exe
wlink @appvid.lnk OPTION QUIET OPTION STack = 65536 NAME app_vid.exe
wlink @appspkt.lnk OPTION QUIET OPTION STack = 65536 NAME app_spkt.exe
@echo off
del keys.obj
del spk.obj
del vid.obj
del gl.obj
del i1.obj
del i2.obj
del o.obj
del s.obj
del sp.obj
del d.obj
del f.obj
del e.obj
del app_sppl.obj
