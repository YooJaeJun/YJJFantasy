"""preview_*.png를 한 장으로 모아 본다: python contact_sheet.py <폴더> <출력.png> [이름 필터...]"""
import glob
import os
import sys

from PIL import Image, ImageDraw

folder, out = sys.argv[1], sys.argv[2]
filters = sys.argv[3:]
files = sorted(f for f in glob.glob(os.path.join(folder, "preview_*.png")) if not filters or any(k in os.path.basename(f) for k in filters))
if not files:
    sys.exit("no previews")
w, h = Image.open(files[0]).size
cols = 6
rows = (len(files) + cols - 1) // cols
sheet = Image.new("RGB", (cols * w, rows * h), (40, 40, 40))
draw = ImageDraw.Draw(sheet)
for i, f in enumerate(files):
    x, y = (i % cols) * w, (i // cols) * h
    sheet.paste(Image.open(f).convert("RGB"), (x, y))
    draw.text((x + 6, y + 6), os.path.basename(f)[8:-4], fill=(0, 0, 0))
sheet.save(out)
print(out, len(files))
