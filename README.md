# Software OpenGL for DOS/WatcomC++

Just include and link:  
  GL.H  
  GLIMPL.CPP  
in your WatcomC++ project.  

For some fun: link _YELLOW.CPP to see "Yellow Rose of Texas" a 4k by Fit and Bandwagon.  

A demo game can be found here: https://github.com/kosmonautdnb/OpenGLDemoGame_DOS  

This is part of my "template lib" here: https://github.com/kosmonautdnb/TemplateLib_WatcomC.git  

If you use DJGPP instead of WatcomC++ on DOS maybe this is an additional software OpenGL option:  
https://github.com/EMGD-Community/mesa/blob/master/docs/README.DJ  
Current Mesa Version is: 25.1.6. DJGPP was removed again from support as of Mesa6.5.3, in 2007, though.

## Typical usage:

if (!glVesa(640,480,32))  // try to use Vesa 640x480x32 + Linear Frame Buffer  
  glVGA(); // use 320x200 with emulated 8192 Colors  

while(true) {  
  int key = glNextKey(); if (key == GL_VK_ESCAPE) break;  
  double mouseX, mouseY; glNextMouseDelta(&mouseX,&mouseY); int mouseButtons = glMouseButtons();  
  double time = glSeconds(); // rather bad granularity of 18.1 hz  
  glClearColor(0.25,0.5,1.0,0.0);  
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);  
  glRefresh(); // important  
}  
glDone();  
