## Simple OpenGL "Emulator"

Just link GLIMPL.CPP and include GL.H to your WatcomC project. 

May also work in other (non Dos) environments.

Current hot topics: barycentric coordinates cause polygon edges to be calculated incorrectly (in very very seldom cases).

There was a bug, that Textures where flipped on Y. Now they are flipped at glTexImage2d already. In older versions of this library the texture coordinates needed to be flipped.

For some fun: link _YELLOW.CPP to see "Yellow Rose of Texas" a 4k by Fit and Bandwagon.

Btw. seems like NVidia and maybe AMD are keeping the support for Vesa (under Dos).

This OpenGL "Emulation" is purely in software "slow" and not optimized yet. 

A demo game can be found here: https://github.com/kosmonautdnb/OpenGLDemoGame_DOS

This is part of my "template lib" here: https://github.com/kosmonautdnb/TemplateLib_WatcomC.git
