#!/usr/bin/env python3
"""Portrait kit: puts generated portrait frames on one canvas and checks the 56-frame set.

The original Might and Magic party portraits (pc01..pc29, 7pcNN) share one layout: the head is seen through a fixed
egg-shaped window (a soft oval that touches the bottom edge). Every frame keeps the same oval, except the duck frames
35 and 36, where the whole oval with the head drops down. Inside the oval the head turns, nods and changes expression;
its size does not change. This kit reproduces that layout on a 944x1264 canvas (x8 of the in-game 118x158).

Subcommands (all need Pillow and numpy; run with `python3 tools/portrait_kit.py <command> --help`):

  mask       write the oval template (alpha) and a guide overlay with the canonical lines
  grid       draw a labelled pixel grid over an image, to read landmark coordinates by eye
  regions    preview the edit regions of the master (from landmarks.json) for checking
  compose    align one generated image to the master and build a canvas frame (expression, colour or pose)
  qa         check a folder of 01..56.png and write review sheets, animations and a report

Typical flow (see docs/design/portrait-generation-brief-v2.md):
  compose --kind master raw/01.png --out final/01.png             # place the master on the canvas
  (write final/landmarks.json for the master)
  compose --kind eyes  raw/13.png --master final/01.png --out final/13.png
  compose --kind pose  raw/25.png --master final/01.png --out final/25.png
  qa final --reference portraits/reference/mm --partial          # during the run
  qa final --reference portraits/reference/mm                    # final: must end with FAIL 0
"""

import argparse
import json
import math
import sys
from pathlib import Path

try:
    import numpy as np
    from PIL import Image, ImageDraw, ImageFilter
except ImportError:
    sys.exit("Pillow and numpy are required: python3 -m pip install pillow numpy")

W, H = 944, 1264
GAME_W, GAME_H = 118, 158

# Oval window measured on the originals (mean alpha of 57 characters' frame 01, scaled x8).
OVAL_CX, OVAL_CY, OVAL_A, OVAL_B = 472.0, 640.0, 430.0, 622.0
OVAL_FEATHER = 14.0  # alpha ramp in pixels, about 1.75 px in game

# Canonical master landmarks (frame 01), pixels on the 944x1264 canvas.
CANON = {
    "pupil_y": 620,           # 49 %
    "pupil_left_x": 330,      # 35 % (left side of the image)
    "pupil_right_x": 614,     # 65 %
    "nose_tip_y": 800,        # 63 %
    "mouth_y": 935,           # 74 %
    "chin_y": 1110,           # 88 %, the neck continues below and fades into the oval
}
CANON_TOL = {"pupil_y": 25, "pupil_left_x": 30, "pupil_right_x": 30, "nose_tip_y": 40, "mouth_y": 40, "chin_y": 45}

# How far the whole oval drops in the duck frames (fraction of the height), measured on pc02/7pc02/7pc05/pc14.
DUCK_SHIFT = {35: 0.25, 36: 0.52}

KIND_OF_FRAME = {}
for _n in (2, 3, 5, 6, 7, 10, 13, 14, 15, 16, 17, 20, 21, 22, 23, 24, 27, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
           51, 52, 53, 55, 56):
    KIND_OF_FRAME[_n] = "face"
for _n in (8, 9, 12, 54):
    KIND_OF_FRAME[_n] = "head"
for _n in (4, 11, 18, 19, 25, 26, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39):
    KIND_OF_FRAME[_n] = "pose"
KIND_OF_FRAME[1] = "master"

COLOUR_FRAMES = {8, 9, 12, 54}

SEQUENCES = {
    # name: (frames, raw frame length each; 1 raw unit = 1/16 s, portrait_frame_data.txt)
    "blink": ([1, 1, 1, 1, 13], [8, 8, 8, 8, 2]),
    "talk": ([21, 22, 21, 23, 24, 21, 1], [2, 2, 2, 2, 2, 2, 8]),
    "no": ([1, 27, 26, 25, 26, 27, 28, 29, 28, 27, 1], [8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 8]),
    "yes": ([1, 32, 33, 32, 31, 30, 31, 1], [8, 2, 2, 2, 2, 2, 2, 8]),
    "dodge": ([1, 34, 35, 36, 35, 34, 1], [4, 4, 4, 4, 4, 4, 8]),
    "damage": ([1, 37, 1, 38, 1, 39, 1], [8, 8, 8, 8, 8, 8, 8]),
    "wake": ([4, 5, 19, 18, 1], [8, 8, 8, 8, 8]),
    "look": ([1, 17, 1, 18, 1, 19, 1, 20, 1], [8] * 9),
}


# ----------------------------------------------------------------------------------------------------------------
# basic helpers

def load_rgba(path):
    return np.asarray(Image.open(path).convert("RGBA")).astype(np.float32) / 255.0


def save_rgba(arr, path):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(np.clip(arr * 255.0 + 0.5, 0, 255).astype(np.uint8), "RGBA").save(path)


def oval_alpha(width=W, height=H, shift_y=0.0, grow=1.0):
    """Soft oval window; shift_y is a fraction of the height (duck frames), grow enlarges the axes."""
    sx, sy = width / W * grow, height / H * grow
    yy, xx = np.mgrid[0:height, 0:width].astype(np.float32)
    cx, cy = OVAL_CX * width / W, (OVAL_CY + shift_y * H) * height / H
    a, b = OVAL_A * sx, OVAL_B * sy
    r = np.sqrt(((xx + 0.5 - cx) / a) ** 2 + ((yy + 0.5 - cy) / b) ** 2)
    feather = OVAL_FEATHER * sx / min(a, b)
    return np.clip((1.0 - r) / feather + 0.5, 0.0, 1.0)


def luma(rgb):
    return rgb[..., 0] * 0.299 + rgb[..., 1] * 0.587 + rgb[..., 2] * 0.114


def blur(mask, radius):
    img = Image.fromarray(np.clip(mask * 255, 0, 255).astype(np.uint8), "L")
    return np.asarray(img.filter(ImageFilter.GaussianBlur(radius))).astype(np.float32) / 255.0


def ellipse_mask(cx, cy, rx, ry, feather):
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    r = np.sqrt(((xx - cx) / max(rx, 1)) ** 2 + ((yy - cy) / max(ry, 1)) ** 2)
    return np.clip((1.0 - r) * min(rx, ry) / max(feather, 1) + 0.5, 0, 1)


def warp(arr, matrix, size):
    """matrix maps OUTPUT pixel (x, y, 1) to SOURCE pixel; returns float RGBA (premultiplied resampling)."""
    premul = arr.copy()
    premul[..., :3] *= premul[..., 3:4]
    channels = []
    for c in range(4):
        img = Image.fromarray(premul[..., c], "F")
        out = img.transform(size, Image.AFFINE, data=tuple(matrix[:2].reshape(-1)), resample=Image.BICUBIC)
        channels.append(np.asarray(out))
    res = np.stack(channels, -1)
    alpha = np.clip(res[..., 3:4], 0, 1)
    res[..., :3] = np.where(alpha > 1e-4, res[..., :3] / np.maximum(alpha, 1e-4), 0)
    res[..., 3:4] = alpha
    return np.clip(res, 0, 1)


def similarity(scale, angle_deg, tx, ty, cx, cy):
    """Matrix mapping OUTPUT coordinates to SOURCE coordinates for: source = scale*R*(out - c) + c + t."""
    a = math.radians(angle_deg)
    c, s = math.cos(a) * scale, math.sin(a) * scale
    m = np.array([[c, -s, 0.0], [s, c, 0.0], [0.0, 0.0, 1.0]])
    m[0, 2] = cx + tx - (c * cx - s * cy)
    m[1, 2] = cy + ty - (s * cx + c * cy)
    return m


def head_box(alpha, threshold=0.5):
    ys, xs = np.nonzero(alpha > threshold)
    if len(xs) == 0:
        raise SystemExit("image has no opaque pixels: the generator must return the head on a transparent background")
    return xs.min(), xs.max(), ys.min(), ys.max()


def matte(arr, background_tolerance=0.06):
    """Use the image alpha; if the image is opaque, key out a flat background sampled at the corners."""
    if arr[..., 3].min() < 0.98:
        return arr
    corners = np.concatenate([arr[:8, :8, :3].reshape(-1, 3), arr[:8, -8:, :3].reshape(-1, 3)])
    bg = np.median(corners, 0)
    dist = np.sqrt(((arr[..., :3] - bg) ** 2).sum(-1))
    out = arr.copy()
    out[..., 3] = np.clip((dist - background_tolerance) / background_tolerance, 0, 1)
    print("note: opaque input, flat background", np.round(bg * 255).astype(int), "keyed out")
    return out


# ----------------------------------------------------------------------------------------------------------------
# landmarks and regions

def read_landmarks(master_path):
    path = Path(master_path).with_name("landmarks.json")
    if not path.exists():
        raise SystemExit(f"{path} is missing: write the master's landmarks first (see the brief, step 2)")
    data = json.loads(path.read_text())
    need = ["pupil_left", "pupil_right", "brow_y", "nose_tip", "mouth_left", "mouth_right", "chin"]
    missing = [k for k in need if k not in data]
    if missing:
        raise SystemExit(f"{path}: missing keys {missing}")
    return data


def region_mask(kind, lm):
    """Edit region on the master for expression frames. Coordinates are canvas pixels."""
    (plx, ply), (prx, pry) = lm["pupil_left"], lm["pupil_right"]
    eye_dist = prx - plx
    eye_cy = (ply + pry) / 2
    brow_y = lm["brow_y"]
    mlx, mly = lm["mouth_left"]
    mrx, mry = lm["mouth_right"]
    chin_y = lm["chin"][1]
    nose_y = lm["nose_tip"][1]
    feather = eye_dist * 0.12
    eyes = ellipse_mask((plx + prx) / 2, (brow_y + eye_cy) / 2 + eye_dist * 0.05, eye_dist * 0.95,
                        (eye_cy - brow_y) + eye_dist * 0.42, feather)
    mouth_cy = (mly + mry) / 2
    mouth = ellipse_mask((mlx + mrx) / 2, (nose_y + chin_y) / 2 + eye_dist * 0.04, (mrx - mlx) * 1.05,
                         (chin_y - nose_y) / 2 + eye_dist * 0.08, feather)
    face = np.maximum(eyes, mouth)
    face = np.maximum(face, ellipse_mask((plx + prx) / 2, (eye_cy + mouth_cy) / 2, eye_dist * 0.75,
                                         (mouth_cy - eye_cy) * 0.75, feather))
    return {"eyes": eyes, "mouth": mouth, "face": face}[kind]


# ----------------------------------------------------------------------------------------------------------------
# registration

def structure(arr, size):
    """Downscaled band-passed luma over black: keeps the silhouette and the large shapes, ignores tone shifts."""
    img = Image.fromarray(np.clip(arr * 255, 0, 255).astype(np.uint8), "RGBA").resize(size, Image.BILINEAR)
    a = np.asarray(img).astype(np.float32) / 255.0
    lum = luma(a[..., :3]) * a[..., 3]
    return lum - box_blur(lum, max(1, size[0] // 12))


def box_blur(x, r):
    """Separable box blur, applied twice (close to a Gaussian); float arrays of any range."""
    for _ in range(2):
        for axis in (0, 1):
            pad = [(0, 0), (0, 0)]
            pad[axis] = (r + 1, r)
            c = np.cumsum(np.pad(x, pad, mode="edge"), axis=axis, dtype=np.float64)
            n = x.shape[axis]
            hi = np.take(c, np.arange(2 * r + 1, 2 * r + 1 + n), axis=axis)
            lo = np.take(c, np.arange(0, n), axis=axis)
            x = ((hi - lo) / (2 * r + 1)).astype(np.float32)
    return x


def ncc(a, b, mask):
    w = mask / max(mask.sum(), 1e-6)
    ma, mb = (a * w).sum(), (b * w).sum()
    da, db = a - ma, b - mb
    cov = (da * db * w).sum()
    return cov / math.sqrt(max((da * da * w).sum() * (db * db * w).sum(), 1e-12))


def register(master, gen, zone, initial_scale, initial_t, search_scale=0.10, search_angle=5.0, search_shift=0.06):
    """Find source = s*R*(out-c)+c+t that best matches gen to master inside zone. Returns (3x3 matrix, score)."""
    cx, cy = W / 2, H / 2
    # (downscale, steps for scale, angle, shift)
    levels = ((8, 11, 6, 13), (4, 5, 5, 5), (2, 5, 3, 5), (1, 3, 3, 3))
    centre = (initial_scale, 0.0, initial_t[0], initial_t[1])
    span = (initial_scale * search_scale, search_angle, W * search_shift, H * search_shift)
    best = None
    for div, n_s, n_a, n_t in levels:
        size = (W // div, H // div)
        ref = structure(master, size)
        zmask = np.asarray(Image.fromarray((zone * 255).astype(np.uint8), "L").resize(size, Image.BILINEAR)) / 255.0
        g = structure(gen, (gen.shape[1] // div, gen.shape[0] // div))
        g_img = Image.fromarray(g, "F")
        down = np.diag([div, div, 1.0])
        candidates = []
        for s in np.linspace(centre[0] - span[0], centre[0] + span[0], n_s):
            for ang in np.linspace(centre[1] - span[1], centre[1] + span[1], n_a):
                for tx in np.linspace(centre[2] - span[2], centre[2] + span[2], n_t):
                    for ty in np.linspace(centre[3] - span[3], centre[3] + span[3], n_t):
                        m_small = np.linalg.inv(down) @ similarity(s, ang, tx, ty, cx, cy) @ down
                        warped = g_img.transform(size, Image.AFFINE, data=tuple(m_small[:2].reshape(-1)),
                                                 resample=Image.BILINEAR)
                        candidates.append((ncc(ref, np.asarray(warped), zmask), s, ang, tx, ty))
        best = max(candidates, key=lambda c: c[0])
        centre = best[1:]
        # next level searches +-1.5 steps of this level around the best point
        span = (span[0] * 3 / max(n_s - 1, 1), span[1] * 3 / max(n_a - 1, 1),
                span[2] * 3 / max(n_t - 1, 1), span[3] * 3 / max(n_t - 1, 1))
    return similarity(best[1], best[2], best[3], best[4], cx, cy), best[0]


def colour_match(src, ref, mask):
    """Per-channel mean/std transfer of src towards ref, statistics taken inside mask."""
    out = src.copy()
    w = mask > 0.5
    if w.sum() < 100:
        return out
    for c in range(3):
        s, r = src[..., c][w], ref[..., c][w]
        gain = np.clip(r.std() / max(s.std(), 1e-4), 0.8, 1.25)
        out[..., c] = (src[..., c] - s.mean()) * gain + r.mean()
    return np.clip(out, 0, 1)


def finish(arr):
    """Opaque core to 255, no pure magenta/cyan (the HUD keys them out)."""
    out = arr.copy()
    a = out[..., 3]
    a[a > 0.985] = 1.0
    a[a < 0.01] = 0.0
    rgb8 = np.round(out[..., :3] * 255)
    for key in ((255, 0, 255), (0, 255, 255)):
        hit = np.all(rgb8 == key, -1)
        out[..., 1][hit] = np.where(out[..., 1][hit] > 0.5, 254 / 255, 1 / 255)
    return out


# ----------------------------------------------------------------------------------------------------------------
# commands

def cmd_mask(args):
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    alpha = oval_alpha()
    Image.fromarray((alpha * 255).astype(np.uint8), "L").save(out / "oval-alpha-944x1264.png")
    guide = Image.new("RGBA", (W, H), (128, 128, 128, 255))
    d = ImageDraw.Draw(guide)
    d.ellipse([OVAL_CX - OVAL_A, OVAL_CY - OVAL_B, OVAL_CX + OVAL_A, OVAL_CY + OVAL_B], outline=(255, 255, 255), width=3)
    d.line([(W / 2, 0), (W / 2, H)], fill=(200, 200, 200), width=1)
    for key, colour in (("pupil_y", (255, 40, 40)), ("nose_tip_y", (255, 170, 0)), ("mouth_y", (255, 170, 0)),
                        ("chin_y", (255, 120, 0))):
        d.line([(0, CANON[key]), (W, CANON[key])], fill=colour, width=3)
        d.text((8, CANON[key] - 14), key, fill=colour)
    for key in ("pupil_left_x", "pupil_right_x"):
        x = CANON[key]
        d.ellipse([x - 10, CANON["pupil_y"] - 10, x + 10, CANON["pupil_y"] + 10], outline=(255, 40, 40), width=3)
    guide.save(out / "layout-guide-944x1264.png")
    print("wrote", out / "oval-alpha-944x1264.png", "and", out / "layout-guide-944x1264.png")


def cmd_grid(args):
    img = Image.open(args.image).convert("RGBA")
    bg = Image.new("RGBA", img.size, (128, 128, 128, 255))
    bg.alpha_composite(img)
    d = ImageDraw.Draw(bg)
    step = args.step
    for x in range(0, img.width, step):
        d.line([(x, 0), (x, img.height)], fill=(255, 255, 0, 255) if x % (step * 5) == 0 else (0, 255, 0, 255))
        d.text((x + 2, 2), str(x), fill=(255, 255, 0, 255))
    for y in range(0, img.height, step):
        d.line([(0, y), (img.width, y)], fill=(255, 255, 0, 255) if y % (step * 5) == 0 else (0, 255, 0, 255))
        d.text((2, y + 2), str(y), fill=(255, 255, 0, 255))
    if args.crop:
        x0, y0, x1, y1 = args.crop
        bg = bg.crop((x0, y0, x1, y1)).resize(((x1 - x0) * 2, (y1 - y0) * 2), Image.NEAREST)
    bg.convert("RGB").save(args.out)
    print("wrote", args.out)


def cmd_regions(args):
    master = load_rgba(args.master)
    lm = read_landmarks(args.master)
    view = master.copy()
    view[..., :3] = view[..., :3] * view[..., 3:4] + 0.5 * (1 - view[..., 3:4])
    view[..., 3] = 1
    tint = {"eyes": (1, 0.2, 0.2), "mouth": (0.2, 0.4, 1)}
    for kind, colour in tint.items():
        m = region_mask(kind, lm)[..., None]
        view[..., :3] = view[..., :3] * (1 - 0.45 * m) + np.array(colour) * 0.45 * m
    save_rgba(view, args.out)
    print("wrote", args.out)


def place_master(gen, lm_hint):
    """Scale and move a generated master so its pupils land on the canonical points."""
    (lx, ly), (rx, ry) = lm_hint
    src_mid = np.array([(lx + rx) / 2, (ly + ry) / 2])
    dst_mid = np.array([(CANON["pupil_left_x"] + CANON["pupil_right_x"]) / 2, CANON["pupil_y"]])
    scale = (rx - lx) / (CANON["pupil_right_x"] - CANON["pupil_left_x"])
    angle = math.degrees(math.atan2(ry - ly, rx - lx))
    a = math.radians(angle)
    c, s = math.cos(a) * scale, math.sin(a) * scale
    m = np.array([[c, -s, 0.0], [s, c, 0.0], [0, 0, 1.0]])
    m[:2, 2] = src_mid - m[:2, :2] @ dst_mid
    return m


def cmd_compose(args):
    gen = matte(load_rgba(args.image))
    kind = args.kind
    if kind == "master":
        if not args.pupils:
            raise SystemExit("--kind master needs --pupils LX LY RX RY (pupil centres in the generated image)")
        p = args.pupils
        m = place_master(gen, ((p[0], p[1]), (p[2], p[3])))
        out = warp(gen, m, (W, H))
        out[..., 3] *= oval_alpha()
        save_rgba(finish(out), args.out)
        print("master placed: pupils ->", CANON["pupil_left_x"], CANON["pupil_right_x"], "at y", CANON["pupil_y"],
              "| now write landmarks.json next to it and check with: qa / grid")
        return

    if not args.master:
        raise SystemExit("--master is required")
    master = load_rgba(args.master)
    frame = args.frame
    shift = DUCK_SHIFT.get(frame, 0.0) if frame else 0.0
    window = oval_alpha(shift_y=shift)

    # initial guess: match the head width (upper half of the silhouette) and the top of the head
    mx0, mx1, my0, _ = head_box(master[..., 3])
    gx0, gx1, gy0, _ = head_box(gen[..., 3])
    s0 = (gx1 - gx0) / max(mx1 - mx0, 1)
    t0 = ((gx0 + gx1) / 2 - s0 * (mx0 + mx1) / 2 - (W / 2 - s0 * W / 2),
          gy0 - s0 * my0 - (H / 2 - s0 * H / 2))

    if kind in ("eyes", "mouth", "face", "head"):
        lm = read_landmarks(args.master)
        edit = region_mask("face", lm) if kind == "head" else region_mask(kind, lm)
        zone = np.clip(master[..., 3] - blur(edit, 6) * 1.2, 0, 1)  # register on what must NOT change
        if kind == "head":
            zone = master[..., 3]
        m, score = register(master, gen, zone, s0, t0)
        aligned = warp(gen, m, (W, H))
        print(f"registration score {score:.3f} (below 0.6: the generator changed the hair or head; check or regenerate)")
        if kind == "head":
            out = aligned.copy()
            out[..., 3] = master[..., 3]
            hole = (aligned[..., 3] < 0.5) & (master[..., 3] > 0.5)
            out[..., :3][hole] = master[..., :3][hole]
        else:
            if not args.no_colour_match:
                ring = np.clip(blur(edit, 20) - edit, 0, 1) + (edit > 0.5) * 0
                aligned[..., :3] = colour_match(aligned[..., :3], master[..., :3], np.maximum(ring, edit) * master[..., 3])
            r = edit[..., None] * np.minimum(aligned[..., 3:4] * 1.02, 1)
            out = master.copy()
            out[..., :3] = master[..., :3] * (1 - r) + aligned[..., :3] * r
    elif kind == "pose":
        if args.align == "register":
            m, score = register(master, gen, master[..., 3], s0, t0, search_scale=0.12, search_angle=8,
                                search_shift=0.10)
            print(f"registration score {score:.3f}")
        else:
            # silhouette: same head width and top as the master, centred
            m = similarity(s0, 0.0, t0[0], t0[1], W / 2, H / 2)
        if shift:
            m = m @ np.array([[1, 0, 0], [0, 1, -shift * H], [0, 0, 1.0]])
        out = warp(gen, m, (W, H))
        if not args.no_colour_match:
            out[..., :3] = colour_match(out[..., :3], master[..., :3], np.minimum(out[..., 3], master[..., 3]))
    else:
        raise SystemExit(f"unknown kind {kind}")
    out[..., 3] = np.minimum(out[..., 3], window) if kind == "pose" else out[..., 3]
    save_rgba(finish(out), args.out)
    print("wrote", args.out)


# ----------------------------------------------------------------------------------------------------------------
# QA

def frame_stats(arr):
    a = arr[..., 3]
    rgb = arr[..., :3]
    core = a > 0.9
    lum = luma(rgb)
    yy, xx = np.mgrid[0:arr.shape[0], 0:arr.shape[1]]
    wl = core * lum
    mx, mn = rgb.max(-1), rgb.min(-1)
    sat = np.where(mx > 1e-3, (mx - mn) / np.maximum(mx, 1e-3), 0)
    return {
        "area": float(a.sum()),
        "cx": float((a * xx).sum() / max(a.sum(), 1)),
        "cy": float((a * yy).sum() / max(a.sum(), 1)),
        "lcx": float((wl * xx).sum() / max(wl.sum(), 1)),  # brightness centroid: follows the face when it turns
        "lcy": float((wl * yy).sum() / max(wl.sum(), 1)),
        "luma": float(lum[core].mean() * 255) if core.any() else 0.0,
        "sat": float(sat[core].mean()) if core.any() else 0.0,
        "rg": float((rgb[..., 0] - rgb[..., 1])[core].mean() * 255) if core.any() else 0.0,
        "yb": float(((rgb[..., 0] + rgb[..., 1]) / 2 - rgb[..., 2])[core].mean() * 255) if core.any() else 0.0,
    }


def iou(a, b):
    a, b = a > 0.5, b > 0.5
    return float((a & b).sum() / max((a | b).sum(), 1))


class Report:
    def __init__(self):
        self.lines = []
        self.fails = 0
        self.warns = 0

    def check(self, ok, what, warn_only=False):
        if ok:
            return
        if warn_only:
            self.warns += 1
            self.lines.append("WARN " + what)
        else:
            self.fails += 1
            self.lines.append("FAIL " + what)

    def info(self, text):
        self.lines.append("     " + text)


def checker_bg(size, cell=8):
    w, h = size
    yy, xx = np.mgrid[0:h, 0:w]
    c = (((xx // cell) + (yy // cell)) % 2) * 20 + 50
    return Image.fromarray(np.stack([c, c, c, np.full_like(c, 255)], -1).astype(np.uint8), "RGBA")


def game_size(arr):
    img = Image.fromarray(np.clip(arr * 255 + 0.5, 0, 255).astype(np.uint8), "RGBA")
    return img.resize((GAME_W, GAME_H), Image.LANCZOS)


def cmd_qa(args):
    src = Path(args.folder)
    out = Path(args.out or src / "qa")
    out.mkdir(parents=True, exist_ok=True)
    rep = Report()
    frames = {}
    for n in range(1, 57):
        p = src / f"{n:02d}.png"
        if not p.exists():
            rep.check(False, f"{n:02d}: missing {p.name}", warn_only=args.partial)
            continue
        img = Image.open(p)
        rep.check(img.mode == "RGBA", f"{n:02d}: mode {img.mode}, need RGBA")
        rep.check(img.size == (W, H), f"{n:02d}: size {img.size}, need {W}x{H} (use compose)")
        frames[n] = np.asarray(img.convert("RGBA").resize((W, H), Image.LANCZOS)).astype(np.float32) / 255.0
    if 1 not in frames:
        print("\n".join(rep.lines))
        sys.exit(1)

    master = frames[1]
    st = {n: frame_stats(a) for n, a in frames.items()}
    m = st[1]

    # 1. format, forbidden colours, window
    for n, a in frames.items():
        rgb8 = np.round(a[..., :3] * 255).astype(int)
        vis = a[..., 3] > 0.01
        bad = (np.all(rgb8 == (255, 0, 255), -1) | np.all(rgb8 == (0, 255, 255), -1)) & vis
        rep.check(bad.sum() == 0, f"{n:02d}: {bad.sum()} pure #FF00FF/#00FFFF pixels (transparent in the HUD)")
        window = oval_alpha(shift_y=DUCK_SHIFT.get(n, 0.0), grow=1.04)
        outside = (a[..., 3] > 0.5) & (window < 0.01)
        rep.check(outside.sum() < 200, f"{n:02d}: {outside.sum()} opaque pixels outside the oval window (+4 %)")
        edge_rows = a[-3:, :, 3] > 0.5
        rep.check(edge_rows.sum(1).max() < W * 0.45 or n in DUCK_SHIFT,
                  f"{n:02d}: bottom edge is a wide straight cut; the oval must narrow towards the bottom")
        core = a[..., 3] > 0.98
        rep.check(core.sum() > 0.35 * W * H or n in DUCK_SHIFT,
                  f"{n:02d}: fully opaque area only {core.sum() / (W * H):.0%} of the canvas (head too small, or "
                  "alpha not normalised)", warn_only=True)

    # 2. master layout
    area_oval = float(oval_alpha().sum())
    rep.check(0.80 <= m["area"] / area_oval <= 1.02,
              f"01: head fills {m['area'] / area_oval:.0%} of the oval; originals fill 90-100 % (hair to the edge)",
              warn_only=m["area"] / area_oval > 0.72)
    rep.check(95 <= m["luma"] <= 130, f"01: mean head brightness {m['luma']:.0f}; originals 77-117 (pc12 77, pc02 117), "
              "aim 95-125 (too dark reads as a blob at 118x158)", warn_only=80 <= m["luma"] <= 150)
    gap = H - 1 - int(np.nonzero((master[..., 3] > 0.5).any(1))[0].max())
    rep.check(gap <= 12, f"01: the head/neck ends {gap} px above the bottom edge; in the originals the neck or hair "
              "reaches the bottom of the oval (a gap shows as an empty curve under the neck in the HUD)",
              warn_only=True)
    lm_path = src / "landmarks.json"
    lm = None
    if lm_path.exists():
        lm = json.loads(lm_path.read_text())
        checks = {"pupil_y": (lm["pupil_left"][1] + lm["pupil_right"][1]) / 2, "pupil_left_x": lm["pupil_left"][0],
                  "pupil_right_x": lm["pupil_right"][0], "nose_tip_y": lm["nose_tip"][1],
                  "mouth_y": (lm["mouth_left"][1] + lm["mouth_right"][1]) / 2, "chin_y": lm["chin"][1]}
        for key, value in checks.items():
            rep.check(abs(value - CANON[key]) <= CANON_TOL[key],
                      f"01: {key} = {value:.0f}, canonical {CANON[key]} ± {CANON_TOL[key]}")
    else:
        rep.check(False, "landmarks.json missing: master layout and edit regions cannot be checked", warn_only=True)

    # 3. per-frame consistency
    rep.info("frame  kind   IoU01  dArea   dLuma  static  change  lcx-01  lcy-01")
    for n in sorted(frames):
        if n == 1:
            continue
        a, s = frames[n], st[n]
        kind = KIND_OF_FRAME[n]
        i = iou(a[..., 3], master[..., 3])
        d_area = s["area"] / m["area"] - 1
        d_luma = s["luma"] - m["luma"]
        static = change = float("nan")
        if lm is not None and kind == "face":
            edit = region_mask("face", lm)
            keep = (blur(edit, 12) < 0.02) & (master[..., 3] > 0.9) & (a[..., 3] > 0.9)
            diff = np.abs(a[..., :3] - master[..., :3]).mean(-1) * 255
            static = float(diff[keep].mean())
            inside = (edit > 0.5) & (master[..., 3] > 0.9)
            change = float(diff[inside].mean())
            rep.check(static <= 2.5, f"{n:02d}: hair/snakes/outline changed vs 01 (mean diff {static:.1f}); an "
                      "expression frame must be the master outside the face", warn_only=static <= 6)
            rep.check(change >= 2.0 or n == 27, f"{n:02d}: face almost identical to 01 (diff {change:.1f})",
                      warn_only=True)
        rep.info(f"{n:02d}     {kind:6} {i:5.2f}  {d_area:+6.1%}  {d_luma:+6.1f}  {static:6.1f}  {change:6.1f}  "
                 f"{(s['lcx'] - m['lcx']) / W:+6.1%}  {(s['lcy'] - m['lcy']) / H:+6.1%}")
        if n in DUCK_SHIFT:
            dy = (s["cy"] - m["cy"]) / H
            want = {35: (0.09, 0.17), 36: (0.20, 0.34)}[n]
            rep.check(want[0] <= dy <= want[1], f"{n:02d}: silhouette dropped {dy:.0%}, originals {want[0]:.0%}-"
                      f"{want[1]:.0%}")
            continue
        rep.check(i >= 0.90, f"{n:02d}: silhouette IoU with 01 is {i:.2f}; the oval window and head size must stay "
                  "(originals 0.90-0.99)", warn_only=i >= 0.85)
        area_limit = 0.06 if kind == "face" else 0.08
        rep.check(abs(d_area) <= area_limit, f"{n:02d}: silhouette area {d_area:+.0%} vs 01 (head size jump)",
                  warn_only=abs(d_area) <= area_limit * 1.75)
        if n not in COLOUR_FRAMES:
            # pose and damage frames may change shading (turned face, bruises); expression frames may not
            limit = 10 if kind == "face" else 22
            rep.check(abs(d_luma) <= limit, f"{n:02d}: brightness {d_luma:+.0f} vs 01 (tone drift, flickers in the HUD)",
                      warn_only=abs(d_luma) <= limit * 1.6)

    # 4. colour frames
    def colour(n, ok, text, warn_only=True):
        if n in st:
            rep.check(ok(st[n]), f"{n:02d}: {text}", warn_only=warn_only)
    colour(8, lambda s: s["yb"] - m["yb"] >= 6, "poison must be clearly more yellow than 01")
    colour(9, lambda s: s["rg"] - m["rg"] >= 3, "disease must add red blotches")
    colour(12, lambda s: s["sat"] <= 0.12, "petrified must be grey stone (saturation <= 0.12)", warn_only=False)
    colour(54, lambda s: s["rg"] - m["rg"] >= 8, "straining must flush the face red")

    # 5. motion sequences: direction and symmetry, from the brightness centroid of the head
    def dx(n):
        return (st[n]["lcx"] - m["lcx"]) / W if n in st else 0.0

    def dy(n):
        return (st[n]["lcy"] - m["lcy"]) / H if n in st else 0.0
    if all(n in st for n in (25, 26, 27, 28, 29)):
        xs = [dx(n) for n in (25, 26, 27, 28, 29)]
        mono = all(b > a for a, b in zip(xs, xs[1:])) or all(b < a for a, b in zip(xs, xs[1:]))
        rep.check(mono, "no 25-29: the face brightness centre does not move steadily from one side to the other "
                  + " ".join(f"{x:+.1%}" for x in xs) + " (proxy; confirm in anim-no.gif)", warn_only=True)
        rep.check(abs(xs[0]) >= 0.015 and abs(xs[4]) >= 0.015, "no 25/29: the head turn may be too small to read",
                  warn_only=True)
        rep.check(min(abs(xs[0]), abs(xs[4])) / max(abs(xs[0]), abs(xs[4]), 1e-6) >= 0.5,
                  "no 25/29: the turns to the two sides are very unequal", warn_only=True)
    if all(n in st for n in (30, 31, 32, 33)):
        ys = [dy(n) for n in (30, 31, 32, 33)]
        mono = all(b > a for a, b in zip(ys, ys[1:])) or all(b < a for a, b in zip(ys, ys[1:]))
        rep.check(mono, "yes 30-33: the face does not move steadily up/down " + " ".join(f"{y:+.1%}" for y in ys),
                  warn_only=True)

    # 6. review material
    reference = Path(args.reference) if args.reference else None
    cols = 8
    cell_w, cell_h = GAME_W + 6, GAME_H * (2 if reference else 1) + 18
    sheet = Image.new("RGBA", (cols * cell_w, 7 * cell_h), (34, 30, 28, 255))
    d = ImageDraw.Draw(sheet)
    for n in range(1, 57):
        x, y = ((n - 1) % cols) * cell_w, ((n - 1) // cols) * cell_h
        d.text((x + 2, y + 2), f"{n:02d} {KIND_OF_FRAME[n]}", fill=(255, 220, 120, 255))
        if n in frames:
            sheet.alpha_composite(game_size(frames[n]), (x, y + 16))
        if reference:
            for prefix in args.reference_prefix:
                rp = reference / f"{prefix}-{n:02d}.png"
                if rp.exists():
                    sheet.alpha_composite(Image.open(rp).convert("RGBA").resize((GAME_W, GAME_H)), (x, y + 16 + GAME_H))
                    break
    sheet.save(out / "sheet-game-size.png")

    big = Image.new("RGBA", (cols * 240, 7 * 340), (34, 30, 28, 255))
    d = ImageDraw.Draw(big)
    for n, a in frames.items():
        x, y = ((n - 1) % cols) * 240, ((n - 1) // cols) * 340
        img = Image.fromarray(np.clip(a * 255 + 0.5, 0, 255).astype(np.uint8), "RGBA").resize((236, 316), Image.LANCZOS)
        big.alpha_composite(img, (x, y + 20))
        d.text((x + 4, y + 4), f"{n:02d}", fill=(255, 220, 120, 255))
    big.save(out / "sheet-x2.png")

    # flicker check: |frame - master| heat map at game size for expression frames
    heat = Image.new("RGB", (cols * (GAME_W + 4), 7 * (GAME_H + 4)), (0, 0, 0))
    for n, a in frames.items():
        diff = np.abs(np.asarray(game_size(a)).astype(int) - np.asarray(game_size(master)).astype(int))[..., :3]
        hm = np.clip(diff.mean(-1) * 4, 0, 255).astype(np.uint8)
        heat.paste(Image.fromarray(hm, "L").convert("RGB"), (((n - 1) % cols) * (GAME_W + 4), ((n - 1) // cols) * (GAME_H + 4)))
    heat.save(out / "diff-vs-01.png")

    for name, (seq, lengths) in SEQUENCES.items():
        if not all(n in frames for n in seq):
            continue
        imgs = []
        for n in seq:
            bg = checker_bg((GAME_W * 2, GAME_H * 2))
            bg.alpha_composite(game_size(frames[n]).resize((GAME_W * 2, GAME_H * 2), Image.NEAREST))
            imgs.append(bg.convert("RGB"))
        imgs[0].save(out / f"anim-{name}.gif", save_all=True, append_images=imgs[1:], loop=0,
                     duration=[int(1000 * l / 16) for l in lengths])

    text = [f"Portrait set QA: {src}", f"FAIL {rep.fails}  WARN {rep.warns}", ""] + rep.lines + [
        "", "Review files: sheet-game-size.png (top: set at game size, bottom: original reference), sheet-x2.png,",
        "diff-vs-01.png (bright = changed vs 01; expression frames must be dark outside the face),",
        "anim-*.gif (game timing: blink, talk, no, yes, dodge, damage, wake, look)."]
    (out / "report.txt").write_text("\n".join(text) + "\n")
    print("\n".join(text))
    sys.exit(1 if rep.fails else 0)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("mask", help="write the oval alpha template and layout guide")
    p.add_argument("--out", default="portraits/templates")
    p.set_defaults(func=cmd_mask)

    p = sub.add_parser("grid", help="draw a coordinate grid over an image to read landmarks")
    p.add_argument("image")
    p.add_argument("--out", required=True)
    p.add_argument("--step", type=int, default=20)
    p.add_argument("--crop", type=int, nargs=4, metavar=("X0", "Y0", "X1", "Y1"))
    p.set_defaults(func=cmd_grid)

    p = sub.add_parser("regions", help="preview the eyes/mouth edit regions of the master")
    p.add_argument("master")
    p.add_argument("--out", required=True)
    p.set_defaults(func=cmd_regions)

    p = sub.add_parser("compose", help="align a generated image to the master and build a canvas frame")
    p.add_argument("image", help="generated image (any size; transparent background, or a flat background)")
    p.add_argument("--kind", required=True, choices=["master", "eyes", "mouth", "face", "head", "pose"],
                   help="master: place frame 01; eyes/mouth/face: paste only that region onto the master; "
                        "head: whole-head colour change, master silhouette; pose: new head pose in the oval")
    p.add_argument("--master", help="final 01.png (with landmarks.json next to it)")
    p.add_argument("--frame", type=int, help="frame number (35 and 36 move the oval down)")
    p.add_argument("--pupils", type=float, nargs=4, metavar=("LX", "LY", "RX", "RY"),
                   help="master only: pupil centres in the generated image")
    p.add_argument("--align", choices=["silhouette", "register"], default="silhouette",
                   help="pose only: match head width/top (default) or search by image similarity")
    p.add_argument("--no-colour-match", action="store_true")
    p.add_argument("--out", required=True)
    p.set_defaults(func=cmd_compose)

    p = sub.add_parser("qa", help="check a folder of 01..56.png and write review material")
    p.add_argument("folder")
    p.add_argument("--out")
    p.add_argument("--reference", help="folder with original frames, e.g. pc02-01.png ... pc02-56.png")
    p.add_argument("--reference-prefix", nargs="+", default=["pc02", "7pc05"])
    p.add_argument("--partial", action="store_true", help="missing frames are warnings (checks during the run)")
    p.set_defaults(func=cmd_qa)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
