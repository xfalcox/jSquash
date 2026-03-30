#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <cstdlib>
#include <cstring>
#include "gif_lib.h"

using namespace emscripten;

thread_local const val Uint8ClampedArray = val::global("Uint8ClampedArray");
thread_local const val ImageData = val::global("ImageData");
thread_local const val Array = val::global("Array");
thread_local const val Object = val::global("Object");

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

GifFileType* openGif(MemoryReader* reader, int* error) {
  GifFileType* gif = DGifOpen(reader, readFromMemory, error);
  if (!gif) return nullptr;
  if (DGifSlurp(gif) != GIF_OK) {
    DGifCloseFile(gif, error);
    return nullptr;
  }
  return gif;
}

// Get frame disposal method and delay from Graphics Control Extension
struct FrameInfo {
  int transparentIndex;
  int disposalMethod;
  int delay; // in centiseconds (1/100th of a second)
};

FrameInfo getFrameInfo(SavedImage* frame) {
  FrameInfo info;
  info.transparentIndex = -1;
  info.disposalMethod = 0;
  info.delay = 0;

  for (int i = 0; i < frame->ExtensionBlockCount; i++) {
    ExtensionBlock* eb = &frame->ExtensionBlocks[i];
    if (eb->Function == GRAPHICS_EXT_FUNC_CODE && eb->ByteCount >= 4) {
      info.disposalMethod = (eb->Bytes[0] >> 2) & 0x07;
      info.delay = (unsigned char)eb->Bytes[1] | ((unsigned char)eb->Bytes[2] << 8);
      if (eb->Bytes[0] & 0x01) {
        info.transparentIndex = (unsigned char)eb->Bytes[3];
      }
    }
  }
  return info;
}

void renderFrame(uint8_t* rgba, int canvasWidth, int canvasHeight,
                 SavedImage* frame, ColorMapObject* globalColorMap,
                 int transparentIndex) {
  GifImageDesc* desc = &frame->ImageDesc;
  ColorMapObject* colorMap = desc->ColorMap ? desc->ColorMap : globalColorMap;
  if (!colorMap) return;

  for (int y = 0; y < desc->Height; y++) {
    int destY = desc->Top + y;
    if (destY < 0 || destY >= canvasHeight) continue;
    for (int x = 0; x < desc->Width; x++) {
      int destX = desc->Left + x;
      if (destX < 0 || destX >= canvasWidth) continue;

      int colorIndex = frame->RasterBits[y * desc->Width + x];
      if (colorIndex == transparentIndex) continue;
      if (colorIndex >= colorMap->ColorCount) continue;

      GifColorType color = colorMap->Colors[colorIndex];
      size_t offset = ((size_t)destY * canvasWidth + destX) * 4;
      rgba[offset]     = color.Red;
      rgba[offset + 1] = color.Green;
      rgba[offset + 2] = color.Blue;
      rgba[offset + 3] = 255;
    }
  }
}

void initCanvas(uint8_t* rgba, int width, int height, GifFileType* gif) {
  size_t total = (size_t)width * height * 4;
  ColorMapObject* colorMap = gif->SColorMap;
  if (colorMap && gif->SBackGroundColor < colorMap->ColorCount) {
    GifColorType bg = colorMap->Colors[gif->SBackGroundColor];
    for (size_t i = 0; i < total; i += 4) {
      rgba[i]     = bg.Red;
      rgba[i + 1] = bg.Green;
      rgba[i + 2] = bg.Blue;
      rgba[i + 3] = 255;
    }
  } else {
    memset(rgba, 0, total);
  }
}

void clearFrameArea(uint8_t* rgba, int canvasWidth, int canvasHeight,
                    SavedImage* frame, GifFileType* gif) {
  GifImageDesc* desc = &frame->ImageDesc;
  ColorMapObject* colorMap = gif->SColorMap;
  for (int y = 0; y < desc->Height; y++) {
    int destY = desc->Top + y;
    if (destY < 0 || destY >= canvasHeight) continue;
    for (int x = 0; x < desc->Width; x++) {
      int destX = desc->Left + x;
      if (destX < 0 || destX >= canvasWidth) continue;
      size_t offset = ((size_t)destY * canvasWidth + destX) * 4;
      if (colorMap && gif->SBackGroundColor < colorMap->ColorCount) {
        GifColorType bg = colorMap->Colors[gif->SBackGroundColor];
        rgba[offset]     = bg.Red;
        rgba[offset + 1] = bg.Green;
        rgba[offset + 2] = bg.Blue;
        rgba[offset + 3] = 255;
      } else {
        rgba[offset]     = 0;
        rgba[offset + 1] = 0;
        rgba[offset + 2] = 0;
        rgba[offset + 3] = 0;
      }
    }
  }
}

val decode(std::string gifimage) {
  MemoryReader reader = {
    reinterpret_cast<const uint8_t*>(gifimage.c_str()),
    gifimage.length(), 0
  };

  int error = 0;
  GifFileType* gif = openGif(&reader, &error);
  if (!gif || gif->ImageCount < 1) return val::null();

  int width = gif->SWidth;
  int height = gif->SHeight;
  size_t total_bytes = (size_t)width * height * 4;

  uint8_t* rgba = static_cast<uint8_t*>(malloc(total_bytes));
  if (!rgba) { DGifCloseFile(gif, &error); return val::null(); }

  initCanvas(rgba, width, height, gif);

  FrameInfo info = getFrameInfo(&gif->SavedImages[0]);
  renderFrame(rgba, width, height, &gif->SavedImages[0],
              gif->SColorMap, info.transparentIndex);

  val result = ImageData.new_(
      Uint8ClampedArray.new_(typed_memory_view(total_bytes, rgba)),
      width, height);

  free(rgba);
  DGifCloseFile(gif, &error);
  return result;
}

val decodeAnimated(std::string gifimage) {
  MemoryReader reader = {
    reinterpret_cast<const uint8_t*>(gifimage.c_str()),
    gifimage.length(), 0
  };

  int error = 0;
  GifFileType* gif = openGif(&reader, &error);
  if (!gif || gif->ImageCount < 1) return val::null();

  int width = gif->SWidth;
  int height = gif->SHeight;
  size_t total_bytes = (size_t)width * height * 4;

  uint8_t* canvas = static_cast<uint8_t*>(malloc(total_bytes));
  uint8_t* prevCanvas = static_cast<uint8_t*>(malloc(total_bytes));
  if (!canvas || !prevCanvas) {
    free(canvas);
    free(prevCanvas);
    DGifCloseFile(gif, &error);
    return val::null();
  }

  initCanvas(canvas, width, height, gif);

  val frames = Array.new_();

  for (int i = 0; i < gif->ImageCount; i++) {
    SavedImage* frame = &gif->SavedImages[i];
    FrameInfo info = getFrameInfo(frame);

    // Save canvas state before rendering (for restore-to-previous disposal)
    memcpy(prevCanvas, canvas, total_bytes);

    renderFrame(canvas, width, height, frame, gif->SColorMap, info.transparentIndex);

    // Default delay of 100ms if not specified or zero
    int delayMs = info.delay > 0 ? info.delay * 10 : 100;

    val imageData = ImageData.new_(
        Uint8ClampedArray.new_(typed_memory_view(total_bytes, canvas)),
        width, height);

    val frameObj = Object.new_();
    frameObj.set("imageData", imageData);
    frameObj.set("duration", delayMs);

    frames.call<void>("push", frameObj);

    // Apply disposal method for next frame
    switch (info.disposalMethod) {
      case 2: // Restore to background
        clearFrameArea(canvas, width, height, frame, gif);
        break;
      case 3: // Restore to previous
        memcpy(canvas, prevCanvas, total_bytes);
        break;
      // 0, 1: no disposal / do not dispose — leave canvas as-is
    }
  }

  free(canvas);
  free(prevCanvas);
  DGifCloseFile(gif, &error);
  return frames;
}

bool isAnimated(std::string gifimage) {
  MemoryReader reader = {
    reinterpret_cast<const uint8_t*>(gifimage.c_str()),
    gifimage.length(), 0
  };

  int error = 0;
  GifFileType* gif = openGif(&reader, &error);
  if (!gif) return false;

  bool animated = gif->ImageCount > 1;
  DGifCloseFile(gif, &error);
  return animated;
}

EMSCRIPTEN_BINDINGS(my_module) {
  function("decode", &decode);
  function("decodeAnimated", &decodeAnimated);
  function("isAnimated", &isAnimated);
}
