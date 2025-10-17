// Bezier curve evaluation for delay map generation
export class CurveEditor {
  constructor(canvas) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.controlPoints = [];
    this.curveType = 'bezier'; // 'bezier' or 'catmull-rom'
  }

  // Add control point
  addPoint(x, y) {
    this.controlPoints.push({ x, y });
    this.redraw();
  }

  // Bezier curve calculation (De Casteljau)
  evaluateBezier(t, points) {
    if (points.length < 2) return { x: 0, y: 0 };
    if (points.length === 1) return points[0];

    const newPoints = [];
    for (let i = 0; i < points.length - 1; i++) {
      const x = (1 - t) * points[i].x + t * points[i + 1].x;
      const y = (1 - t) * points[i].y + t * points[i + 1].y;
      newPoints.push({ x, y });
    }
    return this.evaluateBezier(t, newPoints);
  }

  // Sample curve to 160 LED delay values
  sampleToDelayMap(minDelay, maxDelay) {
    const delayMap = new Uint16Array(160);

    for (let i = 0; i < 160; i++) {
      const t = i / 159.0;
      const point = this.evaluateBezier(t, this.controlPoints);

      // Map y-coordinate to delay range
      const normalizedY = point.y / this.canvas.height;
      const delay = minDelay + (maxDelay - minDelay) * normalizedY;
      delayMap[i] = Math.round(Math.max(minDelay, Math.min(maxDelay, delay)));
    }

    return delayMap;
  }

  validateDelayMap(delayMap) {
    const errors = [];
    if (delayMap.length !== 160) {
      errors.push(`Invalid length: ${delayMap.length}, expected 160`);
    }
    for (let i = 0; i < delayMap.length; i++) {
      if (delayMap[i] > 65535) {
        errors.push(`LED ${i}: delay ${delayMap[i]} exceeds 65535ms`);
      }
    }
    const totalBytes = delayMap.length * 2;
    if (totalBytes !== 320) {
      errors.push(`Total size ${totalBytes} bytes, expected 320`);
    }
    return errors;
  }

  // Render curve to canvas
  redraw() {
    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);

    // Draw grid
    this.ctx.strokeStyle = '#333';
    this.ctx.lineWidth = 1;
    for (let i = 0; i <= 160; i += 20) {
      const x = (i / 160) * this.canvas.width;
      this.ctx.beginPath();
      this.ctx.moveTo(x, 0);
      this.ctx.lineTo(x, this.canvas.height);
      this.ctx.stroke();
    }

    // Draw curve
    this.ctx.strokeStyle = '#0f0';
    this.ctx.lineWidth = 2;
    this.ctx.beginPath();
    for (let i = 0; i <= 160; i++) {
      const t = i / 160.0;
      const point = this.evaluateBezier(t, this.controlPoints);
      if (i === 0) this.ctx.moveTo(point.x, point.y);
      else this.ctx.lineTo(point.x, point.y);
    }
    this.ctx.stroke();

    // Draw control points
    this.ctx.fillStyle = '#f00';
    this.controlPoints.forEach(p => {
      this.ctx.beginPath();
      this.ctx.arc(p.x, p.y, 5, 0, 2 * Math.PI);
      this.ctx.fill();
    });
  }
}

