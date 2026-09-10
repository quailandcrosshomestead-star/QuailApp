# 🧑‍🎨 Person Outfit & Location Generator

Give it **one clear photo of a face** and it produces several images of that **same
person** in different **outfits** and **locations** — for free, with nothing to
install on your own computer.

It runs on **Google Colab's free cloud GPU**, so you don't need a powerful computer
or graphics card. Everything is open source ([InstantID](https://github.com/InstantID/InstantID)
keeps the face consistent across new clothes and backgrounds).

## ▶️ Run it (one click)

[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/quailandcrosshomestead-star/QuailApp/blob/claude/person-outfit-image-generator-jjtwoc/person_image_generator.ipynb)

Then:

1. In Colab, click **Runtime ▸ Change runtime type** and choose a **GPU** (the free **T4** is perfect). Save.
2. **Runtime ▸ Run all** (or press ▶ on each cell, top to bottom). The first run takes about 5–8 minutes while it installs and downloads the open-source models.
3. The last cell prints a public **`https://…gradio.live`** link. Open it, upload a face photo, pick how many images you want, and click **✨ Generate**.

That's it. Uploading a photo and generating happens in a friendly web page — no code needed.

## What you can control

- **Number of images** — 1 to 8 per run.
- **Subject** — `person` / `woman` / `man`, which nudges the wording of the prompts.
- **Identity strength** (advanced) — higher keeps the face closer to your photo.
- **Quality steps** (advanced) — higher looks better but is slower.
- **Seed** (advanced) — set a fixed number to reproduce a result, or `-1` for random.

The outfits and locations are picked automatically ("surprise me"), mixing a wardrobe
and a set of backdrops so each run is different. Want your own? Edit the `OUTFITS` and
`LOCATIONS` lists in **cell 5** of the notebook and re-run that cell.

## Is it really free?

Yes — Google Colab gives free GPU time, and all the models are open source and free to
download. The only catches are Colab's usage limits and that a session eventually times
out; when it does, just re-run the cells to get a fresh (still free) link.

## Tips & troubleshooting

- **Best results:** a clear, well-lit, front-facing photo where the face isn't tiny.
- **"No GPU found":** Runtime ▸ Change runtime type ▸ **T4 GPU** ▸ Save, then run again.
- **Out of memory:** lower the number of images, or switch `pipe.cuda()` to
  `pipe.enable_model_cpu_offload()` in cell 4 (slower, less memory).
- **The link stopped working:** it expires with the Colab session — re-run the last cell.

## Prefer top quality / a permanent app later?

This free route trades some quality and speed for $0 cost. If you ever want the highest
quality and an always-on hosted app (no per-run setup), that path exists too but uses
paid image-generation credits — ask and we can build it.
