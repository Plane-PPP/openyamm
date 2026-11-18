"""Prepare and register Medusa paperdoll head candidates."""
import json
import sys
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'portraits/med01/doll-graft'
if '--pc04' in sys.argv:
    sys.argv.remove('--pc04')
    WORK = WORK / 'pc04'
CFG = json.loads((WORK / 'graft.json').read_text())
BODY = Image.open(ROOT / CFG['donor_body']).convert('RGBA')
BOX = tuple(CFG['head_box'])
CROP = BODY.crop(BOX)
LANCZOS = Image.Resampling.LANCZOS

def prepare():
    side = CFG['square_canvas']
    square = Image.new('RGBA', (side, side))
    square.paste(CROP, ((side-CROP.width)//2, (side-CROP.height)//2))
    square.resize((side*CFG['upscale'],)*2, LANCZOS).save(WORK/'donor.png')
    strip = Image.new('RGBA', (300*len(CFG['identity_frames']), 402))
    for i, number in enumerate(CFG['identity_frames']):
        frame = Image.open(ROOT/CFG['identity_dir']/f'{number:02d}.png').convert('RGBA')
        assert frame.size == (944, 1264)
        strip.paste(frame.resize((300,402), LANCZOS), (300*i,0))
    strip.save(WORK/'identity.png')
    seams = CROP.resize((CROP.width*3, CROP.height*3), Image.Resampling.NEAREST)
    for offset in [-CFG['seam_band']//2, 0, CFG['seam_band']//2]:
        y = (CFG['seam_y']+offset)*3
        ImageDraw.Draw(seams).line((0,y,seams.width,y), fill='orange' if offset else 'red')
    seams.save(WORK/'seams.png')
    brief = (ROOT/'docs/design/doll-generation-brief-v3.md').read_text()
    (WORK/'prompt.txt').write_text(brief.split('```text\n')[1].split('```')[0])

def register(raw, crop, seam_y):
    """Find the scale and shift that put the generated figure on the donor: Codex returns a square image and may
    reframe a tall crop, so search the scale widely and template-match the donor's body below the seam (OpenCV)."""
    import cv2
    import numpy as np
    target = np.asarray(crop).astype(np.float32)
    top = min(seam_y + 8, crop.height - 16)
    template = np.ascontiguousarray(target[top:, :, :3])
    mask = np.ascontiguousarray(np.repeat((target[top:, :, 3:4] > 200).astype(np.float32), 3, axis=2))
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
    if not count:
        raise ValueError("Empty generated skin mask")
    source = labels == (1 + np.argmax(np.bincount(labels.ravel())[1:]))
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


def head_weight(generated, donor, threshold):
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
    mask = ndimage.binary_dilation(mask, iterations=2)
    return np.clip(ndimage.gaussian_filter(mask.astype(float), 1.2) * 1.4, 0, 1)



def extract(name):
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
    for strength in CFG['skin_match_strengths']:
        generated, skin, counts = match_skin(registered, donor, seam, CFG['skin_match_other'], strength)
        report.update(counts)
        weights = head_weight(generated, donor, CFG['difference_threshold'])
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
        report['checks'][tag] = dict(format=result.size==(166,594) and path.read_bytes()[24:26]==bytes([8,6]),
                                     size=path.stat().st_size<=1000000,
                                     rows=np.array_equal(saved[keep:],np.asarray(BODY)[keep:]),
                                     colours=not invalid, top=not bool(visible[0].any()), sampling=bool(sampling>=2))
        if tag == 's065':
            mask = result.crop(BOX).convert('RGB')
            overlay = np.array(mask, dtype=float)
            red = (weights*rows*.45)[:,:,None]
            overlay = overlay*(1-red) + np.array([255,0,0])*red
            overlay[skin] = .4*overlay[skin]+.6*np.array([0,255,0])
            Image.fromarray(overlay.astype('uint8')).save(folder/'mask.png')
    (folder/'report.json').write_text(json.dumps(report,indent=2)+'\n')

def green(name):
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

def qa():
    from doll_full import STATES, HANDS
    base=(ROOT/CFG['donor_body']).parent
    prefix = Path(CFG['donor_body']).stem.removesuffix('bod')
    seam = CFG['seam_y']
    cutoff = seam - CFG['seam_band']//2
    donor_bbox = BODY.getchannel('A').crop((0,0,BODY.width,cutoff)).getbbox()
    def view(img, state):
        result = Image.open(base/'backdoll.png').convert('RGBA')
        result.alpha_composite(img,(120,94))
        for role, position in STATES[state]:
            filename = HANDS[role][0].replace('pc02',prefix)
            result.alpha_composite(Image.open(base/filename).convert('RGBA'),position)
        return result
    out=WORK/'qa'
    sources=[(prefix,BODY),('9'+prefix,Image.open(base/f'9{prefix}bod.png').convert('RGBA'))]
    candidates=['g1','g2','g3']
    def body(name,tag):
        return Image.open(WORK/'bodies'/f'{name}_{tag}.png').convert('RGBA')
    all_bodies=sources+[(name,body(name,'s065')) for name in candidates]
    box=(10,0,150,max(150,seam+12))
    heads=[(label,img.crop(box)) for label,img in all_bodies]
    portrait=Image.open(ROOT/CFG['identity_dir']/'01.png').convert('RGBA')
    portrait.thumbnail((140,150), LANCZOS)
    panel=Image.new('RGBA',(140,150))
    panel.paste(portrait,((140-portrait.width)//2,0))
    sheet(out/'heads-x4.png',heads+[('Portrait 01',panel)],4,guides=(50,88,seam))
    sheet(out/'dolls-x2.png',[(label,view(img,'empty')) for label,img in all_bodies],2)
    for name,img in all_bodies[2:]:
        sheet(out/f'skin-{name}.png',[(tag,body(name,tag)) for tag in ('s100','s065','s035')],4,box)
        sheet(out/f'neck-{name}.png',sources+[(name,img)],6,(0,seam-27,166,seam+18))
        sheet(out/f'states-{name}.png',[(state,view(img,state)) for state in STATES],1)

    lines = ['# Раунд 1', '', f'Донор {prefix}. Шов y={seam}, рамка до y={cutoff}: {donor_bbox}.', '',
             '| Кандидат | Ошибка RGB RMS | Масштаб | Смещение | Sampling | Кожа, лицо / донор | Проверки | WARN | Файлы |',
             '|---|---:|---:|---|---:|---|---|---|---|']
    for name in candidates:
        path = out/name/'report.json'
        r = json.loads(path.read_text())
        img = body(name,'s065')
        bbox = img.getchannel('A').crop((0,0,img.width,CFG['seam_y']-CFG['seam_band']//2)).getbbox()
        warnings = []
        if abs(r['scale_vs_height_fit']-1) > .08:
            warnings.append('масштаб')
        if abs((bbox[2]-bbox[0])/(donor_bbox[2]-donor_bbox[0])-1)>.12 or not 2<=bbox[1]<=14:
            warnings.append('рамка головы')
        r.update(head_bbox=bbox, warnings=warnings)
        path.write_text(json.dumps(r,indent=2,ensure_ascii=False)+'\n')
        status='PASS' if all(all(c.values()) for c in r['checks'].values()) else 'FAIL'
        lines.append(f"| {name} | {r['registration_error']:.2f} | {r['scale_vs_height_fit']:.3f} | {r['offset']} | "
                     f"{r['sampling']:.2f} | {r['skin_source_count']} / {r['skin_target_count']} | {status} | "
                     f"{', '.join(warnings) or '—'} | [тон](skin-{name}.png) · [шея](neck-{name}.png) · "
                     f"[руки](states-{name}.png) · [маска]({name}/mask.png) · [отчёт]({name}/report.json) |")
    (out/'summary.md').write_text('\n'.join(lines)+'\n')
    
if __name__ == '__main__':
    {'prepare':prepare, 'extract':extract, 'green':green,'qa':qa}[sys.argv[1]](*sys.argv[2:])
