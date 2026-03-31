import gifInit, {
  decodeAnimated as gifDecodeAnimated,
  decode as gifDecode,
  isAnimated as gifIsAnimated,
} from '../../packages/gif/codec/pkg/squoosh_gif.js';

import avifEncInit from '../../packages/avif/codec/enc/avif_enc.js';

const dropZone = document.getElementById('drop-zone');
const fileInput = dropZone.querySelector('input[type="file"]');
const statusEl = document.getElementById('status');
const optionsEl = document.getElementById('options');
const convertBtn = document.getElementById('convert-btn');
const comparison = document.getElementById('comparison');
const gifPreview = document.getElementById('gif-preview');
const avifPreview = document.getElementById('avif-preview');
const gifMeta = document.getElementById('gif-meta');
const avifMeta = document.getElementById('avif-meta');
const downloadBtn = document.getElementById('download-btn');
const qualityInput = document.getElementById('quality');
const qualityValue = document.getElementById('quality-value');
const speedInput = document.getElementById('speed');
const speedValue = document.getElementById('speed-value');

let gifBuffer = null;
let gifFileName = '';
let gifInitialized = false;
let avifModule = null;

function setStatus(msg, type = 'info') {
  statusEl.style.display = 'block';
  statusEl.textContent = msg;
  statusEl.className = type === 'error' ? 'error' : type === 'success' ? 'success' : '';
}

function formatBytes(bytes) {
  if (bytes < 1024) return bytes + ' B';
  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
  return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
}

qualityInput.addEventListener('input', () => { qualityValue.textContent = qualityInput.value; });
speedInput.addEventListener('input', () => { speedValue.textContent = speedInput.value; });

async function initGif() {
  if (!gifInitialized) {
    await gifInit();
    gifInitialized = true;
  }
}

async function initAvifEnc() {
  if (!avifModule) {
    avifModule = await avifEncInit({ noInitialRun: true });
  }
  return avifModule;
}

async function handleFile(file) {
  if (!file) return;

  gifBuffer = await file.arrayBuffer();
  gifFileName = file.name.replace(/\.gif$/i, '');
  comparison.style.display = 'none';
  downloadBtn.style.display = 'none';

  const gifUrl = URL.createObjectURL(file);
  gifPreview.src = gifUrl;

  setStatus('Initializing GIF decoder...');
  await initGif();

  const data = new Uint8Array(gifBuffer);
  const animated = gifIsAnimated(data);
  const frames = animated ? gifDecodeAnimated(data) : null;
  const frameCount = frames ? frames.length : 1;

  gifMeta.textContent = `${formatBytes(gifBuffer.byteLength)} \u2014 ${frameCount} frame${frameCount > 1 ? 's' : ''}`;

  optionsEl.style.display = 'block';
  convertBtn.disabled = false;
  setStatus(`GIF loaded: ${frameCount} frame${frameCount > 1 ? 's' : ''}. Adjust options and click Convert.`);
}

convertBtn.addEventListener('click', async () => {
  if (!gifBuffer) return;
  convertBtn.disabled = true;

  try {
    setStatus('Initializing AVIF encoder...');
    const module = await initAvifEnc();

    setStatus('Decoding GIF frames...');
    const data = new Uint8Array(gifBuffer);
    const animated = gifIsAnimated(data);

    const quality = parseInt(qualityInput.value);
    const speed = parseInt(speedInput.value);

    const encodeOptions = {
      quality,
      qualityAlpha: -1,
      denoiseLevel: 0,
      tileColsLog2: 0,
      tileRowsLog2: 0,
      speed,
      subsample: 1,
      chromaDeltaQ: false,
      sharpness: 0,
      tune: 0, // auto
      enableSharpYUV: false,
      bitDepth: 8,
    };

    let result;
    const t0 = performance.now();

    if (animated) {
      const frames = gifDecodeAnimated(data);
      setStatus(`Encoding ${frames.length} frames to animated AVIF (this may take a while)...`);
      // Yield to UI before heavy encode
      await new Promise((r) => setTimeout(r, 50));
      result = module.encodeAnimated(frames, encodeOptions);
    } else {
      const imageData = gifDecode(data);
      setStatus('Encoding to AVIF...');
      result = module.encode(imageData.data, imageData.width, imageData.height, encodeOptions);
    }

    const elapsed = (performance.now() - t0).toFixed(0);

    if (!result) throw new Error('AVIF encoding failed');

    const avifBlob = new Blob([result], { type: 'image/avif' });
    const avifUrl = URL.createObjectURL(avifBlob);
    avifPreview.src = avifUrl;

    const ratio = ((1 - result.byteLength / gifBuffer.byteLength) * 100).toFixed(1);
    const sign = ratio >= 0 ? 'smaller' : 'larger';
    avifMeta.textContent = `${formatBytes(result.byteLength)} \u2014 ${Math.abs(ratio)}% ${sign} \u2014 encoded in ${elapsed}ms`;

    downloadBtn.href = avifUrl;
    downloadBtn.download = `${gifFileName}.avif`;
    downloadBtn.style.display = 'inline-block';

    comparison.style.display = '';
    setStatus(`Done! Converted in ${elapsed}ms.`, 'success');
  } catch (err) {
    setStatus(`Error: ${err.message}`, 'error');
    console.error(err);
  } finally {
    convertBtn.disabled = false;
  }
});

// File input
dropZone.addEventListener('click', () => fileInput.click());
fileInput.addEventListener('change', (e) => handleFile(e.target.files[0]));

// Drag and drop
dropZone.addEventListener('dragover', (e) => { e.preventDefault(); dropZone.classList.add('dragover'); });
dropZone.addEventListener('dragleave', () => dropZone.classList.remove('dragover'));
dropZone.addEventListener('drop', (e) => { e.preventDefault(); dropZone.classList.remove('dragover'); handleFile(e.dataTransfer.files[0]); });
