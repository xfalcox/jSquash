#include <emscripten/bind.h>
#include <emscripten/val.h>
#include "avif/avif.h"

using namespace emscripten;

thread_local const val Uint8ClampedArray = val::global("Uint8ClampedArray");
thread_local const val Uint16Array = val::global("Uint16Array");
thread_local const val ImageData = val::global("ImageData");
thread_local const val Object = val::global("Object");
thread_local const val Array = val::global("Array");

val decode(std::string avifimage, uint32_t bitDepth = 8) {
  avifImage* image = avifImageCreateEmpty();
  avifDecoder* decoder = avifDecoderCreate();
  avifResult decodeResult =
      avifDecoderReadMemory(decoder, image, (uint8_t*)avifimage.c_str(), avifimage.length());

  // image is an independent copy of decoded data, decoder may be destroyed here
  avifDecoderDestroy(decoder);

  val result = val::null();
  if (decodeResult == AVIF_RESULT_OK) {
    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, image);

    rgb.depth = bitDepth;

    avifRGBImageAllocatePixels(&rgb);
    avifImageYUVToRGB(image, &rgb);

    if (bitDepth != 8) {
      const size_t pixelCount = rgb.width * rgb.height;
      const size_t channelCount = 4;
      const size_t totalElements = pixelCount * channelCount;

      auto pixelData = Uint16Array.new_(typed_memory_view(totalElements,
                                        reinterpret_cast<uint16_t*>(rgb.pixels)));

      auto pixelArray = pixelData.call<val>("slice");

      result = Object.new_();
      result.set("data", pixelArray);
      result.set("width", rgb.width);
      result.set("height", rgb.height);
    } else {
      result = ImageData.new_(
          Uint8ClampedArray.new_(typed_memory_view(rgb.rowBytes * rgb.height, rgb.pixels)),
          rgb.width,
          rgb.height);
    }

    // Now we can safely free the RGB pixels:
    avifRGBImageFreePixels(&rgb);
  }

  avifImageDestroy(image);
  return result;
}

bool isAnimated(std::string avifimage) {
  avifDecoder* decoder = avifDecoderCreate();
  avifResult setIOResult = avifDecoderSetIOMemory(
      decoder, (const uint8_t*)avifimage.c_str(), avifimage.length());
  if (setIOResult != AVIF_RESULT_OK) {
    avifDecoderDestroy(decoder);
    return false;
  }

  avifResult parseResult = avifDecoderParse(decoder);
  if (parseResult != AVIF_RESULT_OK) {
    avifDecoderDestroy(decoder);
    return false;
  }

  bool result = decoder->imageCount > 1;
  avifDecoderDestroy(decoder);
  return result;
}

val decodeAnimated(std::string avifimage) {
  avifDecoder* decoder = avifDecoderCreate();
  avifResult setIOResult = avifDecoderSetIOMemory(
      decoder, (const uint8_t*)avifimage.c_str(), avifimage.length());
  if (setIOResult != AVIF_RESULT_OK) {
    avifDecoderDestroy(decoder);
    return val::null();
  }

  avifResult parseResult = avifDecoderParse(decoder);
  if (parseResult != AVIF_RESULT_OK) {
    avifDecoderDestroy(decoder);
    return val::null();
  }

  val frames = Array.new_();

  while (avifDecoderNextImage(decoder) == AVIF_RESULT_OK) {
    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, decoder->image);
    rgb.depth = 8;
    avifRGBImageAllocatePixels(&rgb);
    avifImageYUVToRGB(decoder->image, &rgb);

    val imageData = ImageData.new_(
        Uint8ClampedArray.new_(typed_memory_view(rgb.rowBytes * rgb.height, rgb.pixels)),
        rgb.width, rgb.height);

    // Duration is in seconds, convert to milliseconds
    int durationMs = (int)(decoder->imageTiming.duration * 1000.0);

    val frameObj = Object.new_();
    frameObj.set("imageData", imageData);
    frameObj.set("duration", durationMs);
    frames.call<void>("push", frameObj);

    avifRGBImageFreePixels(&rgb);
  }

  avifDecoderDestroy(decoder);
  return frames;
}

EMSCRIPTEN_BINDINGS(my_module) {
  function("decode", &decode);
  function("decodeAnimated", &decodeAnimated);
  function("isAnimated", &isAnimated);
}