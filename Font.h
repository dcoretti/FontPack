#pragma once
#pragma once

#include "HashTablei.h"
#include <cstdint>

// FontPack
// By Danny Coretti (dcoretti@gmail.com)
// Relatively simple font packing and rendering for 3D graphics API usage such as OpenGL
// Uses stb_truetype and stb_image_write for TTF font analysis/bitmap rendering, and file saving respectively.
// 
// -STB libs generate a bitmap representing the rendered font for a set of code points.  
// -The texture is created by sorting the glyphs by size and packing them into a bitmap by bin-packing 
// using a tree to partition the texture space. 
// -Geometry/UV data is generated to render a string from the glyph data and texture atlas.  
// -Implementation can be modified to serialize/restore the necessary glyph data in order to pre-generate a texture/atlas
// -Currently only uses std::sort from stl (I should get around to just writing a quick replacement).
namespace FontPack {
	struct Vec3 {
		float x;
		float y;
		float z;
	};

	struct Vec2 {
		float x;
		float y;
	};

	struct Glyph {
		int codePoint;
		int advanceWidth; // not scaled by xScale
		int leftSideBearing;

		// scaled width and height with apron for tree placement ONLY (not as accurate as x1-x0 y1-y0
		int width, height;

		// unscaled character coordinates sourced from GetCodepointBox (Y increases up)
		int x0, y0, x1, y1;
		Vec2 uv[6];

		// Texture location
		int bitmapX;
		int bitmapY;
	};

	struct Font {
		Font() : kernTable(512),	// TODO tune this to the number of code point pairs as this will likely resize a few times as is
			scaleX(1.0f),
			scaleY(1.0f),
			heightPixels(0.0f),
			ascent(0),
			descent(0),
			lineGap(0),
			glyphs(nullptr) {
		}

		int textureW;
		int textureH;
		float scaleX;
		float scaleY;

		// defined prior to rasterizing
		float heightPixels; // font height in pixels
		int ascent;
		int descent;
		int lineGap;
		char startCodePoint;
		char endCodePoint;
		const int apron = 1;

		Glyph *glyphs{ nullptr };
		HashTablei kernTable;
	};


	int getCodePointPairKey(int a, int b);


	struct GlyphNode {
		GlyphNode() = default;
		~GlyphNode() {
			if (child[0] != nullptr) {
				delete child[0];
			}
			if (child[1] != nullptr) {
				delete child[1];
			}
		}
		void initChildren(int firstHalfW, int firstHalfH, bool isChildTopBottom);

		// x,y relative to the whole grid, not just the parent
		// 0,0 is at the bottom left
		int x{ 0 };
		int y{ 0 };
		int width{ 0 };
		int height{ 0 };

		// child represents alternating upper/lower half and left/right half of area defined by the parent
		GlyphNode *child[2]{ nullptr, nullptr };
		Glyph *glyph{ nullptr };
	};




	// Load a font file specified by fname, and generate: font statistics in the Font struct, a 1-bit bitmap texture in texture
	// limited to size texW,texH, for a set of ASCII characters in a code point range of fontCodePointStart to fontCodePointEnd
	// example: loadFontToTexture(font, "ComicSans.ttf", texture, 256, 256, ' ', '~'); 
	void loadFontToTexture(Font &font, char * fname, unsigned char * texture, int texW, int texH, char fontCodePointStart, char fontCodePointEnd);

	// Given a font, render a particular string of \0 terminated text as a set of texture mapped vertices.
	void fontTextToGeometry(Font &f, char *text, Vec3 **geometryData, Vec2 **uvData, int *numVertices);
}