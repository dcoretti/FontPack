# FontPack
STB library-based font packing tool and 3d font geometry generator for TTF fonts

This is my attempt at a simple font-packing, texture and geometry data generating tool for use in 3D rendering of text.

## Features
- STB libs used to gather font information and render to the texture.
- Font texture is created by packing glyps into a bitmap using a tree-based bin packing algorithm to partition the space.
- Geometry/uv data is generated to render a string from the glyph data and texture atlas using proper kerning and line advance.  
- Implementation can be modified to serialize/restore the necessary glyph data in order to utilize a pre-generated texture and atlas.
- Currently only uses std::sort from stl to organize glyphs by size for efficient packing (i should get around to just writing a quick drop-replacement).  
- Includes a basic hash table for storing kerning advance for a given glyph pair.

### Example
![Example Image](/example.png?raw=true "Example image")

 Example generated texture using the Windows Arial font at 96.0f pixel max glyph height packed into a 512x512 single-channel image.



## Use
1. Grab stb_truetype and stb_image_write from the [nothings/stb](https://github.com/nothings/stb) repo.
2. #include "font.h"
3. Load a texture: loadFontToTexture(f, "C:/Windows/Fonts/Arial.ttf", texture, texW, texH, ' ', '~');
4. Generate some geometry for some text: fontTextToGeometry(font, "Hi.", &geometry, &uv, &vertices);
5. Get OpenGL or something to render it to the screen!



