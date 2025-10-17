# Geometry Profile schema (PROPOSED)
```json
{
  "schema": 1,
  "id": "K1_LGP_v1",
  "faceplate": "StdClear_2mm_v1",
  "revision": "r1.2",
  "signature": "sha256-hex",
  "gamma": 2.2,
  "whiteBalance": [1.00, 0.98, 1.04],
  "maxBrightness": 0.85,
  "pixels": [
    { "u": 0.123, "v": 0.456, "edgeW0": 0.62, "edgeW1": 0.38, "gain": 1.05 }
  ]
}
```
Notes:
- (u,v) are normalized 0..1 coordinates of the visible plate.
- edgeW0/edgeW1 are energy weights from each edge injector.
- gain is a per-pixel uniformity correction derived from calibration.
