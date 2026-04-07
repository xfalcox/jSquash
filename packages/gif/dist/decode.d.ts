import type { GIFFrame, InitInput, InitOutput as GifModule } from './codec/pkg/squoosh_gif.js';
export type { GIFFrame };
export declare function init(moduleOrPath?: InitInput): Promise<GifModule>;
export default function decode(buffer: ArrayBuffer): Promise<ImageData>;
export declare function decodeAnimated(buffer: ArrayBuffer): Promise<GIFFrame[]>;
export declare function isAnimated(buffer: ArrayBuffer): Promise<boolean>;
//# sourceMappingURL=decode.d.ts.map