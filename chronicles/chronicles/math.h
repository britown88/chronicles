#pragma once
#include "defs.h"

typedef struct {
   i32 x, y;
} Int2;

typedef struct {
   i32 x, y, z;
} Int3;

typedef struct {
   f32 x, y;
} Float2;

typedef struct {
   f32 x, y, z;
} Float3;

Float3 vCross(Float3 a, Float3 b);
f32 vDot(Float3 a, Float3 b);
Float3 vSubtract(Float3 a, Float3 b);
Float3 vAdd(Float3 a, Float3 b);
Float3 vNormalized(Float3 v);
Float3 *vNormalize(Float3 *v);
Float3 vScale(Float3 v, f32 s);

typedef struct {
   i32 x, y, w, h;
} Recti;

static void rectiOffset(Recti *r, i32 x, i32 y) {
   r->x += x;
   r->y += y;
}

static bool rectiContains(Recti r, Int2 p) {
   if (p.x < r.x ||
      p.y < r.y ||
      p.x >= r.x + r.w ||
      p.y >= r.y + r.h) return false;
   return true;
}

static bool rectiIntersects(Recti a, Recti b) {
   if (a.x >= b.x + b.w ||
      a.y >= b.y + b.h ||
      b.x >= a.x + a.w ||
      b.y >= a.y + a.h) return false;
   return true;
}
