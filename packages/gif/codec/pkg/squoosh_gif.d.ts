/* tslint:disable */
/* eslint-disable */

export class GIFFrame {
    private constructor();
    free(): void;
    [Symbol.dispose](): void;
    readonly duration: number;
    readonly imageData: ImageData;
}

export function decode(data: Uint8Array): ImageData;

export function decodeAnimated(data: Uint8Array): GIFFrame[];

export function isAnimated(data: Uint8Array): boolean;

export type InitInput = RequestInfo | URL | Response | BufferSource | WebAssembly.Module;

export interface InitOutput {
    readonly memory: WebAssembly.Memory;
    readonly __wbg_gifframe_free: (a: number, b: number) => void;
    readonly gifframe_imageData: (a: number) => any;
    readonly gifframe_duration: (a: number) => number;
    readonly decode: (a: number, b: number) => any;
    readonly decodeAnimated: (a: number, b: number) => [number, number];
    readonly isAnimated: (a: number, b: number) => number;
    readonly __wbindgen_free: (a: number, b: number, c: number) => void;
    readonly __wbindgen_externrefs: WebAssembly.Table;
    readonly __wbindgen_malloc: (a: number, b: number) => number;
    readonly __externref_drop_slice: (a: number, b: number) => void;
    readonly __wbindgen_start: () => void;
}

export type SyncInitInput = BufferSource | WebAssembly.Module;

/**
 * Instantiates the given `module`, which can either be bytes or
 * a precompiled `WebAssembly.Module`.
 *
 * @param {{ module: SyncInitInput }} module - Passing `SyncInitInput` directly is deprecated.
 *
 * @returns {InitOutput}
 */
export function initSync(module: { module: SyncInitInput } | SyncInitInput): InitOutput;

/**
 * If `module_or_path` is {RequestInfo} or {URL}, makes a request and
 * for everything else, calls `WebAssembly.instantiate` directly.
 *
 * @param {{ module_or_path: InitInput | Promise<InitInput> }} module_or_path - Passing `InitInput` directly is deprecated.
 *
 * @returns {Promise<InitOutput>}
 */
export default function __wbg_init (module_or_path?: { module_or_path: InitInput | Promise<InitInput> } | InitInput | Promise<InitInput>): Promise<InitOutput>;
