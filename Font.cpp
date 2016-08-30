#include "Font.h"
#include <algorithm>	// for std::sort.  TODO, replace this with something inline

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <cstdio>

#ifdef DEBUG
#define DBG_ASSERT(condition, msg, ...) do {if (!(condition)) {fprintf(stderr, "(%s:%d) %s : " msg  "\n", __FILE__, __LINE__, #condition, ##__VA_ARGS__ ); fflush(stderr);  __debugbreak();}} while (0)
#else
#define DBG_ASSERT(condition, msg, ...) 
#endif

namespace FontPack {
	// init children such that is child is a top/bottom split, the top half height is equal to firstHalfH.
	// If left/right split (isChildTopBottom == false) the left half width is equal to firstHalfW
	void GlyphNode::initChildren(int firstHalfW, int firstHalfH, bool isChildTopBottom) {
		child[0] = new GlyphNode();
		child[1] = new GlyphNode();

		if (isChildTopBottom) {
			child[0]->x = x;
			child[0]->y = y + height - firstHalfH;

			child[1]->x = x;
			child[1]->y = y;

			child[0]->width = width;
			child[1]->width = width;
			child[0]->height = firstHalfH;
			child[1]->height = height - firstHalfH;
		} else {
			child[0]->x = x;
			child[0]->y = y;

			child[1]->x = x + firstHalfW;
			child[1]->y = y;

			child[0]->width = firstHalfW;
			child[1]->width = width - firstHalfW;
			child[0]->height = height;
			child[1]->height = height;
		}
	}

	GlyphNode * insert(GlyphNode * tree, Glyph *glyph, bool topBottom) {
		// leaf
		if (tree->child[0] == nullptr && tree->child[1] == nullptr) {
			if (tree->width < glyph->width || tree->height < glyph->height) {
				return nullptr;
			}
			if (tree->glyph != nullptr) {
				return nullptr;
			}

			// either fits perfectly or needs to be split
			if (glyph->height == tree->height && glyph->width == tree->width) {
				// insert here!
				tree->glyph = glyph;
				return tree;
			} else {
				// split node into perfect size and remainder
				tree->initChildren(glyph->width, glyph->height, !topBottom);
				return insert(tree->child[0], glyph, !topBottom); // glyph will fit into left half
			}

		} else {
			// non-leaf
			GlyphNode *node = insert(tree->child[0], glyph, !topBottom);
			if (node != nullptr) {
				return node;
			} else {
				return insert(tree->child[1], glyph, !topBottom);
			}
		}
	}

	GlyphNode * insert(GlyphNode * tree, Glyph * glyph) {
		// All trees starts with top half bottom half level
		return insert(tree, glyph, false);
	}

	float clamp1(float f) {
		return f > 1.0f ? 1.0f : f;
	}

	Vec2 pixelToTexel(int pixelX, int pixelY, int w, int h) {
		return{ clamp1((2.0f * pixelX) / (2.0f * w)), clamp1((2.0f * pixelY) / (2.0f * h)) };
	}

	Vec2 pixelToTexelf(float pixelX, float pixelY, int w, int h) {
		return{ clamp1((2.0f * pixelX) / (2.0f * w)), clamp1((2.0f * pixelY) / (2.0f * h)) };
	}

	void packGlyphs(unsigned char *texture, int texW, int texH, Glyph* glyphs, int numGlyphs, stbtt_fontinfo *font, float scaleX, float scaleY, int apron) {
		GlyphNode *tree = new GlyphNode();
		tree->x = 0;
		tree->y = 0;
		tree->width = texW;
		tree->height = texH;

		// insert the first node
		DBG_ASSERT(texH >= glyphs[0].height, "texture not tall enough for largest character");
		DBG_ASSERT(texW >= glyphs[0].width, "texture not wide enough for largest character");

		for (int i = 0; i < numGlyphs; i++) {
			GlyphNode * node = insert(tree, &glyphs[i]);
			DBG_ASSERT(node != nullptr, "glyph could not fit!");
			if (node != nullptr) {
				node->glyph = &glyphs[i];
				Glyph * g = node->glyph;

				g->bitmapX = node->x;
				g->bitmapY = node->y;

				// Find where the actual glyph is placed in the texture and inside the bounding box for the glyph node
				// which includes a N-pixel apron.
				// get the subset of the texture where the glyph is at the top left (bitmap 0,0 is top left)
				int offsetRow = texH - (node->y + g->height); // g->height already has apron
				DBG_ASSERT(offsetRow < texH, "Offset row %d higher than texh %d", offsetRow, texH);
				// subsection of the texture such that the current element is at the top left
				unsigned char * textureStart = texture + (offsetRow * texW + node->x + apron);

				stbtt_MakeCodepointBitmap(font,
					textureStart,
					g->width,
					g->height,
					texW,
					scaleX,
					scaleY,
					g->codePoint);

				g->uv[0] = pixelToTexel(g->bitmapX + apron, g->bitmapY + apron, texW, texH);
				g->uv[1] = pixelToTexel(g->bitmapX + g->width - apron, g->bitmapY + apron, texW, texH);
				g->uv[2] = pixelToTexel(g->bitmapX + g->width - apron, g->bitmapY + g->height - apron, texW, texH);
				g->uv[3] = g->uv[0];
				g->uv[4] = g->uv[2];
				g->uv[5] = pixelToTexel(g->bitmapX + apron, g->bitmapY + g->height - apron, texW, texH);
			}
		}
		delete tree;
	}


	void flipTexture(unsigned char * texture, int texW, int texH) {
		unsigned char * tmpRow = new unsigned char[texW]();

		for (int i = 0; i < texH / 2; i++) {
			for (int j = 0; j < texW; j++) {
				tmpRow[j] = texture[texW * i + j];
			}
			// copy lower row to upper
			for (int j = 0; j < texW; j++) {
				texture[texW * i + j] = texture[texW * (texH - i - 1) + j];
			}
			// copy tmp to lower
			for (int j = 0; j < texW; j++) {
				texture[texW * (texH - i - 1) + j] = tmpRow[j];
			}
		}
	}

	// Load a font and create a packed texture.  Only needs to be run once to cache the texture
	// TODO Font data should be persisted too in order to allow rendering of text with pre-generated texture image.
	void loadFontToTexture(Font &font, char * fname, unsigned char * texture, int texW, int texH, char fontCodePointStart, char fontCodePointEnd) {
		font.startCodePoint = fontCodePointStart;
		font.endCodePoint = fontCodePointEnd;

		stbtt_fontinfo fontInfo;
		FILE * f = fopen(fname, "rb");
		DBG_ASSERT(f != NULL, "File unable to be opened");
		fseek(f, 0, SEEK_END);
		size_t size = ftell(f); /* how long is the file ? */
		fseek(f, 0, SEEK_SET); /* reset */
		unsigned char *buffer = new unsigned char[size];
		DBG_ASSERT(1 == fread(buffer, size, 1, f), "problem reading ");
		fclose(f);

		font.textureW = texW;
		font.textureH = texH;
		stbtt_InitFont(&fontInfo, buffer, stbtt_GetFontOffsetForIndex(buffer, 0));
		font.scaleY = stbtt_ScaleForPixelHeight(&fontInfo, font.heightPixels);
		font.scaleX = font.scaleY;
		stbtt_GetFontVMetrics(&fontInfo, &font.ascent, &font.descent, &font.lineGap);

		int numGlyphs = font.endCodePoint - font.startCodePoint;
		font.glyphs = new Glyph[numGlyphs];
		for (int c = 0; c < numGlyphs; c++) {
			Glyph *g = &font.glyphs[c];
			// CodepointBox uses the y increasing up vs bitmapbox
			stbtt_GetCodepointBox(&fontInfo, c + font.startCodePoint, &g->x0, &g->y0, &g->x1, &g->y1);

			// Add a width/height apron for glyph placement in tree.  Adding 2x apron to add apron to both sides.
			// on glyph rendering the glyph itself will be rendered within that apron.
			g->height = (int)((float)(g->y1 - g->y0) * font.scaleY) + 2 * font.apron;
			g->width = (int)((float)(g->x1 - g->x0) * font.scaleX) + 2 * font.apron;
			g->codePoint = c + font.startCodePoint;

			stbtt_GetCodepointHMetrics(&fontInfo, c + font.startCodePoint, &g->advanceWidth, &g->leftSideBearing);
		}

		// sort in size order to get a bit better packing by inserting in height order.
		// Chose height due to the fact that the tree has a property of causing an entire strip along the x axis to be constrained by the height
		// of the previous glyph.  This is due to the constraint of a glyph having to fit perfectly in a given rectangle where we split a parent
		// rectangle top to bottom first then left to right.  The top/bottom split means that any further division can fit glyphs <= that glyphs height.
		// Sorting by height means that any subsequent glyph will fit potentially right next to the previous.
		std::sort(font.glyphs, font.glyphs + numGlyphs,
			[](Glyph &a, Glyph &b) -> bool { return a.height > b.height; });
		packGlyphs(texture, texW, texH, font.glyphs, numGlyphs, &fontInfo, font.scaleX, font.scaleY, font.apron);
		std::sort(font.glyphs, font.glyphs + numGlyphs,
			[](Glyph &a, Glyph &b) -> bool { return a.codePoint < b.codePoint; });

		font.kernTable = new int[numGlyphs * numGlyphs]();
		for (int i = 0; i < numGlyphs; i++) {
			for (int j = 0; j < numGlyphs; j++) {
				int kernAdvance = stbtt_GetCodepointKernAdvance(&fontInfo, font.glyphs[i].codePoint, font.glyphs[j].codePoint);
				DBG_ASSERT(font.glyphs[i].codePoint < INT16_MAX && font.glyphs[i].codePoint > INT16_MIN, "code point out of bounds");
				DBG_ASSERT(font.glyphs[j].codePoint < INT16_MAX && font.glyphs[i].codePoint > INT16_MIN, "code point out of bounds");

				int cp1 = font.glyphs[i].codePoint - font.startCodePoint;
				int cp2 = font.glyphs[j].codePoint - font.startCodePoint;
				font.kernTable[cp1 * numGlyphs + cp2] = kernAdvance;
			}
		}
		DBG_ASSERT(stbi_write_png("out.png", texW, texH, 1, texture, texW) > 0, "Unable to write file!");
		// Flip the local representation of the texture since we want it in opengl format (0,0 bottom left).  STB does flipping on load from a file
		// and generally expects 0,0 to be top left. 
		flipTexture(texture, texW, texH);

		delete[] buffer;
	}

	// Used as hash for kerning key pairs to map [key1,key2] tuple -> kerning advance 
	int FontPack::getCodePointPairKey(int a, int b) {
		return (a << 16) | (b & 0xffff);
	}

	//output needed is to generate a set of geometry, uv data to be sent to the gpu
	void fontTextToGeometry(Font &f, char *text, Vec3 **geometryData, Vec2 **uvData, int *numVertices) {
		int numGlyphs = f.endCodePoint - f.startCodePoint;
		int len = 0;
		for (char * c = text; *c; c++, len++) {
		}
		*geometryData = new Vec3[len * 6]();
		*uvData = new Vec2[len * 6];

		float startX = 0;
		float startY = 0;
		int cur = 0;

		float fontOrigin = f.heightPixels / 2.0f;
		float fontRelativeDistance = (fontOrigin / f.heightPixels) - (f.descent / f.heightPixels);
		for (char * c = text; *c; c++, cur += 6) {
			if (*c == '\n') {
				// line advance
				startY -= (f.scaleY * (float)(f.ascent - f.descent + f.lineGap));
				startX = 0.0f;
				continue;
			}
			DBG_ASSERT(*c - f.startCodePoint >= 0, "Font char %s not valid", c);
			Glyph & glyph = f.glyphs[*c - f.startCodePoint];

			float x0 = startX;
			float x1 = x0 + (f.scaleX * (float)(glyph.x1 - glyph.x0));
			float y0 = startY + (float)(glyph.y0 + f.ascent) * f.scaleY;
			float y1 = y0 + ((float)(glyph.y1 - glyph.y0) * f.scaleY);
			*(geometryData)[cur] = { x0, y0, 0.0f };
			*(geometryData)[cur + 1] = { x1, y0, 0.0f };
			*(geometryData)[cur + 2] = { x1, y1, 0.0f };
			*(geometryData)[cur + 3] = *(geometryData)[cur];
			*(geometryData)[cur + 4] = *(geometryData)[cur + 2];
			*(geometryData)[cur + 5] = { x0, y1, 0.0f };

			for (int i = 0; i < 6; i++) {
				*(uvData)[cur + i] = glyph.uv[i];
			}

			startX += ((glyph.advanceWidth - glyph.leftSideBearing)* f.scaleX);//glyph.width;
			if ((c + 1)) {
				//stbtt_GetCodepointKernAdvance(&, word[i], word[i + 1]);
				int cp1 = *c - f.startCodePoint;
				int cp2 = *(c+1) - f.startCodePoint;

				int kernAdvance = f.kernTable[cp1 * numGlyphs + cp2];
				startX += (kernAdvance * f.scaleX);
			}
		}

		/*
		// Example OpenGL Code for loading the geometry data into a vao
		unsigned int uvb;
		unsigned int vb;
		unsigned int shaderAttributeBindingUv = 1;
		unsigned int shaderAttributeBindingVertices = 0;

		//1. create a VAO
		unsigned int vao;
		glGenVertexArrays(1, vao);
		glBindVertexArray(*vao);
		//2. load the UV and vertex buffers
		glGenBuffers(1, &vb);
		glBindBuffer(GL_ARRAY_BUFFER, vb);
		glBufferData(GL_ARRAY_BUFFER, len * 6 * sizeof(Vec3), geometryData, GL_STATIC_DRAW);
		// Bind this array buffer to the shader layout and define the data format.
		glVertexAttribPointer(shaderAttributeBindingVertices, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(shaderBinding);
	
		glGenBuffers(1, &uvb);
		glBindBuffer(GL_ARRAY_BUFFER, uvb);
		glBufferData(GL_ARRAY_BUFFER, len * 6 * sizeof(Vec2), uvData, GL_STATIC_DRAW);

		// Bind this array buffer to the shader layout and define the data format.
		glVertexAttribPointer(shaderAttributeBindingUv, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(shaderBinding);

		delete[] geometryData;
		delete[] uvData;
		*/

		*numVertices = len * 6;
	}

}



