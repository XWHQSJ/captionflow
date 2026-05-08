# CaptionFlow

> Independent third-party plugin for OBS Studio that turns local audio into live captions on your machine.
>
> CaptionFlow is not developed by, endorsed by, or affiliated with the OBS Project.

[GitHub](https://github.com/XWHQSJ/captionflow) · [Latest release](https://github.com/XWHQSJ/captionflow/releases/latest) · [Report issue](https://github.com/XWHQSJ/captionflow/issues) · GPL-2.0-or-later

## Why this plugin?

CaptionFlow keeps speech recognition local during use: your microphone or desktop audio is decoded by sherpa-onnx on your machine, and captions are written to a text file that OBS Studio can read. The first model download contacts the upstream model host; after that, captioning works offline with the cached model.

The initial release focuses on low-latency English captions, a bilingual Chinese/English preset, and an optional delay-line mute filter for sensitive words.

## Features

- 🎯 **Real-time captions** — low-latency partial results while speech is still in progress, written atomically to a text file any Text (GDI+ / FreeType 2) source can read.
- 📥 **One-click model download** — pick English / bilingual / tiny preset in the filter properties; the plugin downloads and extracts on demand.
- 🤫 **Sensitive-word mute** — load a hotwords file (`word :boost`); the plugin delays output audio so it can retroactively beep out matches. Beep frequency and volume adapt to the speaker's F0 + RMS.
- ⚡ **Hardware providers** — CPU (default), CUDA (Windows + NVIDIA), DirectML (Windows + any GPU). CoreML coming in v0.2.
- 🔐 **Supply-chain verifiable** — every release ships with a Sigstore build provenance attestation, so you can verify the binary came out of our public CI.

## Supported platforms

- Windows 10 / 11 x64
- macOS 11+ universal (Apple Silicon + Intel)

Linux users: the code compiles cleanly, we just aren't shipping builds yet — contributions welcome.

## Installation

Download from the [GitHub release](https://github.com/XWHQSJ/captionflow/releases/latest):

- **Windows**: extract the ZIP, merge `obs-plugins\` and `data\obs-plugins\` into `%ProgramFiles%\obs-studio\`.
- **macOS**: open the `.pkg`; it installs into the user OBS Studio plugin directory.

The current packages are unsigned. Download them only from the GitHub release page and verify the Sigstore build provenance attestation before installing.

Verify the binary came out of our public CI run before installing:

```bash
gh attestation verify captionflow-0.1.0-macos-universal.pkg \
  --repo XWHQSJ/captionflow
```

## First use (60 seconds)

1. Right-click an audio source → **Filters** → **+** → **CaptionFlow**.
2. Click **Download Model…** and pick a preset.
3. Set **Caption Output File** to somewhere like `/tmp/captions.txt`.
4. Add a `Text (GDI+)` / `Text (FreeType 2)` source → enable **Read from file** → point at the same path.
5. Speak. Watch captions.

## Model presets

| Preset | Languages | Size |
| --- | --- | --- |
| English (20M, fast) | en | ~70 MB |
| Chinese + English (bilingual) | zh, en | ~300 MB |
| English (tiny) | en | ~40 MB |

## Links & credits

- **Source / issues**: https://github.com/XWHQSJ/captionflow
- **License**: GPL-2.0-or-later
- Built on:
  - [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx) — Next-gen Kaldi team
  - [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate)
  - [OBS Studio](https://github.com/obsproject/obs-studio) plugin SDK

Development note: LLM tools helped draft and revise parts of the code and documentation. The maintainer reviewed, edited, built, and tested the release before publication.
