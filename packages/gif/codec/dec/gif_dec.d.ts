export interface GIFFrame {
  imageData: ImageData;
  duration: number;
}

export interface GIFModule extends EmscriptenWasm.Module {
  decode(data: BufferSource): ImageData | null;
  decodeAnimated(data: BufferSource): GIFFrame[] | null;
  isAnimated(data: BufferSource): boolean;
}

declare var moduleFactory: EmscriptenWasm.ModuleFactory<GIFModule>;

export default moduleFactory;
