# Submission kits for CaptionFlow

Templates for each public distribution channel.

## 1. OBS Forum resource page

Use only after a separate policy review confirms the resource should be resubmitted. The full, rendered description lives in [`docs/forum-description.md`](forum-description.md) and is pre-filled by `scripts/obs-forum-submit.mjs` only when `CAPTIONFLOW_ENABLE_OBS_FORUM_SUBMIT=1` is set.

Upload order recommended:

1. Hero screenshot (filter properties panel with captions flowing)
2. Demo GIF / MP4 — we already keep one at `docs/screenshots/hero.gif`
3. Model-download dialog screenshot
4. Sensitive-word mute demo (audio waveform before/after)

Tags to set: `captions`, `transcription`, `accessibility`, `subtitles`, `sherpa-onnx`, `chinese`.

## 2. OBS Discord #plugin-dev

A short announcement in [Discord #plugin-dev](https://discord.gg/obsproject)
reaches core maintainers directly and has **no account-age or 2FA
restriction**. The announcement template we use:

```
Hey folks — just shipped **CaptionFlow** v0.1.0, a GPL-2.0-or-later,
on-device streaming ASR captioning plugin for OBS Studio.

• Windows x64 + macOS universal builds (Sigstore attested)
• Bilingual 中文/English Zipformer from sherpa-onnx
• Adaptive-beep sensitive-word mute filter
• One-click model download from the filter properties
• Third-party plugin; not developed by, endorsed by, or affiliated with the OBS Project

https://github.com/XWHQSJ/captionflow
https://github.com/XWHQSJ/captionflow/releases/tag/0.1.0

Feedback and bug reports very welcome!
```

## 3. OBS Plugin Manager (upstream not ready yet)

OBS's official Plugin Manager is tracked in
[obsproject/rfcs#4](https://github.com/obsproject/rfcs/pull/4); the registry
repo does not exist as of 2026-04. **Do not try to submit there yet.** Revisit
once the RFC merges and `obsproject/obs-plugin-registry` (or equivalent) is
announced.

## 4. Announcement channels post-launch

After the Forum listing is live:

- [ ] Post release notes on [r/obs](https://reddit.com/r/obs)
- [ ] Tweet / Mastodon with the hero GIF
- [ ] Open a "made with sherpa-onnx" issue in
      [k2-fsa/sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx) asking to be
      listed
- [ ] Submit to [awesome-obs](https://github.com/awesome-foss/awesome-obs)
