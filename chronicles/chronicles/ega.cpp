#include "ega.h"
#include "app.h"

#include <string.h>
#include <list>
#include <vector>
#include <algorithm>

byte getBit(byte dest, byte pos/*0-7*/) {
   return !!(dest & (1 << (pos & 7)));
}

static void _buildColorTable(ColorRGB *table) {
   byte i;
   //                   00 01  10   11
   byte rgbLookup[] = { 0, 85, 170, 255 };
   for (i = 0; i < EGA_COLORS; ++i) {
      byte shift = 5;

      byte r = getBit(i, shift--);
      byte g = getBit(i, shift--);
      byte b = getBit(i, shift--);
      byte R = getBit(i, shift--);
      byte G = getBit(i, shift--);
      byte B = getBit(i, shift);

      byte rgb_r = rgbLookup[(R << 1) + r];
      byte rgb_g = rgbLookup[(G << 1) + g];
      byte rgb_b = rgbLookup[(B << 1) + b];

      table[i] = ColorRGB{ rgb_r, rgb_g, rgb_b };
   }
}

ColorRGB egaGetColor(EGAColor c) {
   static ColorRGB lookup[EGA_COLORS] = { 0 };
   static bool loaded = 0;

   if (!loaded) {
      _buildColorTable(lookup);
      loaded = 1;
   }
   return lookup[c];
}

/*
pixel data organization

alpha is stored as 1 bit per pixel in scanlines
pixel data is stored plane-interleaved with 4 bits per pixel
the lower 4 bits are leftmost in the image and higher 4 bits are rightmost

[byte 0  ] [byte 1]
[MSB][LSB] [MSB][LSB]
[x:1][x:0] [x:3][x:2]

alpha scanlines are bytealigned so use alphaSLwidth for traversing

pixel scanlines need enough room for a 4bit right-shift and
so may have an extra byte at the end, use pixelSLWidth for traversiong

pixel data is also stored in an offset form where each scanline is left-shifted by 4
so that the scanline can be memcpy'd directly onto the target if its not aligned
this buffer is generated on-demand with offsetDirty
*/

enum Tex_ {
   Tex_DECODE_DIRTY = (1 << 0),
   Tex_OFFSET_DIRTY = (1 << 1),
   Tex_ALL_DIRTY = (Tex_DECODE_DIRTY | Tex_OFFSET_DIRTY)
};
typedef byte TexCleanFlag;

struct EGATexture {
   u32 w = 0, h = 0;
   EGARegion fullRegion = { 0 };

   u32 alphaSLWidth = 0; //size in bytes of the alpha channel scanlines
   u32 pixelSLWidth = 0; //size in bytes of the pixel datga scanlines
   u32 pixelCount = 0;

   // 1 bit per pixel, 0 for transparent
   byte *alphaChannel = nullptr;

   byte *pixelData = nullptr;
   byte *pixelDataOffset = nullptr;

   ColorRGBA *decodePixels = nullptr;
   EGAPalette lastDecodedPalette = { 0 };

   TexCleanFlag dirty = Tex_ALL_DIRTY;
};

static void _freeTextureBuffers(EGATexture *self) {
   if (self->alphaChannel) {
      delete[] self->alphaChannel;
      self->alphaChannel = nullptr;
   }

   if (self->decodePixels) {
      delete[] self->decodePixels;
      self->decodePixels = nullptr;
   }

   if (self->pixelData) {
      delete[] self->pixelData;
      self->pixelData = nullptr;
   }

   if (self->pixelDataOffset) {
      delete[] self->pixelDataOffset;
      self->pixelDataOffset = nullptr;
   }
}


EGATexture *egaTextureCreate(u32 width, u32 height) {
   EGATexture *self = new EGATexture();

   egaTextureResize(self, width, height);

   return self;
}
void egaTextureDestroy(EGATexture *self) {
   _freeTextureBuffers(self);
   delete self;
}

#pragma region OLD ENCODING CODE

struct PaletteColor;

struct PaletteEntry {
   PaletteEntry *next, *prev;
   float distance;
   PaletteColor* color;
};

typedef PaletteEntry* pPaletteEntry;

struct PaletteColor {
   byte removable;
   byte pPos;
   byte EGAColor;
   float distance;
   PaletteEntry entries[64];
   PaletteColor() :removable(1) {}
   PaletteColor(byte c) :EGAColor(c), removable(1) {}
   PaletteColor(byte c, byte targetPalettPosition) :EGAColor(c), removable(0), pPos(targetPalettPosition) {}
};

struct ImageColor {
   pPaletteEntry closestColor;
   ImageColor() :closestColor(0) {}
};

float GCRGB(byte component) {
   static float GCRGBTable[256] = { 0.0f };
   static int loaded = 0;

   if (!loaded) {
      int i;
      for (i = 0; i < 256; ++i) {
         GCRGBTable[i] = pow(i / 255.0f, 2.2f);
      }
      loaded = 1;
   }

   return GCRGBTable[component];
}

static f32 sRGB(byte b) {
   f32 x = b / 255.0f;
   if (x <= 0.00031308f) {
      return 12.92f * x;
   }
   else {
      return 1.055f*pow(x, (1.0f / 2.4f)) - 0.055f;
   }
}

Float3 srgbToLinear(ColorRGB const&srgb) {
   return { sRGB(srgb.r), sRGB(srgb.g), sRGB(srgb.b) };
}
Float3 srgbToLinear(ColorRGBA const&srgb) {
   return { sRGB(srgb.r), sRGB(srgb.g), sRGB(srgb.b) };
}

f32 vDistSquared(Float3 const& a, Float3 const& b) {
   return pow(b.x - a.x, 2) + pow(b.y - a.y, 2) + pow(b.z - a.z, 2);
}

float colorDistance(ColorRGBA c1, ColorRGBA c2) {
   float r, g, b;
   r = GCRGB(c1.r) - GCRGB(c2.r);
   g = GCRGB(c1.g) - GCRGB(c2.g);
   b = GCRGB(c1.b) - GCRGB(c2.b);

   return r * r + g * g + b * b;
}

float colorDistanceLinear(ColorRGBA c1, ColorRGBA c2) {
   return vDistSquared(srgbToLinear(c1), srgbToLinear(c2));
}

ColorRGBA EGAColorLookup(byte c) {
   auto ci = egaGetColor(c);
   ColorRGBA r = {ci.r, ci.g, ci.b, 255};
   return r;
}

void insertSortedPaletteEntry(ImageColor &color, PaletteColor &parent, byte target, byte current, PaletteEntry &out) {

   out.color = &parent;
   out.distance = sqrt(colorDistance(EGAColorLookup(target), EGAColorLookup(current)));

   auto iter = color.closestColor;
   if (!iter) {
      out.next = out.prev = nullptr;
      color.closestColor = &out;
   }
   else {
      PaletteEntry *prev = nullptr;
      while (iter && out.distance > iter->distance) {
         prev = iter;
         iter = iter->next;
      }

      if (!prev) {
         out.prev = nullptr;
         out.next = color.closestColor;
         out.next->prev = &out;
         color.closestColor = &out;
      }
      else {
         out.next = iter;
         out.prev = prev;
         if (out.next) {
            out.next->prev = &out;
         }

         out.prev->next = &out;
      }
   }
}

bool isClosest(ImageColor &color, PaletteEntry &entry) {
   return color.closestColor == &entry;
}

void removeColorEntries(PaletteColor *removedColor, ImageColor *colors) {
   for (auto& entry : removedColor->entries) {
      for (auto color = colors; color != colors + 64; ++color) {
         if (color->closestColor == &entry) {
            color->closestColor = entry.next;
         }
      }
      if (entry.prev) entry.prev->next = entry.next;
      if (entry.next) entry.next->prev = entry.prev;
   }
}

struct rgbega {
   int rgb;
   byte ega;
   rgbega() {}
   rgbega(int _rgb, byte _ega) :rgb(_rgb), ega(_ega) {}
   bool operator<(int other) {
      return rgb < other;
   }
};

byte closestEGA(int rgb) {
   float lowest = 1000.0;
   int closest = 0;

   for (byte i = 0; i < 64; ++i) {
      auto c = EGAColorLookup(i);

      float diff = colorDistance(*(ColorRGBA*)&rgb, c);

      if (diff < lowest) {
         lowest = diff;
         closest = i;
      }
   }

   return closest;
}

#pragma endregion

EGATexture *egaTextureCreateFromTextureEncode(Texture *source, EGAPalette *targetPalette, EGAPalette *resultPalette) {
   int colorCounts[64];

   auto texSize = textureGetSize(source);

   auto pixelCount = texSize.x * texSize.y;
   byte* alpha = new byte[pixelCount];
   byte* pixelMap = new byte[pixelCount];
   std::vector<int> cArray(pixelCount);

   memset(resultPalette->colors, 0, 16);
   memset(colorCounts, 0, sizeof(int) * 64);
   memset(alpha, 0, pixelCount);
   memset(pixelMap, 0, pixelCount);

   auto texColors = textureGetPixels(source);

   //push every pixel into a vector
   for (int i = 0; i < texSize.x * texSize.y; ++i) {
      alpha[i] = texColors[i].a == 255;
      cArray[i] = *(int*)&texColors[i];
   }

   //sort and unique
   std::sort(begin(cArray), end(cArray));
   cArray.erase(std::unique(cArray.begin(), cArray.end()), cArray.end());

   //map the unique colors to their ega equivalents
   std::vector<rgbega> colorMap(cArray.size());

   for (unsigned int i = 0; i < cArray.size(); ++i)
      colorMap[i] = rgbega(cArray[i], closestEGA(cArray[i]));

   //go throuygh the image and log how often each EGA color appears
   for (int i = 0; i < pixelCount; ++i) {
      int c = *(int*)&texColors[i];

      if (texColors[i].a != 255) {
         pixelMap[i] = 0;
         continue;
      }

      byte ega = std::lower_bound(begin(colorMap), end(colorMap), c)->ega;

      pixelMap[i] = ega;
      colorCounts[ega]++;
   }


   auto p = targetPalette->colors;

   byte forced[64];
   memset(forced, EGA_COLOR_UNUSED, 64);

   byte totalCount = 0;
   for (int i = 0; i < 16; ++i) {
      if (p[i] != EGA_COLOR_UNUSED) {
         if (p[i] != EGA_COLOR_UNDEFINED) {
            forced[p[i]] = totalCount;
         }

         ++totalCount;
      }
   }

   std::list<PaletteColor> palette;
   ImageColor colors[64];
   for (int i = 0; i < 64; ++i)
   {
      if (forced[i] != EGA_COLOR_UNUSED) {
         palette.push_back(PaletteColor(i, forced[i]));
      }
      else {
         palette.push_back(PaletteColor(i));
      }

      for (int j = 0; j < 64; ++j)
      {
         insertSortedPaletteEntry(colors[j], palette.back(), j, i, palette.back().entries[j]);
      }
   }

   while (palette.size() > totalCount)
   {
      //worst color, worst error...
      float lowestDistance = FLT_MAX;
      std::list<PaletteColor>::iterator rarestColor;

      for (auto color = palette.begin(); color != palette.end(); ++color) //do this with iterators to erase.
      {
         float distance = 0.0f;
         for (auto& entry : color->entries)
         {
            if (isClosest(colors[color->EGAColor], entry))
            {
               distance += colorCounts[color->EGAColor] * (entry.next->distance - entry.distance);
            }
         }
         if (distance < lowestDistance && color->removable)
         {
            lowestDistance = distance;
            rarestColor = color;
         }
      }

      //remove rarest color, and all of its palette entries in the colors array....
      removeColorEntries(&*rarestColor, colors);
      palette.erase(rarestColor);
   }

   //eliminate unused colors
   for (auto color = palette.begin(); color != palette.end();) {
      if (color->removable) {
         color->distance = 0.0f;

         for (auto& entry : color->entries) {
            if (isClosest(colors[color->EGAColor], entry)) {
               color->distance += colorCounts[color->EGAColor] * (entry.next->distance - entry.distance);
            }
         }

         if (color->distance == 0.0f) {
            //color wasnt used
            removeColorEntries(&*color, colors);
            color = palette.erase(color);
            continue;
         }
      }
      ++color;
   }

   palette.sort([&](const PaletteColor&r1, const PaletteColor&r2) {return r1.distance > r2.distance; });


   //this also gives you the look-up table on output...
   byte paletteOut[16];
   memset(paletteOut, EGA_COLOR_UNUSED, 16);
   int LUTcolor = 0;

   //two passes, first to inster colors who have locked positions in the palette
   for (auto& color : palette) {
      if (!color.removable) {
         paletteOut[color.pPos] = color.EGAColor;
         color.EGAColor = color.pPos;
      }
   }

   //next is to fill in the blanks with the rest
   for (auto& color : palette)
   {
      if (color.removable) {
         while (paletteOut[LUTcolor] != EGA_COLOR_UNUSED) { LUTcolor += 1; };
         paletteOut[LUTcolor] = color.EGAColor;
         color.EGAColor = LUTcolor++;
      }
   }

   byte colorLUT[64]; //look-up table from 64 colors down the 16 remaining colors.
   LUTcolor = 0;
   for (auto& color : colors){
      colorLUT[LUTcolor++] = color.closestColor->color->EGAColor;
   }

   memcpy(resultPalette->colors, paletteOut, 16);

   auto out = egaTextureCreate(texSize.x, texSize.y);
   egaClearAlpha(out);
   for (int i = 0; i < pixelCount; ++i) {
      

      if (texColors[i].a == 255) {
         EGAColor c = colorLUT[pixelMap[i]];

         auto x = i % texSize.x;
         auto y = i / texSize.x;

         egaRenderPoint(out, { x, y }, c);
      }
   }

   delete[] alpha;
   delete[] pixelMap;

   return out;
}

// target must exist and must match ega's size, returns !0 on success
int egaTextureDecode(EGATexture *self, Texture* target, EGAPalette *palette){

   auto texSize = textureGetSize(target);
   if (texSize.x != self->w || texSize.y != self->h) {
      return 0;
   }

   if (!self->decodePixels) {
      self->decodePixels = new ColorRGBA[self->w * self->h];
      self->dirty |= Tex_DECODE_DIRTY;
   }

   //palette changed!
   if (memcmp(palette->colors, self->lastDecodedPalette.colors, sizeof(EGAPalette))) {
      self->lastDecodedPalette = *palette;
      self->dirty |= Tex_DECODE_DIRTY;
   }
   
   if (self->dirty&Tex_DECODE_DIRTY) {
      memset(self->decodePixels, 0, self->pixelCount * sizeof(ColorRGBA));
      u32 x, y;
      u32 asl = 0, psl = 0, dsl = 0;
      for (y = 0; y < self->h; ++y) {

         for (x = 0; x < self->w; ++x) {


            if (self->alphaChannel[asl + (x >> 3)] & (1 << (x & 7))) {
               byte twoPix = self->pixelData[psl + (x >> 1)];
               EGAPColor pIdx = x & 1 ? twoPix >> 4 : twoPix & 15;
               ColorRGB rgb = egaGetColor(palette->colors[pIdx]);

               self->decodePixels[dsl + x] = ColorRGBA{ rgb.r, rgb.g, rgb.b, 255 };
            }
         }

         asl += self->alphaSLWidth; //alpha byte position
         psl += self->pixelSLWidth; //pixel byte position
         dsl += self->w; //decode pixel position
      }

      self->dirty &= ~Tex_DECODE_DIRTY;
   }

   textureSetPixels(target, (byte*)self->decodePixels);
   return 1;
}

int egaTextureSerialize(EGATexture *self, byte **outBuff, u64 *size) {
   return 0;
}
EGATexture *egaTextureDeserialize(byte *buff, u64 size) {
   return nullptr;
}

void egaTextureResize(EGATexture *self, u32 width, u32 height) {
   if (width == self->w && height == self->h) {
      return;
   }

   _freeTextureBuffers(self);

   self->w = width;
   self->h = height;
   self->pixelCount = self->w * self->h;
   self->fullRegion = EGARegion{ 0, 0, (i32)self->w, (i32)self->h };

   // w/8 + w%8 ? 1 : 0
   self->alphaSLWidth = (self->w >> 3) + ((self->w & 7) ? 1 : 0);

   //add an extra byte if width is even (odd has extra half byte)
   self->pixelSLWidth = (self->w >> 1) + ((self->w & 1) ? 0 : 1);

   self->alphaChannel = new byte[self->h * self->alphaSLWidth];
   self->pixelData = new byte[self->h * self->pixelSLWidth];

   self->dirty = Tex_ALL_DIRTY;
}

u32 egaTextureGetWidth(EGATexture *self) { return self->w; }
u32 egaTextureGetHeight(EGATexture *self) { return self->h; }
EGARegion *egaTextureGetFullRegion(EGATexture *self) { return &self->fullRegion; }

EGAPColor egaTextureGetColorAt(EGATexture *self, EGARegion *vp, u32 x, u32 y) {
   if (!vp) { vp = &self->fullRegion; }
   return 0;
}

struct EGAFontFactory {
   EMPTY_STRUCT;
};

struct EGAFont {
   EMPTY_STRUCT;
};

EGAFontFactory *egaFontFactoryCreate(EGATexture *font) {
   return nullptr;
}
void egaFontFactoryDestroy(EGAFontFactory *self) {

}
EGAFont *egaFontFactoryGetFont(EGAFontFactory *self, EGAColor bgColor, EGAColor fgColor) {
   return nullptr;
}

void egaClear(EGATexture *target, EGAPColor color, EGARegion *vp) {
   if (!vp) {
      //fast clear
      byte *a = target->alphaChannel;
      byte *p = target->pixelData;
      for (u32 i = 0; i < target->h; ++i) {
         memset(a, 255, target->alphaSLWidth);
         byte c = color | (color << 4);
         memset(p, c, target->pixelSLWidth);

         a += target->alphaSLWidth;
         p += target->pixelSLWidth;
      }

      target->dirty = Tex_ALL_DIRTY;
   }
   else {
      //region clear is just a rect render on the vp
      egaRenderRect(target, *vp, color);
   }
}
void egaClearAlpha(EGATexture *target) {
   memset(target->alphaChannel, 0, target->alphaSLWidth * target->h);
   target->dirty = Tex_ALL_DIRTY;
}
void egaRenderTexture(EGATexture *target, Int2 pos, EGATexture *tex, EGARegion *vp) {
   if (!vp) { vp = &target->fullRegion; }
}
void egaRenderTexturePartial(EGATexture *target, Int2 pos, EGATexture *tex, Recti uv, EGARegion *vp) {
   if (!vp) { vp = &target->fullRegion; }
}
void egaRenderPoint(EGATexture *target, Int2 pos, EGAPColor color, EGARegion *vp) {
   if (!vp) { vp = &target->fullRegion; }

   pos.x += vp->x;
   pos.y += vp->y;

   if (pos.x >= vp->w || pos.y >= vp->h) { return; }

   auto alphaPtr = target->alphaChannel + target->alphaSLWidth * pos.y + (pos.x >> 3);
   *alphaPtr |= 1 << (pos.x & 7);


   auto ptr = target->pixelData + target->pixelSLWidth * pos.y + (pos.x >> 1);
   if (pos.x & 1) {
      *ptr = ((*ptr)&0x0F) | (color << 4);
   }
   else {
      *ptr = ((*ptr) & 0xF0) | color;
   }

   target->dirty = Tex_ALL_DIRTY;
}
void egaRenderLine(EGATexture *target, Int2 pos1, Int2 pos2, EGAPColor color, EGARegion *vp) {
   int dx = abs(pos2.x - pos1.x);
   int dy = abs(pos2.y - pos1.y);
   int x0, x1, y0, y1;
   float x, y, slope;

   //len=0
   if (!dx && !dy) {
      return;
   }

   if (dx > dy) {
      if (pos1.x > pos2.x) {//flip
         x0 = pos2.x; y0 = pos2.y;
         x1 = pos1.x; y1 = pos1.y;
      }
      else {
         x0 = pos1.x; y0 = pos1.y;
         x1 = pos2.x; y1 = pos2.y;
      }

      x = x0;
      y = y0;
      slope = (float)(y1 - y0) / (float)(x1 - x0);

      while (x < x1) {
         egaRenderPoint(target, {(i32) x, (i32) y }, color, vp);

         x += 1.0f;
         y += slope;
      }

      egaRenderPoint(target, { (i32)x1, (i32)y1 }, color, vp);
   }
   else {
      if (_y0 > _y1) {//flip
         x0 = pos2.x; y0 = pos2.y;
         x1 = pos1.x; y1 = pos1.y;
      }
      else {
         x0 = pos1.x; y0 = pos1.y;
         x1 = pos2.x; y1 = pos2.y;
      }

      x = x0;
      y = y0;
      slope = (float)(x1 - x0) / (float)(y1 - y);

      while (y < y1) {

         egaRenderPoint(target, { (i32)x, (i32)y }, color, vp);

         y += 1.0f;
         x += slope;
      }

      egaRenderPoint(target, { (i32)x1, (i32)y1 }, color, vp);
   }
}
void egaRenderLineRect(EGATexture *target, Recti r, EGAPColor color, EGARegion *vp) {
   if (!vp) { vp = &target->fullRegion; }
}
void egaRenderRect(EGATexture *target, Recti r, EGAPColor color, EGARegion *vp) {
   if (!vp) { vp = &target->fullRegion; }
}

void egaRenderCircle(EGATexture *target, Int2 pos, int radius, EGAPColor color, EGARegion *vp) {
   if (!vp) { vp = &target->fullRegion; }
}
void egaRenderEllipse(EGATexture *target, Recti r, EGAPColor color, EGARegion *vp) {
   if (!vp) { vp = &target->fullRegion; }
}
void egaRenderEllipseQB(EGATexture *target, Int2 pos, int radius, double aspect, EGAPColor color, EGARegion *vp) {
   if (!vp) { vp = &target->fullRegion; }
}

void egaRenderTextSingleChar(EGATexture *target, const char c, Int2 pos, EGAFont *font, int spaces) {

}
void egaRenderText(EGATexture *target, const char *text, Int2 pos, EGAFont *font) {

}
void egaRenderTextWithoutSpaces(EGATexture *target, const char *text, Int2 pos, EGAFont *font) {

}