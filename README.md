# Software OpenGL for WatcomC, DJGPP on DOS and any C/C++

#### Just include and link:  
  GL.H  
  GLIMPL.CPP  
#### to your WatcomC/DJGPP/C project.  

WatcomGL demo game: https://github.com/kosmonautdnb/WatcomGL_Demo_Game   

WatcomC library collection: https://github.com/kosmonautdnb/WatcomLibs  

You can use this library as a general C software rasterizer, it is not limited to DOS.

Yellow Rose of Texas is a 4k demo by FIT/BANDWAGON (_YELLOW.CPP)  

Needs a fairly fast FreeDOS/DOS machine. It looks not so well optimized but doesn't get faster by disabling all non fancy stuff. Main limiting factor by far seems to be memory bandwidth(+cache). File names are upper case which is DOS default.  

(Better use the public domain PModeW instead of Dos4GW for 256 MB of memory instead of the normal 32 MB in WatcomC++)  
## Typical usage
#include "GL.H"  

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

## DJGPP OSMesa on DOS
Comparing DJGPP OSMesa to DJGPP WatcomGL:  
https://youtu.be/eBOSpH6WCXY?si=uEAl7VEM6rkouTIv 
  
In this video featuring Wipeout it's OSMesa with around 40 FPS and WatcomGL with around 100 FPS.  
  
OSMesa library compile:
https://bisqwit.iki.fi/jutut/kuvat/programming_examples/djgpp_mesa.zip zip not by me.  

## TinyGL
There is a also the TinyGL project.  
https://github.com/erysdren/TinyGL.git  

# Features
- Header Compatible With OpenGL
- More Performant Than The Early OSMesa (The Original OpenGL)
- MIT License For Ease Of Use
- Vesa2.0 Calls For 32Bit Graphics Modes
- Optional Fallback To VGA Mode With 320x200 And 8192 Simulated Colors
- Z-Buffer: 32 Bit Float  
- FrameBuffer: 32 Bit RGBA (8 Bit Per Channel)  
- GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_QUADS  
- glVertex3f And Alikes (Also glVertex3d And Alikes)  
- Bilinear Texture Filtering And Nearest Point Texture Filtering  
- Pixel And Texel Center
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

# Drawbacks
- No MultiThreading
- No MultiTexturing
- No Assembly/MMX/SSE etc..
- No DisplayLists/VertexBuffers/IndexBuffers
- No Shaders
- Just the very oldschool OpenGL

This repository lives at: https://github.com/kosmonautdnb/WatcomGL.git  
