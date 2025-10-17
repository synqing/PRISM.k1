import PrismExporter from '../wasm/prism_exporter.js';

export class PrismFileExporter {
  constructor() {
    this.wasmModule = null;
    this.ready = this.initWasm();
  }

  async initWasm() {
    this.wasmModule = await PrismExporter();
    console.log('WASM module loaded');
  }

  // Export CUSTOM mode pattern to .prism binary
  async exportCustomPattern(delayMap, metadata) {
    await this.ready;

    // Prepare parameters array
    const params = new Uint16Array([
      metadata.delay_ms || 0,
      metadata.progressive_start_ms || 0,
      metadata.progressive_end_ms || 0,
      metadata.wave_amplitude_ms || 0,
      metadata.wave_frequency_hz || 0,
      metadata.wave_phase_deg || 0
    ]);

    // Call WASM function
    const headerPtr = this.wasmModule.ccall(
      'create_prism_header',
      'number',
      ['number', 'number', 'number', 'number'],
      [
        metadata.motion_direction || 0,
        4, // CUSTOM mode = 4
        params,
        delayMap
      ]
    );

    // Copy data from WASM memory
    const totalSize = 16 + 320; // Header + delay map
    const buffer = new Uint8Array(
      this.wasmModule.HEAPU8.buffer,
      headerPtr,
      totalSize
    );

    // Create blob for download
    const blob = new Blob([buffer], { type: 'application/octet-stream' });
    return blob;
  }

  // Download .prism file
  downloadPattern(blob, filename) {
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    a.click();
    URL.revokeObjectURL(url);
  }
}

