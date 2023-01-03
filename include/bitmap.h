#ifndef GFX2D_BITMAP_H
#define GFX2D_BITMAP_H

#include "types.h"

#include <stb_image_write.h>

class Bitmap {
public:
    Bitmap(i32 width, i32 height):
        width{width},
		height{height},
		data{new Color[width * height]}
	{}

	~Bitmap() {
        delete data;
	}

	void serialize_to_bmp(std::string filename) {
        stbi_write_bmp(filename.c_str(), width, height, 4, data);
	}


	i32 width, height;
    Color* data;
};

#endif // GFX2D_BITMAP_H
