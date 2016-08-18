# FontPack
STB library-based font packing tool and 3d font geometry generator for TTF fonts

This is my attempt at a simple font-packing, texture and geometry data generating tool for use in 3D rendering of text.

## Features
- stb libs generate a bitmap representing the rendered font for a set of code points.  
- the texture is created by sorting the glyphs by size and packing them into a bitmap by bin-packing using a tree to partition the texture space. 
- geometry/uv data is generated to render a string from the glyph data and texture atlas.  
- implementation can be modified to serialize/restore the necessary glyph data in order to pre-generate a texture/atlas
- currently only uses std::sort from stl (i should get around to just writing a quick replacement).

### Example
![Example Image](/example.png?raw=true "Example image")

 Example generated texture using Windows Arial font @ 96.0f pixel height for glyphs packed into a 512x512 single-channel texture.



## Use
1. Grab stb_truetype and stb_image_write from the [nothings/stb](https://github.com/nothings/stb) repo.
2. #include "font.h"
3. Load a texture: loadFontToTexture(f, "C:/Windows/Fonts/Arial.ttf", texture, texW, texH, ' ', '~');
4. Generate some geometry for some text: fontTextToGeometry(font, "Hi.", &geometry, &uv, &vertices);
5. Get OpenGL or something to render it to the screen!



