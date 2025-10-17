export function mapUploadError(raw: string): string {
  const s = String(raw || '').toUpperCase();
  if (s.includes('WS_CONNECT_FAILED')) return 'Unable to connect to device. Check network and host name.';
  if (s.includes('WS_SEND_FAILED')) return 'Network error while uploading. Please retry. If it persists, check Wi‑Fi signal.';
  if (s.includes('WS_CLOSED')) return 'Connection closed unexpectedly. Retrying may help.';
  if (s.includes('TLV_TOO_SHORT') || s.includes('TLV_LENGTH_MISMATCH')) return 'Device replied with an invalid frame.';
  if (s.includes('TLV_CRC_INVALID')) return 'Device reported a CRC error in a frame. Retrying may help.';
  if (s.includes('HTTP_STATUS')) return 'Device returned an HTTP error status.';
  if (s.includes('INVALID_NAME')) return 'Pattern name is invalid. Use 1–63 ASCII characters.';
  if (s.includes('PATTERN_MAX_SIZE') || s.includes('TOO LARGE') || s.includes('> 256')) return 'Pattern exceeds device size limit (256KB). Reduce duration or complexity.';
  if (s.includes('DEVICE_BUSY')) return 'Device is busy. Try again after current operation completes.';
  if (s.includes('STATUS') && s.includes('MAXCHUNK')) return 'Device did not report a valid maxChunk. Using safe default may reduce throughput.';
  if (s.includes('TIMEOUT')) return 'Operation timed out. Check device power and network.';
  if (s.includes('CANCELLED')) return 'Upload cancelled.';
  return raw || 'Unknown error';
}
