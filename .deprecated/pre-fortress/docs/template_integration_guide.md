# PRISM Template Integration Guide
*Practical examples for integrating with the template system*

## Quick Start

This guide shows how to integrate with PRISM K1's template system from:
- **Mobile/Web Apps** - Browse and deploy templates
- **Timeline Editor** - Import templates as pattern sources
- **Rust Compiler** - Embed template references in .prism files
- **Firmware** - Implement new templates

---

## 1. Mobile/Web App Integration

### 1.1 List Available Templates

**JavaScript/TypeScript Example**:

```typescript
import { WebSocket } from 'ws';

interface Template {
    id: string;
    numericId: number;
    name: string;
    category: string;
    description: string;
    parameters: Parameter[];
    tags: string[];
}

async function listTemplates(ws: WebSocket): Promise<Template[]> {
    // Build MSG_LIST_TEMPLATES (0x40)
    const message = Buffer.from([
        0x40,           // Type: LIST_TEMPLATES
        0x00, 0x00,     // Length: 0 (no payload)
        // CRC32 calculated and appended by sendMessage()
    ]);

    const response = await sendMessage(ws, message);

    // Parse response
    const count = response.readUInt8(0);
    const templates: Template[] = [];

    let offset = 1;
    for (let i = 0; i < count; i++) {
        const id = response.readUInt8(offset);
        offset += 1;

        const name = response.toString('utf8', offset, offset + 32).replace(/\0.*$/g, '');
        offset += 32;

        const category = response.toString('utf8', offset, offset + 16).replace(/\0.*$/g, '');
        offset += 16;

        const metadataLen = response.readUInt16LE(offset);
        offset += 2;

        const metadataJson = response.toString('utf8', offset, offset + metadataLen);
        offset += metadataLen;

        const metadata = JSON.parse(metadataJson);

        templates.push({
            id: `pattern_${id.toString().padStart(3, '0')}`,
            numericId: id,
            name: name,
            category: category,
            description: metadata.description,
            parameters: metadata.parameters,
            tags: metadata.tags || []
        });
    }

    return templates;
}

// Usage
const ws = new WebSocket('ws://prism-k1.local/ws');
ws.on('open', async () => {
    const templates = await listTemplates(ws);
    console.log(`Found ${templates.length} templates:`);
    templates.forEach(t => {
        console.log(`  [${t.numericId}] ${t.name} (${t.category})`);
    });
});
```

### 1.2 Deploy Template to Storage

```typescript
interface DeployOptions {
    templateId: number;          // 1-15
    saveAs?: string;             // Optional custom name
    parameters?: {[key: string]: number};  // Parameter overrides
}

async function deployTemplate(ws: WebSocket, options: DeployOptions): Promise<string> {
    // Build parameter override list
    const paramEntries = Object.entries(options.parameters || {});

    // Calculate payload size
    const payloadSize = 1 + 32 + 1 + paramEntries.length * (16 + 4);

    // Build message
    const buffer = Buffer.alloc(3 + payloadSize + 4);
    let offset = 0;

    // Header
    buffer.writeUInt8(0x41, offset++);                      // Type: DEPLOY_TEMPLATE
    buffer.writeUInt16LE(payloadSize, offset); offset += 2; // Length

    // Payload
    buffer.writeUInt8(options.templateId, offset++);        // Template ID

    // Save-as name (null-terminated, padded to 32 bytes)
    const saveName = options.saveAs || '';
    buffer.write(saveName, offset, 32, 'utf8');
    offset += 32;

    // Parameter count
    buffer.writeUInt8(paramEntries.length, offset++);

    // Parameters
    for (const [name, value] of paramEntries) {
        buffer.write(name, offset, 16, 'utf8');
        offset += 16;
        buffer.writeFloatLE(value, offset);
        offset += 4;
    }

    // CRC32 (calculated by helper function)
    const crc = calculateCRC32(buffer.slice(0, offset));
    buffer.writeUInt32LE(crc, offset);

    // Send and await response
    const response = await sendMessage(ws, buffer);

    // Parse response
    const status = response.readUInt8(0);
    if (status !== 0x00) {
        throw new Error(`Deploy failed with status: 0x${status.toString(16)}`);
    }

    const filePath = response.toString('utf8', 1, 129).replace(/\0.*$/g, '');
    const fileSize = response.readUInt32LE(129);

    console.log(`Deployed to ${filePath} (${fileSize} bytes)`);
    return filePath;
}

// Usage
const filePath = await deployTemplate(ws, {
    templateId: 1,  // Ocean Wave
    saveAs: 'My Calm Ocean',
    parameters: {
        speed: 0.5,
        color_shift: 240,  // Blue instead of cyan
        wave_count: 5
    }
});
```

### 1.3 Preview Template (Live Render)

```typescript
async function previewTemplate(
    ws: WebSocket,
    templateId: number,
    parameters?: {[key: string]: number},
    durationMs: number = 0  // 0 = until stopped
): Promise<void> {
    const paramEntries = Object.entries(parameters || {});
    const payloadSize = 1 + 4 + 1 + paramEntries.length * (16 + 4);

    const buffer = Buffer.alloc(3 + payloadSize + 4);
    let offset = 0;

    // Header
    buffer.writeUInt8(0x42, offset++);                      // Type: PREVIEW_TEMPLATE
    buffer.writeUInt16LE(payloadSize, offset); offset += 2;

    // Payload
    buffer.writeUInt8(templateId, offset++);
    buffer.writeUInt32LE(durationMs, offset); offset += 4;
    buffer.writeUInt8(paramEntries.length, offset++);

    for (const [name, value] of paramEntries) {
        buffer.write(name, offset, 16, 'utf8');
        offset += 16;
        buffer.writeFloatLE(value, offset);
        offset += 4;
    }

    const crc = calculateCRC32(buffer.slice(0, offset));
    buffer.writeUInt32LE(crc, offset);

    const response = await sendMessage(ws, buffer);

    const status = response.readUInt8(0);
    if (status !== 0x00) {
        throw new Error(`Preview failed with status: 0x${status.toString(16)}`);
    }

    console.log(`Previewing template ${templateId}`);
}

// Usage: Live preview with custom parameters
await previewTemplate(ws, 1, {
    speed: 1.5,
    intensity: 0.9,
    color_shift: 180
}, 10000);  // Preview for 10 seconds

// Stop preview
await stopPreview(ws);
```

### 1.4 Complete React Component Example

```tsx
import React, { useState, useEffect } from 'react';
import { useWebSocket } from './hooks/useWebSocket';

interface TemplateGalleryProps {
    deviceUrl: string;
}

export const TemplateGallery: React.FC<TemplateGalleryProps> = ({ deviceUrl }) => {
    const [templates, setTemplates] = useState<Template[]>([]);
    const [selectedCategory, setSelectedCategory] = useState<string>('all');
    const [previewingId, setPreviewingId] = useState<number | null>(null);

    const ws = useWebSocket(deviceUrl);

    useEffect(() => {
        if (ws) {
            loadTemplates();
        }
    }, [ws]);

    const loadTemplates = async () => {
        const templates = await listTemplates(ws);
        setTemplates(templates);
    };

    const handlePreview = async (templateId: number) => {
        setPreviewingId(templateId);
        await previewTemplate(ws, templateId);
    };

    const handleDeploy = async (templateId: number) => {
        await deployTemplate(ws, { templateId });
        alert('Template deployed!');
    };

    const filteredTemplates = selectedCategory === 'all'
        ? templates
        : templates.filter(t => t.category === selectedCategory);

    return (
        <div className="template-gallery">
            <div className="category-tabs">
                <button onClick={() => setSelectedCategory('all')}>All</button>
                <button onClick={() => setSelectedCategory('ambient')}>Ambient</button>
                <button onClick={() => setSelectedCategory('energy')}>Energy</button>
                <button onClick={() => setSelectedCategory('artistic')}>Artistic</button>
            </div>

            <div className="template-grid">
                {filteredTemplates.map(template => (
                    <div key={template.id} className="template-card">
                        <div className="template-thumbnail">
                            {/* Render thumbnail from metadata.thumbnailData */}
                            <img src={template.thumbnailData} alt={template.name} />
                        </div>
                        <h3>{template.name}</h3>
                        <p className="category">{template.category}</p>
                        <p className="description">{template.description}</p>
                        <div className="tags">
                            {template.tags.map(tag => (
                                <span key={tag} className="tag">{tag}</span>
                            ))}
                        </div>
                        <div className="actions">
                            <button
                                onClick={() => handlePreview(template.numericId)}
                                disabled={previewingId === template.numericId}
                            >
                                {previewingId === template.numericId ? 'Previewing...' : 'Preview'}
                            </button>
                            <button onClick={() => handleDeploy(template.numericId)}>
                                Deploy
                            </button>
                        </div>
                    </div>
                ))}
            </div>
        </div>
    );
};
```

---

## 2. Timeline Editor Integration

### 2.1 Import Template as Effect Source

**Web Timeline Editor Example**:

```typescript
interface TimelineSegment {
    startMs: number;
    durationMs: number;
    effectType: 'template' | 'custom';
    effectData: TemplateReference | CustomEffect;
}

interface TemplateReference {
    templateId: string;          // "pattern_001"
    parameters: {[key: string]: number};
    blendMode: 'replace' | 'add' | 'multiply';
}

class TimelineEditor {
    private segments: TimelineSegment[] = [];

    async importTemplate(templateId: string): Promise<void> {
        // Fetch template metadata to get parameter schema
        const metadata = await this.fetchTemplateMetadata(templateId);

        // Show parameter editor UI
        const parameters = await this.showParameterEditor(metadata.parameters);

        // Add to timeline
        this.addSegment({
            startMs: this.currentTimeMs,
            durationMs: 5000,  // Default 5 seconds
            effectType: 'template',
            effectData: {
                templateId: templateId,
                parameters: parameters,
                blendMode: 'replace'
            }
        });
    }

    private async fetchTemplateMetadata(templateId: string): Promise<any> {
        // Option 1: From device via WebSocket
        const ws = this.getDeviceWebSocket();
        const templates = await listTemplates(ws);
        return templates.find(t => t.id === templateId);

        // Option 2: From embedded JSON (offline editing)
        // return await fetch(`/templates/${templateId}.json`).then(r => r.json());
    }

    exportToPrism(): Blob {
        // Convert timeline to .prism binary format
        const prismFile = new PrismBuilder();

        for (const segment of this.segments) {
            if (segment.effectType === 'template') {
                const ref = segment.effectData as TemplateReference;

                // Embed template reference in .prism
                prismFile.addTemplateSegment({
                    startMs: segment.startMs,
                    durationMs: segment.durationMs,
                    templateId: ref.templateId,
                    parameters: ref.parameters,
                    blendMode: ref.blendMode
                });
            }
        }

        return prismFile.build();
    }
}

// Usage
const editor = new TimelineEditor();
await editor.importTemplate('pattern_001');  // Ocean Wave
editor.segments[0].effectData.parameters.speed = 1.5;  // Adjust speed
const prismBlob = editor.exportToPrism();
```

### 2.2 Live Preview During Editing

```typescript
class TemplateParameterEditor {
    private ws: WebSocket;
    private templateId: number;
    private previewTimeout: NodeJS.Timeout | null = null;

    constructor(ws: WebSocket, templateId: number) {
        this.ws = ws;
        this.templateId = templateId;
    }

    async onParameterChange(name: string, value: number): Promise<void> {
        // Debounce: Update preview 200ms after last parameter change
        if (this.previewTimeout) {
            clearTimeout(this.previewTimeout);
        }

        this.previewTimeout = setTimeout(async () => {
            const parameters = this.getAllParameters();
            await previewTemplate(this.ws, this.templateId, parameters);
        }, 200);
    }

    private getAllParameters(): {[key: string]: number} {
        // Gather all parameter values from UI controls
        return {
            speed: this.speedSlider.value,
            intensity: this.intensitySlider.value,
            color_shift: this.colorPicker.hue,
            // ... other parameters
        };
    }
}
```

---

## 3. Rust Compiler Integration

### 3.1 Embed Template References in .prism Files

**Rust Compiler Example**:

```rust
// compiler/src/template.rs

use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct TemplateReference {
    pub template_id: String,        // "pattern_001"
    pub numeric_id: u8,              // 1
    pub parameters: HashMap<String, f32>,
}

#[derive(Debug)]
pub struct Pattern {
    segments: Vec<Segment>,
}

#[derive(Debug)]
pub enum Segment {
    Template(TemplateSegment),
    Custom(CustomSegment),
}

#[derive(Debug)]
pub struct TemplateSegment {
    start_ms: u32,
    duration_ms: u32,
    template: TemplateReference,
    blend_mode: BlendMode,
}

#[derive(Debug)]
pub enum BlendMode {
    Replace,
    Add,
    Multiply,
    Crossfade(u32),  // Crossfade duration in ms
}

impl Pattern {
    pub fn new() -> Self {
        Pattern { segments: Vec::new() }
    }

    pub fn add_template(
        mut self,
        start_ms: u32,
        duration_ms: u32,
        template_id: &str,
        parameters: HashMap<String, f32>,
        blend_mode: BlendMode,
    ) -> Self {
        let numeric_id = template_id
            .strip_prefix("pattern_")
            .and_then(|s| s.parse::<u8>().ok())
            .expect("Invalid template ID");

        self.segments.push(Segment::Template(TemplateSegment {
            start_ms,
            duration_ms,
            template: TemplateReference {
                template_id: template_id.to_string(),
                numeric_id,
                parameters,
            },
            blend_mode,
        }));

        self
    }

    pub fn compile_to_prism(&self, output_path: &str) -> Result<(), Box<dyn std::error::Error>> {
        let mut file = PrismFile::create(output_path)?;

        // Write header
        file.write_header(PrismHeader {
            magic: 0x5053524D,  // 'PRSM'
            version: 0x0100,
            flags: 0,
            // ... other fields
        })?;

        // Write segments
        for segment in &self.segments {
            match segment {
                Segment::Template(tmpl) => {
                    file.write_template_segment(tmpl)?;
                }
                Segment::Custom(custom) => {
                    file.write_custom_segment(custom)?;
                }
            }
        }

        file.finalize()?;
        Ok(())
    }
}

// Example usage
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let pattern = Pattern::new()
        // Start with Ocean Wave
        .add_template(
            0,
            5000,
            "pattern_001",
            [
                ("speed".into(), 1.5),
                ("color_shift".into(), 180.0),
                ("intensity".into(), 0.7),
            ].iter().cloned().collect(),
            BlendMode::Replace,
        )
        // Crossfade to Rave Pulse
        .add_template(
            5000,
            3000,
            "pattern_006",
            [
                ("speed".into(), 2.0),
                ("intensity".into(), 0.9),
            ].iter().cloned().collect(),
            BlendMode::Crossfade(1000),  // 1 second crossfade
        )
        // End with Aurora
        .add_template(
            8000,
            7000,
            "pattern_002",
            [
                ("speed".into(), 0.8),
                ("shimmer".into(), 0.4),
            ].iter().cloned().collect(),
            BlendMode::Crossfade(1500),
        );

    pattern.compile_to_prism("my_pattern.prism")?;
    println!("Pattern compiled successfully!");

    Ok(())
}
```

### 3.2 Validate Parameters Against Schema

```rust
// compiler/src/validation.rs

use serde_json::Value;
use std::fs;

pub struct TemplateValidator {
    schemas: HashMap<String, Value>,
}

impl TemplateValidator {
    pub fn load_from_directory(dir: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let mut schemas = HashMap::new();

        for entry in fs::read_dir(dir)? {
            let entry = entry?;
            let path = entry.path();

            if path.extension().and_then(|s| s.to_str()) == Some("json") {
                let template_id = path.file_stem()
                    .and_then(|s| s.to_str())
                    .unwrap()
                    .to_string();

                let schema: Value = serde_json::from_reader(fs::File::open(&path)?)?;
                schemas.insert(template_id, schema);
            }
        }

        Ok(TemplateValidator { schemas })
    }

    pub fn validate_parameters(
        &self,
        template_id: &str,
        parameters: &HashMap<String, f32>,
    ) -> Result<(), ValidationError> {
        let schema = self.schemas.get(template_id)
            .ok_or_else(|| ValidationError::UnknownTemplate(template_id.to_string()))?;

        let params_def = schema["parameters"].as_array()
            .ok_or(ValidationError::InvalidSchema)?;

        for (name, value) in parameters {
            let param_schema = params_def.iter()
                .find(|p| p["name"].as_str() == Some(name))
                .ok_or_else(|| ValidationError::UnknownParameter(name.clone()))?;

            match param_schema["type"].as_str().unwrap() {
                "float" => {
                    let min = param_schema["min"].as_f64().unwrap() as f32;
                    let max = param_schema["max"].as_f64().unwrap() as f32;

                    if *value < min || *value > max {
                        return Err(ValidationError::OutOfRange {
                            param: name.clone(),
                            value: *value,
                            min,
                            max,
                        });
                    }
                }
                "int" => {
                    let min = param_schema["min"].as_i64().unwrap() as f32;
                    let max = param_schema["max"].as_i64().unwrap() as f32;

                    if *value < min || *value > max {
                        return Err(ValidationError::OutOfRange {
                            param: name.clone(),
                            value: *value,
                            min,
                            max,
                        });
                    }
                }
                _ => {}
            }
        }

        Ok(())
    }
}

#[derive(Debug)]
pub enum ValidationError {
    UnknownTemplate(String),
    UnknownParameter(String),
    OutOfRange { param: String, value: f32, min: f32, max: f32 },
    InvalidSchema,
}
```

---

## 4. Firmware Implementation

### 4.1 Adding a New Template

**Step-by-step guide for firmware developers**:

#### Step 1: Define Template Specification

Edit `.taskmaster/docs/template_specifications.md`:

```markdown
#### **Pattern 16: Meteor Shower**
**ID:** `pattern_016`
**Category:** Energy
**Description:** Meteors streak across the strip with trailing tails
**Memory:** 9216 bytes compressed / 20480 bytes runtime
**CPU Load:** 18% average

**Parameters:**
| Name | Type | Range | Default | Description |
|------|------|-------|---------|-------------|
| speed | float | 0.1-5.0 | 1.5 | Meteor travel speed |
| frequency | float | 0.1-2.0 | 0.5 | Spawn frequency |
| tail_length | int | 5-50 | 20 | Tail length in LEDs |
| color_shift | float | 0-360 | 45 | Base hue (45=orange) |

**Algorithm:**
\```c
void render_meteor_shower(rgb_t* leds, uint32_t time_ms, const params_t* params) {
    // Clear background
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = RGB(0, 0, 10);  // Dark blue background
    }

    // Update and render active meteors
    for (int m = 0; m < active_meteor_count; m++) {
        meteor_t* meteor = &meteors[m];

        // Update position
        meteor->position += params->speed * 0.1;

        // Render tail
        for (int t = 0; t < params->tail_length; t++) {
            int led_pos = (int)(meteor->position - t);
            if (led_pos >= 0 && led_pos < NUM_LEDS) {
                float intensity = 1.0 - ((float)t / params->tail_length);
                intensity = intensity * intensity;  // Quadratic falloff

                HSV hsv = {
                    .h = params->color_shift + (t * 2),
                    .s = 255,
                    .v = intensity * 255
                };
                leds[led_pos] = hsv_to_rgb(hsv);
            }
        }

        // Remove if off screen
        if (meteor->position > NUM_LEDS + params->tail_length) {
            remove_meteor(m);
        }
    }

    // Spawn new meteors
    if (random_float() < params->frequency * 0.01) {
        spawn_meteor();
    }
}
\```
```

#### Step 2: Implement Render Function

Edit `firmware/components/templates/src/template_patterns.c`:

```c
// Add meteor state management
#define MAX_METEORS 10

typedef struct {
    float position;
    float velocity;
    uint8_t hue;
} meteor_t;

static meteor_t meteors[MAX_METEORS];
static size_t active_meteor_count = 0;

static void spawn_meteor(void) {
    if (active_meteor_count >= MAX_METEORS) return;

    meteor_t* m = &meteors[active_meteor_count++];
    m->position = 0;
    m->velocity = 1.0 + random_float() * 2.0;
    m->hue = random() % 60;  // Vary hue slightly
}

static void remove_meteor(size_t index) {
    if (index < active_meteor_count - 1) {
        meteors[index] = meteors[active_meteor_count - 1];
    }
    active_meteor_count--;
}

// Main render function
void render_meteor_shower(rgb_t* leds, uint32_t time_ms, const params_t* params) {
    // Implementation from specification above
    // ...
}
```

#### Step 3: Register Template

Edit `firmware/components/templates/src/template_registry.c` (or regenerate with script):

```c
const template_entry_t TEMPLATE_REGISTRY[16] = {
    // ... existing 15 templates ...

    {
        .id = 16,
        .name = "Meteor Shower",
        .category = "energy",
        .render = render_meteor_shower,
        .metadata_json = TEMPLATE_METADATA[15],  // JSON from metadata array
        .thumbnail_data = NULL,
        .thumbnail_size = 0
    }
};
```

#### Step 4: Add Metadata

Edit `firmware/components/templates/src/template_metadata.c` (or regenerate):

```c
const char* TEMPLATE_METADATA[16] = {
    // ... existing 15 ...

    // Pattern 016: Meteor Shower
    "{\"id\":\"pattern_016\",\"numericId\":16,\"name\":\"Meteor Shower\","
    "\"category\":\"energy\",\"description\":\"Meteors streak across the strip...\","
    "\"parameters\":[{\"name\":\"speed\",\"type\":\"float\",\"min\":0.1,"
    "\"max\":5.0,\"default\":1.5},{\"name\":\"frequency\",\"type\":\"float\","
    "\"min\":0.1,\"max\":2.0,\"default\":0.5},{\"name\":\"tail_length\","
    "\"type\":\"int\",\"min\":5,\"max\":50,\"default\":20},"
    "{\"name\":\"color_shift\",\"type\":\"color\",\"default\":45}],"
    "\"performance\":{\"minFps\":30,\"targetFps\":60,\"updatePeriodMs\":16,"
    "\"powerFactor\":0.60},\"memory\":{\"compressedBytes\":9216,"
    "\"runtimeBytes\":20480,\"cpuPercent\":18}}"
};
```

#### Step 5: Build and Test

```bash
# Rebuild firmware
cd firmware
idf.py build

# Flash to device
idf.py flash monitor

# Test via WebSocket
python3 scripts/test_template.py --template-id 16
```

### 4.2 Template Performance Profiling

```c
// Add to firmware/components/templates/test/test_performance.c

TEST_CASE("Template 16 performance", "[template][perf]") {
    rgb_t leds[LED_COUNT];
    params_t params = {
        .speed = 1.5,
        .frequency = 0.5,
        .tail_length = 20,
        .color_shift = 45
    };

    // Warm-up
    for (int i = 0; i < 10; i++) {
        render_meteor_shower(leds, i * 16, &params);
    }

    // Measure 100 frames
    uint32_t start = esp_timer_get_time();
    for (int i = 0; i < 100; i++) {
        render_meteor_shower(leds, i * 16, &params);
    }
    uint32_t elapsed = esp_timer_get_time() - start;

    float avg_frame_us = elapsed / 100.0f;
    ESP_LOGI(TAG, "Meteor Shower: %.2f us/frame (%.1f FPS)",
             avg_frame_us, 1000000.0f / avg_frame_us);

    // Must meet 60 FPS target
    TEST_ASSERT_LESS_THAN(16667, avg_frame_us);
}
```

---

## 5. Testing

### 5.1 End-to-End Integration Test

```typescript
// tests/integration/template_system.test.ts

import { describe, it, expect, beforeAll, afterAll } from '@jest/globals';
import WebSocket from 'ws';

describe('Template System Integration', () => {
    let ws: WebSocket;

    beforeAll(async () => {
        ws = new WebSocket('ws://prism-k1-test.local/ws');
        await new Promise(resolve => ws.on('open', resolve));
    });

    afterAll(() => {
        ws.close();
    });

    it('should list all 15 templates', async () => {
        const templates = await listTemplates(ws);
        expect(templates).toHaveLength(15);

        // Verify IDs are sequential
        for (let i = 0; i < 15; i++) {
            expect(templates[i].numericId).toBe(i + 1);
        }

        // Verify categories
        const categories = new Set(templates.map(t => t.category));
        expect(categories).toContain('ambient');
        expect(categories).toContain('energy');
        expect(categories).toContain('artistic');
    });

    it('should preview template without deploying', async () => {
        await previewTemplate(ws, 1, { speed: 1.0 });

        // Wait for preview to start
        await new Promise(resolve => setTimeout(resolve, 500));

        // Stop preview
        await stopPreview(ws);

        // Verify no file was created
        const deployed = await listDeployedTemplates(ws);
        expect(deployed.find(t => t.numericId === 1)).toBeUndefined();
    });

    it('should deploy template with custom parameters', async () => {
        const filePath = await deployTemplate(ws, {
            templateId: 1,
            saveAs: 'Test Ocean',
            parameters: {
                speed: 0.5,
                color_shift: 200
            }
        });

        expect(filePath).toBe('/littlefs/templates/pattern_001.prism');

        // Verify file exists
        const deployed = await listDeployedTemplates(ws);
        expect(deployed.find(t => t.numericId === 1)).toBeDefined();
    });

    it('should load deployed template for playback', async () => {
        // Deploy first
        await deployTemplate(ws, { templateId: 1 });

        // Load for playback
        await loadPattern(ws, '/littlefs/templates/pattern_001.prism');

        // Verify pattern is active
        const status = await getDeviceStatus(ws);
        expect(status.currentPattern).toBe('pattern_001');
    });

    it('should handle parameter validation', async () => {
        // Try to deploy with out-of-range parameter
        await expect(
            deployTemplate(ws, {
                templateId: 1,
                parameters: {
                    speed: 10.0  // Max is 5.0
                }
            })
        ).rejects.toThrow();
    });

    it('should support all 15 templates', async () => {
        for (let id = 1; id <= 15; id++) {
            const filePath = await deployTemplate(ws, { templateId: id });
            expect(filePath).toContain(`pattern_${id.toString().padStart(3, '0')}.prism`);
        }

        const deployed = await listDeployedTemplates(ws);
        expect(deployed).toHaveLength(15);
    });
});
```

---

## 6. Troubleshooting

### Common Issues

**Issue 1: Template not appearing in list**
```
Symptom: MSG_LIST_TEMPLATES returns < 15 templates
Cause: Template registry corrupted or not compiled
Fix:
1. Check template_registry.c has all 15 entries
2. Verify template_id is sequential (1-15)
3. Rebuild firmware: idf.py clean && idf.py build
```

**Issue 2: Deploy fails with 0x02 (storage full)**
```
Symptom: MSG_DEPLOY_TEMPLATE returns status 0x02
Cause: LittleFS partition full
Fix:
1. Delete unused patterns via WebSocket MSG_DELETE_PATTERN message
2. Check free space on device
3. Increase partition size in partitions.csv (requires reflash)
```

**Issue 3: Preview shows incorrect pattern**
```
Symptom: MSG_PREVIEW_TEMPLATE renders wrong pattern
Cause: Function pointer array out of sync
Fix:
1. Verify template_registry.c order matches IDs
2. Check for duplicate render function assignments
3. Regenerate registry: python scripts/generate_registry.py
```

**Issue 4: Parameters don't affect pattern**
```
Symptom: Changing parameters has no visual effect
Cause: Render function not reading params struct
Fix:
1. Check render function signature: void render_*(rgb_t*, uint32_t, const params_t*)
2. Verify params are accessed correctly: params->speed, not params.speed
3. Add logging: ESP_LOGI(TAG, "speed=%f", params->speed);
```

---

## Appendix: Helper Functions

### WebSocket Binary Protocol Helpers

```typescript
// helpers/websocket.ts

import crc32 from 'crc-32';

export function calculateCRC32(buffer: Buffer): number {
    return crc32.buf(buffer) >>> 0;  // Convert to unsigned
}

export async function sendMessage(ws: WebSocket, message: Buffer): Promise<Buffer> {
    return new Promise((resolve, reject) => {
        const timeout = setTimeout(() => {
            reject(new Error('WebSocket timeout'));
        }, 5000);

        ws.once('message', (data: Buffer) => {
            clearTimeout(timeout);
            resolve(data);
        });

        ws.send(message);
    });
}

export function buildBinaryMessage(
    type: number,
    payload: Buffer = Buffer.alloc(0)
): Buffer {
    const buffer = Buffer.alloc(3 + payload.length + 4);
    let offset = 0;

    // Type (1 byte)
    buffer.writeUInt8(type, offset++);

    // Length (2 bytes, little-endian)
    buffer.writeUInt16LE(payload.length, offset);
    offset += 2;

    // Payload
    payload.copy(buffer, offset);
    offset += payload.length;

    // CRC32 (4 bytes)
    const crc = calculateCRC32(buffer.slice(0, offset));
    buffer.writeUInt32LE(crc, offset);

    return buffer;
}
```

---

*End of Template Integration Guide*
*Last Updated: 2025-10-15*
