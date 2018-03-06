#include "math.h"


i32 int2Dot(Int2 v1, Int2 v2) {
   return v1.x * v2.x + v1.y * v2.y;
}

Int2 int2Perp(Int2 v) {
   return Int2{ -v.y, v.x };
}

Int2 int2Subtract(Int2 v1, Int2 v2) {
   return Int2{ v1.x - v2.x, v1.y - v2.y };
}

i32 pointOnLine(Int2 l1, Int2 l2, Int2 point) {
   return int2Dot(int2Perp(int2Subtract(l2, l1)), int2Subtract(point, l1));
}

bool lineSegmentIntersectsAABBi(Int2 l1, Int2 l2, Recti *rect) {
   i32 topleft, topright, bottomright, bottomleft;

   if (l1.x > rect->x + rect->w && l2.x > rect->x + rect->w) { return false; }
   if (l1.x < rect->x && l2.x < rect->x) { return false; }
   if (l1.y > rect->y + rect->h && l2.y > rect->y + rect->h) { return false; }
   if (l1.y < rect->y && l2.y < rect->y) { return false; }

   topleft = pointOnLine(l1, l2, Int2{ rect->x, rect->y });
   topright = pointOnLine(l1, l2, Int2{ rect->x + rect->w, rect->y });
   bottomright = pointOnLine(l1, l2, Int2{ rect->x + rect->w, rect->y + rect->h });
   bottomleft = pointOnLine(l1, l2, Int2{ rect->x, rect->y + rect->h });

   if (topleft > 0 && topright > 0 && bottomright > 0 && bottomleft > 0) {
      return false;
   }

   if (topleft < 0 && topright < 0 && bottomright < 0 && bottomleft < 0) {
      return false;
   }

   return true;

}
Recti getProportionallyFitRect(Int2 srcSize, Int2 destSize) {
   return getProportionallyFitRect(
      Float2{ (f32)srcSize.x, (f32)srcSize.y }, 
      Float2{ (f32)destSize.x, (f32)destSize.y });
}
Recti getProportionallyFitRect(Float2 srcSize, Float2 destSize) {
   float rw = (float)destSize.x;
   float rh = (float)destSize.y;
   float cw = (float)srcSize.x;
   float ch = (float)srcSize.y;

   float ratio = MIN(rw / cw, rh / ch);

   Recti out = { 0, 0, (i32)(cw * ratio), (i32)(ch * ratio) };
   out.x += (i32)((rw - out.w) / 2.0f);
   out.y += (i32)((rh - out.h) / 2.0f);

   return out;
}