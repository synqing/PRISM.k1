export class LEDPreview {
  constructor(canvas) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.currentFrame = 0;
    this.playing = false;
  }

  // Render LEDs with temporal delays
  renderFrame(ch1Colors, delayMap, frameTimeMs) {
    this.ctx.fillStyle = '#000';
    this.ctx.fillRect(0, 0, this.canvas.width, this.canvas.height);

    const ledWidth = this.canvas.width / 160;
    const ledHeight = 20;

    // CH1 (bottom edge)
    for (let i = 0; i < 160; i++) {
      const x = i * ledWidth;
      this.ctx.fillStyle = ch1Colors[i];
      this.ctx.fillRect(x, this.canvas.height - ledHeight, ledWidth - 2, ledHeight);
    }

    // CH2 (top edge) with delay applied
    for (let i = 0; i < 160; i++) {
      const delay = delayMap[i];
      const active = frameTimeMs >= delay;
      const color = active ? ch1Colors[i] : '#000';

      const x = i * ledWidth;
      this.ctx.fillStyle = color;
      this.ctx.fillRect(x, 0, ledWidth - 2, ledHeight);
    }

    // Draw frame time indicator
    this.ctx.fillStyle = '#fff';
    this.ctx.font = '12px monospace';
    this.ctx.fillText(`Frame: ${this.currentFrame} | Time: ${frameTimeMs}ms`, 10, 40);
  }

  // Animate preview
  play(ch1Colors, delayMap, fps = 60) {
    this.playing = true;
    const framePeriodMs = 1000 / fps;
    let lastTime = performance.now();

    const animate = (currentTime) => {
      if (!this.playing) return;

      const deltaTime = currentTime - lastTime;
      if (deltaTime >= framePeriodMs) {
        this.currentFrame++;
        const frameTimeMs = this.currentFrame * framePeriodMs;
        this.renderFrame(ch1Colors, delayMap, frameTimeMs);
        lastTime = currentTime;
      }

      requestAnimationFrame(animate);
    };

    requestAnimationFrame(animate);
  }

  stop() {
    this.playing = false;
    this.currentFrame = 0;
  }
}

