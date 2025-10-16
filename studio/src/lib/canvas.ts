export type DprCanvas = {
  canvas: HTMLCanvasElement;
  ctx: CanvasRenderingContext2D;
  dpr: number;
};

export function setupDprCanvas(canvas: HTMLCanvasElement): DprCanvas {
  const dpr = (typeof window !== 'undefined' && window.devicePixelRatio) || 1;
  const ctx = canvas.getContext('2d');
  if (!ctx) throw new Error('2D context unavailable');
  const rect = canvas.getBoundingClientRect();
  canvas.width = Math.max(1, Math.floor(rect.width * dpr));
  canvas.height = Math.max(1, Math.floor(rect.height * dpr));
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  return { canvas, ctx, dpr };
}

export function resizeIfNeeded(dc: DprCanvas) {
  const rect = dc.canvas.getBoundingClientRect();
  const targetW = Math.max(1, Math.floor(rect.width * dc.dpr));
  const targetH = Math.max(1, Math.floor(rect.height * dc.dpr));
  if (dc.canvas.width !== targetW || dc.canvas.height !== targetH) {
    dc.canvas.width = targetW;
    dc.canvas.height = targetH;
    dc.ctx.setTransform(dc.dpr, 0, 0, dc.dpr, 0, 0);
    return true;
  }
  return false;
}

