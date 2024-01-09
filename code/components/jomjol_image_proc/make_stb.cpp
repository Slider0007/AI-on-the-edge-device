#include "../../include/defines.h"

#include <stdint.h>
#include <string>

#include "psram.h"


#define STBI_MALLOC(sz)           malloc_psram_heap_STBI("STBI", sz, MALLOC_CAP_SPIRAM)
#define STBI_REALLOC(p,newsz)     realloc_psram_heap("STBI", p, newsz, MALLOC_CAP_SPIRAM)
#define STBI_FREE(p)              free_psram_heap("STBI", p)

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize.h"