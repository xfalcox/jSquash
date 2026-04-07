import initGifModule, { decode as gifDecodeWasm, decodeAnimated as gifDecodeAnimatedWasm, isAnimated as gifIsAnimatedWasm, } from './codec/pkg/squoosh_gif.js';
let gifModule;
function validateGif(buffer) {
    const header = new Uint8Array(buffer, 0, 6);
    const sig = String.fromCharCode(...header);
    if (sig !== 'GIF87a' && sig !== 'GIF89a') {
        throw new Error(`Not a valid GIF file (expected GIF87a/GIF89a header, got "${sig.replace(/[^\x20-\x7E]/g, '?')}")`);
    }
}
export async function init(moduleOrPath) {
    if (!gifModule) {
        gifModule = initGifModule(moduleOrPath);
    }
    return gifModule;
}
export default async function decode(buffer) {
    validateGif(buffer);
    await init();
    const result = gifDecodeWasm(new Uint8Array(buffer));
    if (!result)
        throw new Error('Decoding error');
    return result;
}
export async function decodeAnimated(buffer) {
    validateGif(buffer);
    await init();
    const result = gifDecodeAnimatedWasm(new Uint8Array(buffer));
    if (!result)
        throw new Error('Decoding error');
    return result;
}
export async function isAnimated(buffer) {
    validateGif(buffer);
    await init();
    return gifIsAnimatedWasm(new Uint8Array(buffer));
}
