/**
 * @file prism_config.h
 * @brief PRISM K1 Configuration Constants
 *
 * AUTO-GENERATED FROM CANON.md
 * DO NOT EDIT MANUALLY - Changes will be overwritten
 *
 * Generated: 2025-10-15 23:25:17 UTC
 * Source: CANON.md (Architecture Decision Records)
 */

#ifndef PRISM_CONFIG_H
#define PRISM_CONFIG_H

/*
 * WebSocket Configuration
 * Source: ADR-002 (WebSocket Buffer Size)
 */
#define WS_BUFFER_SIZE      4096    /**< WebSocket frame buffer size (bytes) */
#define WS_MAX_CLIENTS      2       /**< Maximum concurrent WebSocket connections */
#define WS_TIMEOUT_MS       5000    /**< WebSocket frame timeout (milliseconds) */

/*
 * LED Configuration
 * Source: ADR-003 (LED Count Standardization)
 */
#define LED_COUNT           320     /**< Number of addressable LEDs */
#define LED_TYPE_WS2812B    1        /**< LED type: WS2812B */
#define LED_FPS_TARGET      120      /**< Target frame rate (frames per second) */

/*
 * Pattern Configuration
 * Source: ADR-004 (Pattern Maximum Size)
 */
#define PATTERN_MAX_SIZE    262144  /**< Maximum pattern file size (bytes) - 256KB */
#define PATTERN_MIN_COUNT   15      /**< Minimum patterns that must fit in storage */
#define STORAGE_RESERVED    102400  /**< Reserved storage space (bytes) - 100KB safety margin */

/*
 * Storage Configuration
 * Source: ADR-005 (Storage Mount Path)
 */
#define STORAGE_MOUNT_PATH  "/littlefs"    /**< LittleFS VFS mount point */
#define STORAGE_TYPE        "littlefs"      /**< Filesystem type */
#define STORAGE_LABEL       "littlefs"      /**< Partition label */

#endif /* PRISM_CONFIG_H */
