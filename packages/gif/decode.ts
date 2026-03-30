import type { GIFModule, GIFFrame } from './codec/dec/gif_dec.js';
import { initEmscriptenModule } from './utils.js';

import gif_dec from './codec/dec/gif_dec.js';

export type { GIFFrame };

function validateGif(buffer: ArrayBuffer): void {
  const header = new Uint8Array(buffer, 0, 6);
  const sig = String.fromCharCode(...header);
  if (sig !== 'GIF87a' && sig !== 'GIF89a') {
    throw new Error(
      `Not a valid GIF file (expected GIF87a/GIF89a header, got "${sig.replace(/[^\x20-\x7E]/g, '?')}")`,
    );
  }
}

let emscriptenModule: Promise<GIFModule>;

export async function init(
  moduleOptionOverrides?: Partial<EmscriptenWasm.ModuleOpts>,
): Promise<void>;
export async function init(
  module?: WebAssembly.Module,
  moduleOptionOverrides?: Partial<EmscriptenWasm.ModuleOpts>,
): Promise<void> {
  let actualModule: WebAssembly.Module | undefined = module;
  let actualOptions: Partial<EmscriptenWasm.ModuleOpts> | undefined =
    moduleOptionOverrides;

  // If only one argument is provided and it's not a WebAssembly.Module
  if (arguments.length === 1 && !(module instanceof WebAssembly.Module)) {
    actualModule = undefined;
    actualOptions = module as unknown as Partial<EmscriptenWasm.ModuleOpts>;
  }

  emscriptenModule = initEmscriptenModule(
    gif_dec,
    actualModule,
    actualOptions,
  );
}

export default async function decode(
  buffer: ArrayBuffer,
): Promise<ImageData> {
  validateGif(buffer);
  if (!emscriptenModule) {
    init();
  }

  const module = await emscriptenModule;
  const result = module.decode(buffer);
  if (!result) throw new Error('Decoding error');
  return result;
}

export async function decodeAnimated(
  buffer: ArrayBuffer,
): Promise<GIFFrame[]> {
  validateGif(buffer);
  if (!emscriptenModule) {
    init();
  }

  const module = await emscriptenModule;
  const result = module.decodeAnimated(buffer);
  if (!result) throw new Error('Decoding error');
  return result;
}

export async function isAnimated(
  buffer: ArrayBuffer,
): Promise<boolean> {
  validateGif(buffer);
  if (!emscriptenModule) {
    init();
  }

  const module = await emscriptenModule;
  return module.isAnimated(buffer);
}
