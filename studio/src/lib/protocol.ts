// TLV Types
export const TLV_PUT_BEGIN = 0x10 as const;
export const TLV_PUT_DATA  = 0x11 as const;
export const TLV_PUT_END   = 0x12 as const;
export const TLV_CONTROL   = 0x20 as const;
export const TLV_STATUS    = 0x30 as const;

// CONTROL subcommands
export const CTRL_PLAY       = 0x01 as const;
export const CTRL_STOP       = 0x02 as const;
export const CTRL_BRIGHTNESS = 0x10 as const;
export const CTRL_GAMMA      = 0x11 as const;

// Status capabilities bits (if present)
export const CAP_COMPRESS = 1 << 0;
export const CAP_RESUME   = 1 << 1;
export const CAP_EVENTS   = 1 << 2;
export const CAP_PALETTE  = 1 << 3;
export const CAP_PROGRAM  = 1 << 4;
export const CAP_V09      = 1 << 5;
