gpp -c -O3 -ospk.obj app_spkt.cpp
gpp -c -O3 -okeys.obj app_keys.cpp
gpp -c -O3 -ovid.obj app_vid.cpp
gpp -c -O3 -ogl.obj glimpl.cpp
gpp -c -O3 -oi1.obj imgui.cpp
gpp -c -O3 -oi2.obj imguigl.cpp
gpp -c -O3 -oo.obj object.cpp
gpp -c -O3 -os.obj string.cpp
gpp -c -O3 -osp.obj speaker.cpp
gpp -c -O3 -od.obj dos.cpp
gpp -c -O3 -of.obj filedlg.cpp
gpp -c -O3 -oe.obj glega.cpp
gpp -o APP_KEYS.EXE gl.obj keys.obj
gpp -o APP_VID.EXE gl.obj e.obj vid.obj
gpp -o APP_SPKT.EXE gl.obj i1.obj i2.obj o.obj s.obj sp.obj d.obj f.obj e.obj spk.obj
gpp.exe -o APP_SPPL.EXE app_sppl.c
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
