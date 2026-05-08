# Submission kits for CaptionFlow

Templates for each public distribution channel.

## 1. OBS Forum resource page

Use only after a separate policy review confirms the resource should be resubmitted. The full, rendered description lives in [`docs/forum-description.md`](forum-description.md). Run `node scripts/obs-forum-submit.mjs` to print the fields for manual copy/paste into the forum form.

Upload order recommended:

1. Hero screenshot (filter properties panel with captions flowing)
2. Demo GIF / MP4 — we already keep one at `docs/screenshots/hero.gif`
3. Model-download dialog screenshot
4. Sensitive-word mute demo (audio waveform before/after)

Tags to set: `captions`, `transcription`, `accessibility`, `subtitles`, `sherpa-onnx`, `chinese`.

## 2. OBS Discord #plugin-dev

Use Discord only to ask for policy clarification before resubmission, not as a replacement distribution channel. Draft:

```
Hi, I'd like to ask for a policy check before resubmitting CaptionFlow.

CaptionFlow is an independent GPL-2.0-or-later plugin for OBS Studio that runs sherpa-onnx speech recognition locally and writes captions to a text file source. The resource title and repository are now CaptionFlow / XWHQSJ/captionflow, and the description includes third-party and LLM-assistance disclosures.

Source: https://github.com/XWHQSJ/captionflow
Release: https://github.com/XWHQSJ/captionflow/releases/tag/0.1.0

Is there anything else in the name, description, license, or packaging that would conflict with the Forum Resource and IP Policy?
```

## 3. Announcement channels post-launch

After the Forum listing is live:

- [ ] Post release notes on [r/obs](https://reddit.com/r/obs)
- [ ] Tweet / Mastodon with the hero GIF
- [ ] Open a "made with sherpa-onnx" issue in
      [k2-fsa/sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx) asking to be
      listed
- [ ] Submit to [awesome-obs](https://github.com/awesome-foss/awesome-obs)
