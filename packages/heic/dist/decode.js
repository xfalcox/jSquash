import { initEmscriptenModule } from './utils.js';
import heic_dec from './codec/dec/heic_dec.js';
let emscriptenModule;
export async function init(module, moduleOptionOverrides) {
    let actualModule = module;
    let actualOptions = moduleOptionOverrides;
    // If only one argument is provided and it's not a WebAssembly.Module
    if (arguments.length === 1 && !(module instanceof WebAssembly.Module)) {
        actualModule = undefined;
        actualOptions = module;
    }
    emscriptenModule = initEmscriptenModule(heic_dec, actualModule, actualOptions);
}
export default async function decode(buffer) {
    if (!emscriptenModule) {
        init();
    }
    const module = await emscriptenModule;
    const result = module.decode(buffer);
    if (!result)
        throw new Error('Decoding error');
    return result;
}
