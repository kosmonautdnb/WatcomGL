# Software OpenGL for DOS/WatcomC++

Just include and link:  
  GL.H  
  GLIMPL.CPP  
in your WatcomC++ project.  

For some fun: link _YELLOW.CPP to see "Yellow Rose of Texas" a 4k by Fit and Bandwagon.  

WatcomGL demo game: https://github.com/kosmonautdnb/WatcomGL_Demo_Game   

WatcomC library collection: https://github.com/kosmonautdnb/WatcomLibs  

Needs a fairly fast FreeDOS/DOS machine.
## Typical usage
#include "gl.h"  

void main() {  
&emsp;if (!glVesa(640,480,32))  // try to use Vesa 640x480x32 + Linear Frame Buffer  
&emsp;&emsp;glVGA(); // use 320x200 with pseudo 8192 Colors  

&emsp;while(true) {  
&emsp;&emsp;int key = glNextKey(); if (key == GL_VK_ESCAPE) break;  
&emsp;&emsp;double mouseX, mouseY; glNextMouseDelta(&mouseX,&mouseY); int mouseButtons = glMouseButtons();  
&emsp;&emsp;double time = glSeconds(); // rather bad granularity of 18.1 hz  
&emsp;&emsp;glClearColor(0.25,0.5,1.0,0.0);  
&emsp;&emsp;glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);  
&emsp;&emsp; .. Render something ..  
&emsp;&emsp;glRefresh(); // important  
&emsp;}  
&emsp;glDone();  
}

## Mesa
If you use DJGPP instead of WatcomC++ on DOS maybe this is an additional software OpenGL option:  
https://github.com/EMGD-Community/mesa/blob/master/docs/README.DJ  
Current Mesa Version is: 25.1.6. DJGPP was removed again from support as of Mesa6.5.3, in 2007, though.  
You can find the compiled Mesa libraries in the comments of this video (video not by me):  
https://www.youtube.com/watch?v=vkUwT9U1GzA
