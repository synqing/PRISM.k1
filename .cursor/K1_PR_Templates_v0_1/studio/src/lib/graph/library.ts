// studio/src/lib/graph/library.ts (PR TEMPLATE)
// Minimal v1 node library definitions (UI & compiler will reference this)
import type { NodeDef } from "./types";

export const NODE_LIBRARY: Record<string, NodeDef> = {
  AngleField:   { type:"AngleField", inputs:{}, outputs:{scalar:"scalar"}, params:[] },
  RadiusField:  { type:"RadiusField", inputs:{}, outputs:{scalar:"scalar"}, params:[] },
  Phase:        { type:"Phase", inputs:{}, outputs:{phase:"scalar"}, params:[{name:"speed",kind:"scalar",default:0.15,min:-5,max:5}] },
  Sin:          { type:"Sin", inputs:{in:"scalar"}, outputs:{out:"scalar"}, params:[] },
  Add:          { type:"Add", inputs:{a:"scalar",b:"scalar"}, outputs:{out:"scalar"}, params:[] },
  AddScalar:    { type:"AddScalar", inputs:{in:"scalar"}, outputs:{out:"scalar"}, params:[{name:"k",kind:"scalar",default:0.0,min:-1,max:1}] },
  Multiply:     { type:"Multiply", inputs:{a:"scalar",b:"scalar"}, outputs:{out:"scalar"}, params:[{name:"k",kind:"scalar",default":1.0,min:0,max:10}] },
  Ring:         { type:"Ring", inputs:{radius:"scalar",phase:"scalar"}, outputs:{mask:"scalar"}, params:[{name:"width",kind:"scalar",default:0.03,min:0,max:0.5}] },
  HueShift:     { type:"HueShift", inputs:{in:"scalar"}, outputs:{out:"scalar"}, params:[{name:"shift",kind:"scalar",default:0.0,min:-1,max:1}] },
  PaletteMap:   { type:"PaletteMap", inputs:{idx:"scalar"}, outputs:{color:"color"}, params:[{name:"palette",kind:"int",default:0}] },
  Fade:         { type:"Fade", inputs:{color:"color"}, outputs:{color:"color"}, params:[{name:"amount",kind:"scalar",default:0.85,min:0,max:1}] },
  ToK1:         { type:"ToK1", inputs:{color:"color"}, outputs:{}, params:[] },
};
