w# Software OpenGL for DOS/WatcomC++ and DJGPP

#### Just include and link:  
  GL.H  
  GLIMPL.CPP  
#### to your WatcomC++/DJGPP project.  

For some fun: link _YELLOW.CPP to see "Yellow Rose of Texas" a 4k by Fit and Bandwagon.  

WatcomGL demo game: https://github.com/kosmonautdnb/WatcomGL_Demo_Game   

WatcomC library collection: https://github.com/kosmonautdnb/WatcomLibs  

Needs a fairly fast FreeDOS/DOS machine. It looks not so well optimized but doesn't get faster by disabling all non fancy stuff. Main limiting factor by far seems to be memory bandwidth(+cache). File names are upper case which is DOS default.  

(Better use the public domain PModeW instead of Dos4GW for 256 MB of memory instead of 32 MB)  
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
or directly at:
https://bisqwit.iki.fi/jutut/kuvat/programming_examples/djgpp_mesa.zip

## TinyGL
There is a also the TinyGL project. Which seems to do the same. I don't know anything about it maybe it's rendering is a lot faster (but they implemented a lot less), however here is the link:  
https://github.com/erysdren/TinyGL.git  

# Features
- Header Compatible With OpenGL
- MIT License For Ease Of Use
- Vesa2.0 Calls For 32Bit Graphics Modes
- Optional Fallback To VGA Mode With 320x200 And 8192 Simulated Colors
- Z-Buffer: 32 Bit Float  
- FrameBuffer: 32 Bit RGBA (8 Bit Per Channel)  
- GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_QUADS  
- glVertex3f And Alikes (Also glVertex3d And Alikes)  
- Bilinear Texture Filtering And Nearest Point Texture Filtering  
- Perspective Correct Gouraud And Texture Mapping  
- All OpenGL Blending / Transparency Modes  
- Alpha Test  
- Color Channel Masking  
- OpenGL Material And Light / Coloring Pipeline  
- 2 Lights For Additional Lighting  
- Option For Separate Specular Addition  
- Z Clipping On Nearplane  
- All OpenGL Texture Repeat Modes  
- Additional GLU Functions For Setting Up Matrices
- All Fogging Modes
- Vertex Based Spherical Environment Mapping
- glPushMatrix/glPopMatrix
- glPolygonMode For Wireframe Rendering Of Polygons
- Possible Two Sided Lighting
- Complete Fixed Function OpenGL Pipeline

This repository lives at: https://github.com/kosmonautdnb/WatcomGL.git  
