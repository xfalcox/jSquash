/**
 * Copyright 2020 Google Inc. All Rights Reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * Notice: I (Jamie Sinclair) have modified this file to accept an ArrayBuffer instead of typed array
 * and manually allow instantiation of the Wasm Module.
 */
import type { WebPFrame } from './codec/dec/webp_dec.js';
export type { WebPFrame };
export declare function init(moduleOptionOverrides?: Partial<EmscriptenWasm.ModuleOpts>): Promise<void>;
export default function decode(buffer: ArrayBuffer): Promise<ImageData>;
export declare function decodeAnimated(buffer: ArrayBuffer): Promise<WebPFrame[]>;
export declare function isAnimated(buffer: ArrayBuffer): Promise<boolean>;
//# sourceMappingURL=decode.d.ts.map