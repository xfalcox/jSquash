#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <cstdlib>
#include <cstring>
#include "gif_lib.h"

using namespace emscripten;

thread_local const val Uint8ClampedArray = val::global("Uint8ClampedArray");
thread_local const val ImageData = val::global("ImageData");

struct MemoryReader {
  const uint8_t* data;
  size_t size;
  size_t pos;
};

int readFromMemory(GifFileType* gif, GifByteType* buf, int len) {
  MemoryReader* reader = static_cast<MemoryReader*>(gif->UserData);
  size_t remaining = reader->size - reader->pos;
  size_t to_read = (size_t)len < remaining ? (size_t)len : remaining;
  memcpy(buf, reader->data + reader->pos, to_read);
  reader->pos += to_read;
  return (int)to_read;
}

val decode(std::string gifimage) {
  MemoryReader reader;
  reader.data = reinterpret_cast<const uint8_t*>(gifimage.c_str());
  reader.size = gifimage.length();
  reader.pos = 0;

  int error = 0;
  GifFileType* gif = DGifOpen(&reader, readFromMemory, &error);
  if (!gif) {
    return val::null();
  }

  if (DGifSlurp(gif) != GIF_OK) {
    DGifCloseFile(gif, &error);
    return val::null();
  }

  if (gif->ImageCount < 1) {
    DGifCloseFile(gif, &error);
    return val::null();
  }

  int width = gif->SWidth;
  int height = gif->SHeight;
  size_t total_bytes = (size_t)width * height * 4;

  // Allocate RGBA buffer and fill with background color
  uint8_t* rgba = static_cast<uint8_t*>(malloc(total_bytes));
  if (!rgba) {
    DGifCloseFile(gif, &error);
    return val::null();
  }

  // Initialize with background color
  ColorMapObject* colorMap = gif->SColorMap;
  if (colorMap && gif->SBackGroundColor < colorMap->ColorCount) {
    GifColorType bg = colorMap->Colors[gif->SBackGroundColor];
    for (size_t i = 0; i < total_bytes; i += 4) {
      rgba[i]     = bg.Red;
      rgba[i + 1] = bg.Green;
      rgba[i + 2] = bg.Blue;
      rgba[i + 3] = 255;
    }
  } else {
    memset(rgba, 0, total_bytes);
  }

  // Render first frame
  SavedImage* frame = &gif->SavedImages[0];
  GifImageDesc* desc = &frame->ImageDesc;
  ColorMapObject* frameColorMap = desc->ColorMap ? desc->ColorMap : gif->SColorMap;

  if (!frameColorMap) {
    free(rgba);
    DGifCloseFile(gif, &error);
    return val::null();
  }

  // Check for transparency via Graphics Control Extension
  int transparentIndex = -1;
  for (int i = 0; i < frame->ExtensionBlockCount; i++) {
    ExtensionBlock* eb = &frame->ExtensionBlocks[i];
    if (eb->Function == GRAPHICS_EXT_FUNC_CODE && eb->ByteCount >= 4) {
      if (eb->Bytes[0] & 0x01) {
        transparentIndex = (unsigned char)eb->Bytes[3];
      }
    }
  }

  for (int y = 0; y < desc->Height; y++) {
    int destY = desc->Top + y;
    if (destY < 0 || destY >= height) continue;
    for (int x = 0; x < desc->Width; x++) {
      int destX = desc->Left + x;
      if (destX < 0 || destX >= width) continue;

      int colorIndex = frame->RasterBits[y * desc->Width + x];

      if (colorIndex == transparentIndex) continue;

      if (colorIndex >= frameColorMap->ColorCount) continue;

      GifColorType color = frameColorMap->Colors[colorIndex];
      size_t pixelOffset = ((size_t)destY * width + destX) * 4;
      rgba[pixelOffset]     = color.Red;
      rgba[pixelOffset + 1] = color.Green;
      rgba[pixelOffset + 2] = color.Blue;
      rgba[pixelOffset + 3] = 255;
    }
  }

  val result = ImageData.new_(
      Uint8ClampedArray.new_(typed_memory_view(total_bytes, rgba)),
      width, height);

  free(rgba);
  DGifCloseFile(gif, &error);

  return result;
}

EMSCRIPTEN_BINDINGS(my_module) {
  function("decode", &decode);
}
