![ScreenShot](https://raw.github.com/kosmonautdnb/OpenGLDemoGame_DOS/main/watcomgl.png)
by Stefan Mader in 2025
# Software OpenGL for WatcomC, DJGPP on DOS and any C/C++

#### Just include and link:  
  GL.H  
  GLIMPL.CPP
####    

Main performance limiting factor by far seems to be memory bandwidth with cache. You need a fast machine to run this on.

You can use this library as a general **C** software rasterizer, it is not limited to DOS.   

# Quirks

Just on WatcomC glSeconds() is stuttery (18.2Hz).

File names are upper case / DOS default. 

# Examples 

Example **void main()** in GL.H  

**Yellow Rose of Texas** is a 4K demo by **FIT/BANDWAGON** (_YELLOW.CPP)    

**Neon Helium Tutorials** found on the net (_NEHE.CPP)  

**GLFW Amiga Boing Ball** found in an old **GLFW** repository (_GLFWBNG.CPP)  

#### WatcomGL demo game: https://github.com/kosmonautdnb/WatcomGL_Demo_Game   

#### WatcomC library collection with e.g. dear ImGui: https://github.com/kosmonautdnb/WatcomLibs 

## Other software OpenGL options

## DJGPP OSMesa on DOS
OSMesa is providing the original software based OpenGL. Maintained for DOS until around 2007.
  
In this rather trashy video featuring **Wipeout** on DOS, it's ingame:
- DJGPP OSMesa around **40** FPS
- DJGPP WatcomGL around **100** FPS  

Video:  
https://youtu.be/eBOSpH6WCXY?si=uEAl7VEM6rkouTIv 
  
DJGPP OSMesa library compile (DOS/OpenGL):
https://bisqwit.iki.fi/jutut/kuvat/programming_examples/djgpp_mesa.zip **zip and compilation not by me**.  

## TinyGL
TinyGL somehow emulates OpenGL in software, too.  
https://github.com/erysdren/TinyGL.git  

# WatcomGL Features
- Header Compatible With OpenGL
- Just One Header And One C File
- More Performant Than The Early OSMesa (The Original DOS OpenGL)
- MIT License For Ease Of Use
- Vesa2.0 Calls For 32Bit Graphics Modes
- Optional Fallback To VGA Mode With 320x200 And 8192 Simulated Colors
- Optional Just Render To 32Bit RGBA Buffer
- Z-Buffer: 32 Bit Float  
- FrameBuffer: 32 Bit RGBA (8 Bit Per Channel)  
- GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_QUADS  
- GL_POINTS With Texture Coordinates
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

# WatcomGL Drawbacks
- No MultiThreading
- No MultiTexturing
- No Assembly/MMX/SSE etc..
- No DisplayLists/VertexBuffers/IndexBuffers
- No Shaders
- Currently Just RGBA 32Bit Framebuffer
- Uses Doubles And Floats
- Just the very oldschool OpenGL

This repository lives at: https://github.com/kosmonautdnb/WatcomGL.git  
