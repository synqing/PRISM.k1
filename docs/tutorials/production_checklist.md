# Tutorial Series Production Checklist

Use this checklist to track progress from script finalization through publication. Update the status column as you complete each item.

| Step | Video 1 | Video 2 | Video 3 | Video 4 | Video 5 | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| Script locked | ✅ | ✅ | ✅ | ✅ | ✅ | See `docs/tutorials/scripts/*.md` |
| Slide deck finalized |  |  |  |  |  | Export 16:9, include branding |
| CLI demo assets generated |  |  |  |  |  | Ensure `hsluv` installed |
| Hardware footage captured |  |  |  |  |  | White balance locked |
| Narration recorded |  |  |  |  |  | Target -16 LUFS |
| Editing complete |  |  |  |  |  | Export 1080p30, 16 Mbps |
| QA review (audio/video) |  |  |  |  |  | Verify chapters, captions |
| Upload to YouTube |  |  |  |  |  | Title: `PRISM K1 v1.1 – <Topic>` |
| Description updated |  |  |  |  |  | Include commands, docs links |
| Chapters added |  |  |  |  |  | Align with script timings |
| Thumbnail created |  |  |  |  |  | Consistent branding |
| README updated with link |  |  |  |  |  | Add playlist and individual links |
| Release notes updated |  |  |  |  |  | Reference playlist + highlight |

## Publishing Notes
- Create a YouTube playlist titled **"PRISM K1 Firmware v1.1 Tutorials"** and add videos in order.
- Include the following links in each description:
  - Repository root
  - `docs/user-manual.md`
  - `docs/release/soak_test_runbook.md`
  - `docs/release/preset_library_overview.md`
  - `docs/release/v1.1_release_notes.md`
- Use consistent thumbnail styling (logo + video number + topic).
- After uploading, update README and release notes with playlist URL and individual links.

## QA Checklist Per Video
- [ ] Audio levels normalized, no clipping.
- [ ] Hardware footage matches CLI commands shown.
- [ ] Captions (auto or manual) reviewed for technical terms.
- [ ] Links in description verified.
- [ ] Cards/end screens configured to point to next video and soak test resources.

## Archive
- Store project files (OBS, DAW, editor) in `assets/tutorials/<video-number>/` or shared storage.
- Export compressed backups (e.g., ProRes master + MP4) for future updates.
- Document any deviations from scripts for future revisions.
