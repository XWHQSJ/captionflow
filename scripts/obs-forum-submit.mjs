#!/usr/bin/env node
import { readFileSync } from 'fs';
import { fileURLToPath } from 'url';
import { dirname, resolve } from 'path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const repoRoot = resolve(__dirname, '..');
const forumDesc = readFileSync(resolve(repoRoot, 'docs/forum-description.md'), 'utf8');

const fields = [
  ['Title', 'CaptionFlow'],
  ['Tag line', 'Local bilingual captions powered by sherpa-onnx'],
  ['Version', '0.1.0'],
  ['External URL', 'https://github.com/XWHQSJ/captionflow/releases/tag/0.1.0'],
  ['Tags', 'captions, transcription, accessibility, subtitles, sherpa-onnx, chinese'],
  ['Description', forumDesc],
];

for (const [label, value] of fields) {
  console.log(`\n## ${label}\n${value}`);
}
