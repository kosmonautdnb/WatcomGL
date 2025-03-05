wcl386.exe /ms /fe=_build\genmak.exe _build\genmak.c
del genmak.obj
wcl386.exe /ms /fe=_build\fltmechk.exe _build\fltmechk.c
del fltmechk.obj