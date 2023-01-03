#ifndef GFX2D_PAINTER_H
#define GFX2D_PAINTER_H

#include "types.h"

#include "Bitmap.h"

class Painter {
public:
    Painter(Bitmap& bitmap, Color paint_color):
        bitmap{bitmap},
		paint_color{paint_color}
	{}

	void fill_pixel(i32 x, i32 y) {
        assert(x >= 0 && x < bitmap.width);
        assert(y >= 0 && y < bitmap.height);

	    bitmap.data[y * bitmap.width + x] = paint_color;
	}

	Bitmap& bitmap;
	Color paint_color;
};

#endif // GFX2D_PAINTER_H
