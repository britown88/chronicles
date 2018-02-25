#include "ega.h"
#include "app.h"

#include <string.h>

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

EGATexture *egaTextureCreateFromTextureEncode(Texture *source, EGAPalette *targetPalette, EGAPalette *resultPalette) {
   return nullptr;
}

// target must exist and must match ega's size, returns !0 on success
int egaTextureDecode(EGATexture *self, Texture* target, EGAPalette *palette){

   auto texSize = textureGetSize(target);
   if (texSize.x != self->w || texSize.y != self->h) {
      return 0;
   }

   if (!self->decodePixels) {
      self->decodePixels = new ColorRGBA[self->w * self->h];
   }
   
   //if (self->dirty&Tex_DECODE_DIRTY) {
      memset(self->decodePixels, 0, self->pixelCount);
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
   //}

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

   auto ptr = target->pixelData + target->pixelSLWidth * pos.y + (pos.x >> 1);
   if (pos.x & 1) {
      *ptr = ((*ptr)&0x0F) | (color << 4);
   }
   else {
      *ptr = ((*ptr) & 0xF0) | color;
   }
}
void egaRenderLine(EGATexture *target, Int2 pos1, Int2 pos2, EGAPColor color, EGARegion *vp) {
   if (!vp) { vp = &target->fullRegion; }
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