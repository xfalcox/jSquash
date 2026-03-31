export interface AVIFAnimFrame {
  imageData: ImageData;
  duration: number;
}

export interface AVIFModule extends EmscriptenWasm.Module {
  decode(data: BufferSource, bitDepth: 10 | 12 | 16): { data: Uint16Array, height: number, width: number } | null;
  decode(data: BufferSource, bitDepth: 8): ImageData | null;
  decode(data: BufferSource, bitDepth: 8 | 10 | 12 | 16): { data: Uint16Array, height: number, width: number } | ImageData | null;
  decodeAnimated(data: BufferSource): AVIFAnimFrame[] | null;
  isAnimated(data: BufferSource): boolean;
}

declare var moduleFactory: EmscriptenWasm.ModuleFactory<AVIFModule>;

export default moduleFactory;
