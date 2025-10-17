# ToK1 Output node params (PROPOSED)
```json
{
  "type": "ToK1",
  "params": {
    "target": "k1-001.local",
    "geometry": "K1_LGP_v1",
    "color": { "gamma": 2.2, "brightnessLimit": 0.85, "whiteBalance": [1.0,0.98,1.04] },
    "publish": { "mode": "clip", "fps": 120, "seconds": 20 }
  }
}
```
- Set mode to `"program"` to publish IR v0.1 instead of a baked clip.
