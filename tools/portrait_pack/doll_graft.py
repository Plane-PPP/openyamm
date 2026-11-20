"""Build and review a custom portrait's head graft using the selected character row."""
import argparse
import csv
import io
import json
import os
import re
import shutil
import subprocess
import sys
import textwrap
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parents[2]
DOCS = REPO / "docs/portrait-pack"
LANCZOS = Image.Resampling.LANCZOS
HANDS = {"RHd": "Right Hand Open", "RHu": "Right Hand Hold", "RHb": "Right Hand Fingers",
         "LHd": "Left Hand Closed", "LHu": "Left Hand Hold", "LHo": "Left Hand Open"}
STATES = {"empty": ["RHd", "LHo"], "weapon-shield": ["RHu", "LHd", "RHb"], "two-handed": ["RHu", "LHu"]}


def git_file(path):
    proc = subprocess.run(["git", "cat-file", "-p", "HEAD:" + path], cwd=REPO, capture_output=True,
                          env={**os.environ, "GIT_NO_LAZY_FETCH": "1", "GIT_ALLOW_PROTOCOL": ""})
    if proc.returncode == 0:
        return proc.stdout
    return subprocess.check_output(["curl", "--http1.1", "-fsSL",
                                   "https://raw.githubusercontent.com/pjasicek/openyamm/master/" + path])


def table(name, key):
    data = git_file("assets_dev/engine/data_tables/" + name + ".txt").decode("utf-8-sig")
    return {int(row[key]): row for row in csv.DictReader(io.StringIO(data), delimiter="\t") if row[key].isdigit()}


def cache_icon(cache, stem):
    path = cache / (stem.lower() + ".png")
    if not path.exists():
        data = git_file("assets_dev/engine/icons_x2/" + path.name)
        with Image.open(io.BytesIO(data)) as img:
            img.verify()
        path.write_bytes(data)
    return path


def card_values(work):
    # The card uses a single mapping of scalar fields and one list. JSON is also valid YAML.
    contents = (work.parent / "card.yml").read_text()
    if contents.lstrip().startswith("{"):
        return json.loads(contents)
    values = {}
    for line in contents.splitlines():
        if line and not line.startswith((" ", "#", "-")):
            key, separator, value = line.partition(":")
            if separator:
                values[key] = value.strip().strip('"').strip("'")
    return values


def catalog():
    data = {}
    for line in (DOCS / "dolls.md").read_text().splitlines():
        fields = [s.strip() for s in line.strip("|").split("|")]
        if fields[0].isdigit():
            seam = re.match(r"\d+", fields[-1])
            data[int(fields[0])] = (int(seam[0]) if seam else None, fields[-2])
    return data


def fetch(row_id, rows, cache):
    row = rows[row_id]
    cache.mkdir(exist_ok=True, parents=True)
    for key in ["bod", "Background", *HANDS]:
        cache_icon(cache, row[key])
    for index, author in rows.items():
        if index >= 9000 and all(row[key].lower() == author[key].lower() for key in HANDS):
            return cache_icon(cache, author["bod"])
    return None


def init(args, row, cache, benchmark):
    with Image.open(cache / (row["bod"].lower() + ".png")) as body:
        width, height = body.size
    seam = args.seam or catalog().get(args.row, (None, ""))[0]
    if seam is None:
        sys.exit("Measure the bottom of donor hair, beard or hood and pass its y + 4 with --seam.")
    doll_type = int(row["Paperdoll type"])
    limits = {0: (67, 231), 1: (0, 166), 4: (0, 214)}
    left, right = limits.get(doll_type, (0, width))
    box = [left, 0, min(width, right), min(height, seam + 75)]
    side = max(box[2] - left, box[3])
    cfg = dict(base_row=args.row, donor_body=str(cache / (row["bod"].lower() + ".png")), head_box=box,
               square_canvas=side, upscale=round(1000 / side), seam_y=seam, seam_band=24, difference_threshold=30,
               dark_value=.35, skin_match_other=.15, skin_match_strengths=[1., .65, .35],
               identity_dir=str(args.work.parent / "face/final"), identity_frames=[1, 19, 26, 28])
    if benchmark:
        cfg["benchmark"] = str(benchmark)
    with (args.work / "graft.json").open("x") as out:
        json.dump(cfg, out, indent=2)


def catalog_sheet(args, rows, cache):
    tiles = []
    for number in args.rows:
        fetch(number, rows, cache)
        stem = rows[number]["bod"].lower()
        tiles.append((f"{number} · {stem}\n" + textwrap.fill(catalog()[number][1], 25),
                      Image.open(cache / (stem + ".png")).convert("RGBA")))
    width = max(img.width for _, img in tiles) + 16
    height = max(img.height for _, img in tiles) + 120
    cols = len(tiles) if args.master else min(len(tiles), 5)
    head_height = 2 * (args.head_crop[3] - args.head_crop[1]) if args.master else 0
    offset = width if args.master else 0
    image = Image.new("RGB", (offset + cols * width, head_height + (len(tiles) + cols - 1) // cols * height),
                      "#383840")
    font = ImageFont.truetype("DejaVuSans.ttf", 15)
    if args.master:
        with Image.open(args.master) as source:
            master = source.convert("RGBA")
        master.thumbnail((width, head_height - 32), LANCZOS)
        image.paste(master, ((width - master.width) // 2, 0), master)
        ImageDraw.Draw(image).text((10, head_height - 27), "Мастер · кадр 01", font=font)
    for i, (label, tile) in enumerate(tiles):
        x, y = offset + (i % cols) * width, head_height + (i // cols) * height
        if args.master:
            head = tile.crop(args.head_crop)
            head = head.resize((head.width * 2, head.height * 2), Image.Resampling.NEAREST)
            image.paste(head, (x + (width - head.width) // 2, 0), head)
        image.paste(tile, (x + (width - tile.width) // 2, y), tile)
        ImageDraw.Draw(image).multiline_text((x + 4, y + height - 115), label, font=font)
    image.save(args.work / "catalog.png")

def prepare(WORK, CFG, CROP):
    side = CFG['square_canvas']
    square = Image.new('RGBA', (side, side))
    square.paste(CROP, ((side-CROP.width)//2, (side-CROP.height)//2))
    square.resize((side*CFG['upscale'],)*2, LANCZOS).save(WORK/'donor.png')
    strip = Image.new('RGBA', (300*len(CFG['identity_frames']), 402))
    for i, number in enumerate(CFG['identity_frames']):
        frame = Image.open(Path(CFG['identity_dir'])/f'{number:02d}.png').convert('RGBA')
        assert frame.size == (944, 1264)
        strip.paste(frame.resize((300,402), LANCZOS), (300*i,0))
    strip.save(WORK/'identity.png')
    seams = CROP.resize((CROP.width*3, CROP.height*3), Image.Resampling.NEAREST)
    for offset in [-CFG['seam_band']//2, 0, CFG['seam_band']//2]:
        y = (CFG['seam_y']+offset)*3
        ImageDraw.Draw(seams).line((0,y,seams.width,y), fill='orange' if offset else 'red')
    seams.save(WORK/'seams.png')
    prompt = (DOCS / "doll.md").read_text().split("```text\n", 1)[1].split("```", 1)[0]
    card = card_values(WORK)
    prompt = prompt.replace("<hair from card.yml>", card["hair"])
    identity = ", ".join(card[k] for k in ("face", "eyes", "brows", "nose", "lips", "marks"))
    (WORK / "prompt.txt").write_text(prompt.replace("<face, eyes, brows, nose, lips and marks from card.yml>", identity))


def register(raw, crop, seam_y):
    """Find the scale and shift that put the generated figure on the donor: Codex returns a square image and may
    reframe a tall crop, so search the scale widely and template-match the donor's body below the seam (OpenCV)."""
    import cv2
    import numpy as np
    target = np.asarray(crop).astype(np.float32)
    top = min(seam_y + 8, crop.height - 16)
    template = np.ascontiguousarray(target[top:, :, :3])
    mask = np.ascontiguousarray(np.repeat((target[top:, :, 3:4] > 200).astype(np.float32), 3, axis=2))
    if not mask.any():
        raise ValueError("Empty donor registration mask")
    source = np.asarray(raw).astype(np.float32)
    pad = max(crop.width, crop.height)

    def match(scale):
        size = (max(1, round(raw.width * scale)), max(1, round(raw.height * scale)))
        resized = np.asarray(Image.fromarray(source.astype(np.uint8), "RGBA").resize(size, Image.Resampling.LANCZOS))
        resized = resized.astype(np.float32)
        padded = np.zeros((size[1] + 2 * pad, size[0] + 2 * pad, 4), np.float32)
        padded[pad:pad + size[1], pad:pad + size[0]] = resized
        # Transparent pixels count as black, so the body must land on opaque generated pixels.
        rgb = np.ascontiguousarray(padded[:, :, :3] * (padded[:, :, 3:4] / 255))
        if rgb.shape[0] < template.shape[0] or rgb.shape[1] < template.shape[1]:
            return None
        scores = cv2.matchTemplate(rgb, template, cv2.TM_SQDIFF, mask=mask)
        _, _, location, _ = cv2.minMaxLoc(scores)
        error = scores[location[1], location[0]] / max(mask.sum(), 1)
        return float(error), location, padded

    fit = crop.height / raw.height
    best = None
    for scale in np.concatenate([np.linspace(fit * 0.55, fit * 1.15, 49)]):
        result = match(scale)
        if result and (best is None or result[0] < best[0]):
            best = (result[0], scale, result[1], result[2])
    for scale in np.linspace(best[1] * 0.985, best[1] * 1.015, 13):
        result = match(scale)
        if result and result[0] < best[0]:
            best = (result[0], scale, result[1], result[2])
    error, scale, (lx, ly), padded = best
    # Template top-left (0, top) in the donor sits at (lx, ly) in the padded generated image.
    ox, oy = lx, ly - top
    canvas = padded[oy:oy + crop.height, ox:ox + crop.width]
    full = np.zeros(target.shape, np.float32)
    full[:canvas.shape[0], :canvas.shape[1]] = canvas
    return np.sqrt(error), scale / fit, ox - pad, oy - pad, full.astype(float)


def srgb_to_lab(rgb):
    import numpy as np
    c = rgb / 255.0
    c = np.where(c > 0.04045, ((c + 0.055) / 1.055) ** 2.4, c / 12.92)
    xyz = c @ np.array([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]]).T
    xyz /= np.array([0.95047, 1.0, 1.08883])
    f = np.where(xyz > 0.008856, np.cbrt(xyz), 7.787 * xyz + 16 / 116)
    return np.stack([116 * f[..., 1] - 16, 500 * (f[..., 0] - f[..., 1]), 200 * (f[..., 1] - f[..., 2])], -1)


def lab_to_srgb(lab):
    import numpy as np
    fy = (lab[..., 0] + 16) / 116
    f = np.stack([fy + lab[..., 1] / 500, fy, fy - lab[..., 2] / 200], -1)
    xyz = np.where(f > 0.2069, f ** 3, (f - 16 / 116) / 7.787) * np.array([0.95047, 1.0, 1.08883])
    c = xyz @ np.array([[3.2406, -1.5372, -0.4986], [-0.9689, 1.8758, 0.0415], [0.0557, -0.2040, 1.0570]]).T
    c = np.where(c > 0.0031308, 1.055 * np.clip(c, 0, None) ** (1 / 2.4) - 0.055, 12.92 * c)
    return np.clip(c * 255, 0, 255)


def skin_mask(pixels):
    import numpy as np
    """Opaque skin-coloured pixels: warm hue, moderate saturation, not too dark."""
    rgb = pixels[..., :3] / 255.0
    high, low = rgb.max(-1), rgb.min(-1)
    saturation = np.where(high > 0, (high - low) / np.maximum(high, 1e-6), 0)
    warm = (rgb[..., 0] >= rgb[..., 1]) & (rgb[..., 1] >= rgb[..., 2] * 0.85)
    return (pixels[..., 3] > 220) & warm & (saturation > 0.15) & (saturation < 0.75) & (high > 0.2)


def match_skin(generated, donor, seam, strength_other, strength):
    from scipy import ndimage
    import numpy as np
    """Move the new head's skin colour (Lab mean and spread) onto the donor head's, which was rendered under the same
    light as the body below the seam. Non-skin pixels (hair, eyes) get part of the shift so the head keeps one light."""
    region = np.zeros(generated.shape[:2], bool)
    region[:seam] = True
    # Target: the donor's skin just below the seam (neck, shoulders, chest), where the colours must meet; donors whose
    # face differs from their own body (pc18) would otherwise leave a step. Few skin pixels there (beards, robes,
    # skulls): the donor's own head skin, rendered under the same light.
    join = np.zeros(generated.shape[:2], bool)
    join[seam:seam + 60] = True
    target = skin_mask(donor) & join
    labels, count = ndimage.label(ndimage.binary_opening(skin_mask(generated) & region, iterations=2))
    sizes = np.bincount(labels.ravel(), minlength=count + 1)
    # ndimage labels in raster order, so the first eligible component starts highest.
    eligible = np.flatnonzero(sizes[1:] >= 150) + 1
    if not eligible.size:
        raise ValueError("No generated skin component of at least 150 pixels")
    source = labels == eligible[0]
    if min(source.sum(), target.sum()) < 150:
        raise ValueError(f"Skin mask too small: source={source.sum()} donor={target.sum()}")
    lab = srgb_to_lab(generated[..., :3])
    donor_lab = srgb_to_lab(donor[..., :3])
    mean_s, std_s = lab[source].mean(0), lab[source].std(0) + 1e-6
    mean_t, std_t = donor_lab[target].mean(0), donor_lab[target].std(0)
    moved = (lab - mean_s) / std_s * std_t + mean_t
    weight = np.where(source, 1.0, strength_other)[..., None]
    out = generated.copy()
    out[..., :3] = lab_to_srgb(lab + strength * weight * (moved - lab))
    return out, source, {"skin_source_count": int(source.sum()), "skin_target_count": int(target.sum()), "skin_source_rgb": [round(v) for v in generated[source][:, :3].mean(0)],
                 "skin_target_rgb": [round(v) for v in donor[target][:, :3].mean(0)]}


def head_weight(generated, donor, threshold, seam, dark_value):
    import numpy as np
    """Where the new head differs from the donor: the generated pixels there replace the donor's. Codex copies the
    body around the head closely, so shoulders, arms and clothes keep the donor's own pixels (no tone step), while
    the new face, hair, beard or hood comes through, also below the donor's hair line. Holes are filled so a face
    is never patched with the donor's, and the edge is feathered."""
    from scipy import ndimage
    gen_p = generated[:, :, :3] * generated[:, :, 3:4] / 255
    don_p = donor[:, :, :3] * donor[:, :, 3:4] / 255
    difference = np.maximum(np.abs(gen_p - don_p).max(2), np.abs(generated[:, :, 3] - donor[:, :, 3]))
    mask = difference > threshold
    mask = ndimage.binary_opening(mask, iterations=1)
    mask = ndimage.binary_closing(mask, structure=np.ones((3, 3)), iterations=4)
    mask = ndimage.binary_fill_holes(mask)
    labels, count = ndimage.label(mask)
    if count:
        # Keep the head: components touching the top third, or large ones (long hair, beards, hoods).
        sizes = ndimage.sum(mask, labels, range(1, count + 1))
        tops = ndimage.minimum(np.indices(mask.shape)[0], labels, range(1, count + 1))
        keep = [i + 1 for i in range(count) if tops[i] < mask.shape[0] / 3 or sizes[i] > 400]
        mask = np.isin(labels, keep)
    if not mask.any():
        raise ValueError("Empty head difference mask")
    mask = ndimage.binary_dilation(mask, iterations=2)
    dark = ((generated[..., 3] > 220) & (donor[..., 3] > 220)
            & (generated[..., :3].max(axis=2) < dark_value * 255)
            & (donor[..., :3].max(axis=2) < dark_value * 255))
    dark[seam:] = False
    grown = ndimage.binary_propagation(mask, structure=np.ones((3, 3)), mask=mask | dark)
    added = grown & ~mask
    weights = np.clip(ndimage.gaussian_filter(grown.astype(float), 1.2) * 1.4, 0, 1)
    weights[added] = 1
    return weights, mask, added



def extract(name, WORK, CFG, BODY, BOX, CROP):
    import numpy as np
    folder = WORK/'qa'/name
    folder.mkdir(exist_ok=True, parents=True)
    bodies = WORK/'bodies'
    bodies.mkdir(exist_ok=True)
    raw = Image.open(WORK/'raw'/f'{name}.png').convert('RGBA')
    seam, band = CFG['seam_y'], CFG['seam_band']
    error, scale, dx, dy, registered = register(raw, CROP, seam)
    donor = np.asarray(CROP).astype(float)
    sampling = raw.height / (CROP.height * scale)
    report = dict(registration_error=float(error), scale_vs_height_fit=float(scale),
                  offset=[int(dx), int(dy)], sampling=float(sampling), checks={})
    green(name, WORK)
    for strength in CFG['skin_match_strengths']:
        generated, skin, counts = match_skin(registered, donor, seam, CFG['skin_match_other'], strength)
        report.update(counts)
        weights, difference, added = head_weight(generated, donor, CFG['difference_threshold'],
                                                     seam, CFG.get('dark_value', 0.35))
        # Weight of the generated pixels: 1 above the band, 0 below, linear inside it.
        rows = np.clip((seam + band / 2 - np.arange(CROP.height)) / band, 0, 1)[:, None]
        weight = (rows * weights)[:, :, None]
        # Premultiplied blend so the head's own alpha (hair edges) survives.
        gen_p = generated[:, :, :3] * generated[:, :, 3:4] / 255
        don_p = donor[:, :, :3] * donor[:, :, 3:4] / 255
        alpha = weight[:, :, 0] * generated[:, :, 3] + (1 - weight[:, :, 0]) * donor[:, :, 3]
        colour = weight * gen_p + (1 - weight) * don_p
        out = np.zeros_like(donor)
        out[:, :, 3] = alpha
        nz = alpha > 0
        out[nz, :3] = colour[nz] * 255 / alpha[nz, None]
        head_pixels = np.clip(np.rint(out), 0, 255).astype(np.uint8)
        # Below the blend band the weight is 0: copy the donor rows exactly rather than through the float blend.
        keep = seam + (band + 1) // 2
        head_pixels[keep:] = np.asarray(CROP)[keep:]
        head = Image.fromarray(head_pixels, "RGBA")
        result = BODY.copy()
        result.paste(head, BOX[:2])
        tag = f"s{round(100*strength):03}"
        path = bodies/f"{name}_{tag}.png"
        result.save(path)
        saved = np.array(Image.open(path))
        visible = saved[:,:,3] > 0
        invalid = any(np.any(visible & np.all(saved[:,:,:3] == colour, axis=2))
                      for colour in [(255,0,255), (0,255,255)])
        report['checks'][tag] = dict(format=result.size==BODY.size and path.read_bytes()[24:26]==bytes([8,6]),
                                     size=path.stat().st_size<=1000000,
                                     rows=np.array_equal(saved[keep:],np.asarray(BODY)[keep:]),
                                     colours=not invalid, sampling=bool(sampling>=2))
        if tag == 's065':
            mask = result.crop(BOX).convert('RGB')
            overlay = np.array(mask, dtype=float)
            red = (difference*rows*.45)[:,:,None]
            overlay = overlay*(1-red) + np.array([255,0,0])*red
            overlay[added] = .45*overlay[added]+.55*np.array([40,150,255])
            overlay[skin] = .4*overlay[skin]+.6*np.array([0,255,0])
            Image.fromarray(overlay.astype('uint8')).save(folder/'mask.png')
    (folder/'report.json').write_text(json.dumps(report,indent=2)+'\n')

def green(name, WORK):
    raw = Image.open(WORK/'raw'/f'{name}.png').convert('RGBA')
    bg = Image.new('RGBA', raw.size, 'lime')
    bg.alpha_composite(raw)
    bg.save(WORK/'raw'/f'{name}_on_green.png')

def sheet(path, entries, factor, box=None, guides=()):
    tiles = []
    for label, img in entries:
        tile = img.crop(box) if box else img.copy()
        tile = tile.resize((tile.width*factor, tile.height*factor), Image.Resampling.NEAREST)
        tiles.append((label,tile))
    canvas = Image.new('RGB',(sum(tile.width for _,tile in tiles), tiles[0][1].height+28),(54,54,62))
    x=0
    draw=ImageDraw.Draw(canvas)
    for label,tile in tiles:
        canvas.paste(tile,(x,28),tile.getchannel('A'))
        draw.text((x+8,6),label,fill='white')
        for y in guides:
            for xx in range(x,x+tile.width,12):
                draw.line((xx,28+y*factor,xx+3,28+y*factor),fill='#dcbe79')
        x+=tile.width
    canvas.save(path)

def qa(work, cfg, body_image, row, types):
    base = Path(cfg['donor_body']).parent
    prefix = row["bod"]
    seam = cfg['seam_y']
    cutoff = seam - cfg['seam_band']//2
    donor_bbox = body_image.getchannel('A').crop((0,0,body_image.width,cutoff)).getbbox()
    def view(img, state):
        result = Image.open(base / (row["Background"].lower() + ".png")).convert("RGBA")
        result.alpha_composite(img, (2 * int(row["Body X"]), -2 * int(row["Body Y"])))
        offsets = types[int(row["Paperdoll type"])]
        for role in STATES[state]:
            pos = (2 * int(offsets[HANDS[role] + " X"]), -2 * int(offsets[HANDS[role] + " Y"]))
            hand = Image.open(base / (row[role].lower() + ".png")).convert("RGBA")
            result.alpha_composite(hand, pos)
        return result
    out = work / "qa"
    sources = [(prefix, body_image)]
    if cfg.get("benchmark"):
        sources.append(("MM9", Image.open(cfg["benchmark"]).convert("RGBA")))
    candidates = sorted((p.name for p in out.iterdir() if re.fullmatch(r"g[1-9][0-9]*", p.name)),
                        key=lambda name: int(name[1:]))
    if not candidates:
        sys.exit("No extracted candidates")
    def body(name,tag):
        return Image.open(work/'bodies'/f'{name}_{tag}.png').convert('RGBA')
    all_bodies=sources+[(name,body(name,'s065')) for name in candidates]
    box=tuple(cfg['head_box'])
    heads=[(label,img.crop(box)) for label,img in all_bodies]
    portrait=Image.open(Path(cfg['identity_dir'])/'01.png').convert('RGBA')
    portrait.thumbnail((140,150), LANCZOS)
    panel=Image.new('RGBA',(140,150))
    panel.paste(portrait,((140-portrait.width)//2,0))
    sheet(out/'heads-x4.png',heads+[('Portrait 01',panel)],4,guides=(50,88,seam))
    sheet(out/'dolls-x2.png',[(label,view(img,'empty')) for label,img in all_bodies],2)
    for name,img in all_bodies[len(sources):]:
        sheet(out/f'skin-{name}.png',[(tag,body(name,tag)) for tag in ('s100','s065','s035')],4,box)
        sheet(out/f'neck-{name}.png',sources+[(name,img)],6,(0,seam-27,body_image.width,seam+18))
        sheet(out/f'states-{name}.png',[(state,view(img,state)) for state in STATES],1)

    lines = ['# Раунд 1', '', f'Донор {prefix}. Шов y={seam}, рамка до y={cutoff}: {donor_bbox}.', '',
             '| Кандидат | Ошибка RGB RMS | Масштаб | Смещение | Sampling | Кожа, лицо / донор | Проверки | WARN | Файлы |',
             '|---|---:|---:|---|---:|---|---|---|---|']
    for name in candidates:
        path = out/name/'report.json'
        r = json.loads(path.read_text())
        img = body(name,'s065')
        bbox = img.getchannel('A').crop((0,0,img.width,cfg['seam_y']-cfg['seam_band']//2)).getbbox()
        warnings = []
        if abs(r['scale_vs_height_fit']-1) > .08:
            warnings.append('масштаб')
        if abs((bbox[2]-bbox[0])/(donor_bbox[2]-donor_bbox[0])-1)>.12:
            warnings.append('рамка головы')
        if bbox[1] == 0:
            warnings.append('касание верхнего края')
        r.update(head_bbox=bbox, warnings=warnings)
        path.write_text(json.dumps(r,indent=2,ensure_ascii=False)+'\n')
        status='PASS' if all(all(c.values()) for c in r['checks'].values()) else 'FAIL'
        lines.append(f"| {name} | {r['registration_error']:.2f} | {r['scale_vs_height_fit']:.3f} | {r['offset']} | "
                     f"{r['sampling']:.2f} | {r['skin_source_count']} / {r['skin_target_count']} | {status} | "
                     f"{', '.join(warnings) or '—'} | [тон](skin-{name}.png) · [шея](neck-{name}.png) · "
                     f"[руки](states-{name}.png) · [маска]({name}/mask.png) · [отчёт]({name}/report.json) |")
    (out/'summary.md').write_text('\n'.join(lines)+'\n')
    
def recolor(args, cfg, donor):
    import cv2

    source = np.array(Image.open(args.body).convert("RGBA"))
    base = np.asarray(donor)
    hsv = cv2.cvtColor(base[:, :, :3].astype(np.float32) / 255, cv2.COLOR_RGB2HSV)
    if not args.hue_range and Path(cfg["donor_body"]).stem != "pc02bod":
        sys.exit("Use --from LOW-HIGH to select the donor fabric.")
    lo, hi = map(float, (args.hue_range or "170-260").split("-"))
    mask = (base[:, :, 3] > 0) & (hsv[:, :, 1] > .10)
    hue = hsv[:, :, 0]
    mask &= ((lo <= hue) & (hue <= hi)) if lo <= hi else ((lo <= hue) | (hue <= hi))
    mask[:cfg["seam_y"] + cfg["seam_band"] // 2] = False
    if not mask.any():
        sys.exit("Empty garment mask")
    hsv[:, :, 0] = (hue + args.hue) % 360
    hsv[:, :, 1] = np.clip(hsv[:, :, 1] * args.sat, 0, 1)
    original = Image.fromarray(source.copy())
    source[mask, :3] = np.rint(cv2.cvtColor(hsv, cv2.COLOR_HSV2RGB)[mask] * 255).astype(np.uint8)
    result = Image.fromarray(source)
    result.save(args.body.with_stem(args.body.stem + "_recolor"))
    out = args.work / "qa"
    out.mkdir(exist_ok=True)
    Image.fromarray(mask).save(out / "costume-mask.png")
    sheet(out / "costume-variants.png", [("Original", original), ("Recolor", result)], 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, required=True)
    commands = parser.add_subparsers(dest="action", required=True)
    cmd = commands.add_parser("fetch-row")
    cmd.add_argument("row", type=int)
    cmd = commands.add_parser("catalog-sheet")
    cmd.add_argument("--rows", type=int, nargs="+", required=True)
    cmd.add_argument("--master", type=Path)
    cmd.add_argument("--head-crop", type=int, nargs=4, metavar=("LEFT", "TOP", "RIGHT", "BOTTOM"))
    cmd = commands.add_parser("init")
    cmd.add_argument("--row", type=int, required=True)
    cmd.add_argument("--seam", type=int)
    commands.add_parser("prepare")
    cmd = commands.add_parser("extract")
    cmd.add_argument("candidate", type=candidate_name)
    commands.add_parser("qa")
    cmd = commands.add_parser("package")
    cmd.add_argument("--id", dest="package_id", help="Use a distinct ID for this doll variant")
    cmd.add_argument("candidate", type=candidate_name)
    cmd.add_argument("tone", choices=["s035", "s065", "s100"])
    cmd.add_argument("--recolored", action="store_true")
    cmd = commands.add_parser("recolor")
    cmd.add_argument("body", type=Path)
    cmd.add_argument("--hue", required=True, type=float)
    cmd.add_argument("--sat", type=float, default=1.)
    cmd.add_argument("--from", dest="hue_range")
    args = parser.parse_args()
    if args.action == "catalog-sheet" and bool(args.master) != bool(args.head_crop):
        parser.error("--master and --head-crop must be used together")
    work = args.work = args.work.resolve()
    cache = work.parents[1] / "_cache/dolls"
    work.mkdir(parents=True, exist_ok=True)
    if args.action in ("fetch-row", "catalog-sheet", "init", "qa"):
        rows = table("character_data", "StatsUI")
    if args.action == "catalog-sheet":
        return catalog_sheet(args, rows, cache)
    if args.action in ("fetch-row", "init"):
        benchmark = fetch(args.row, rows, cache)
        if args.action == "init":
            init(args, rows[args.row], cache, benchmark)
        return
    cfg = json.loads((work / "graft.json").read_text())
    donor = Image.open(cfg["donor_body"]).convert("RGBA")
    box = tuple(cfg["head_box"])
    crop = donor.crop(box)
    if args.action == "prepare":
        prepare(work, cfg, crop)
    elif args.action == "extract":
        extract(args.candidate, work, cfg, donor, box, crop)
    elif args.action == "qa":
        qa(work, cfg, donor, rows[cfg["base_row"]], table("doll_types", "#"))
    elif args.action == "recolor":
        recolor(args, cfg, donor)
    else:
        checks = json.loads((work / "qa" / args.candidate / "report.json").read_text())["checks"][args.tone]
        if not all(checks.values()):
            sys.exit(f"Candidate failed QA: {checks}")
        body = f"{args.candidate}_{args.tone}" + ("_recolor" if args.recolored else "") + ".png"
        package_doll = work / "package-doll"
        package_doll.mkdir(exist_ok=True)
        shutil.copyfile(work / "bodies" / body, package_doll / "body.png")
        card = card_values(work)
        package_id = args.package_id or card["id"]
        subprocess.run([sys.executable, str(REPO / "tools/make_portrait_package.py"), cfg["identity_dir"],
                        "--id", package_id, "--name", card["name"], "--base-character", str(cfg["base_row"]),
                        "--doll", str(package_doll), "--zip", "--output", str(work.parent / "package" / package_id)],
                       check=True)


def candidate_name(value):
    if not re.fullmatch(r"g[1-9][0-9]*", value):
        raise argparse.ArgumentTypeError("Expected candidate name such as g2")
    return value


if __name__ == "__main__":
    main()
