"""Paints the girl seen from behind: strand-simulated silver hair, satin bow, shaded body."""
import numpy as np, cv2, sys
from paintlib import *

W, H = 880, 1480          # left stage at 2x
SS = 2                    # supersampling for anti-aliased strands
OUT = sys.argv[1] if len(sys.argv) > 1 else '.'
CX, HYc, HR = 440.0, 520.0, 92.0      # head centre / radius
CROWN = np.array([440.0, 470.0])

def inflate_normals(mask, soft=6):
    soft = soft * 2.5
    m8 = (mask > 0.5).astype(np.uint8)
    d = cv2.distanceTransform(m8, cv2.DIST_L2, 5).astype(np.float32)
    h = np.sqrt(d)
    h = blur(h, soft)
    gx = cv2.Sobel(h, cv2.CV_32F, 1, 0, ksize=5)
    gy = cv2.Sobel(h, cv2.CV_32F, 0, 1, ksize=5)
    n = np.dstack([-gx, -gy, np.ones_like(gx) * 1.2])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    return n

def lit(mask, base, shadow, highlight, light, spec_pow=0, spec_amt=0.0, soft=6):
    n = inflate_normals(mask, soft)
    L = np.array(light, np.float32); L /= np.linalg.norm(L)
    lam = np.clip((n * L).sum(2), 0, 1)
    c = lerp(shadow, base, smoothstep(-0.2, 0.9, lam))
    c = lerp(c, highlight, smoothstep(0.85, 1.0, lam) * 0.4)
    if spec_pow:
        Hh = L + np.array([0, 0, 1], np.float32); Hh /= np.linalg.norm(Hh)
        spec = np.clip((n * Hh).sum(2), 0, 1) ** spec_pow
        c = c + spec[..., None] * spec_amt
    return c

def poly_mask(pts, h=H, w=W):
    m = np.zeros((h * SS, w * SS), np.uint8)
    cv2.fillPoly(m, [np.round(np.array(pts) * SS).astype(np.int32)], 255, cv2.LINE_AA)
    return cv2.resize(m, (w, h), interpolation=cv2.INTER_AREA).astype(np.float32) / 255.0

def ellipse_mask(cx, cy, rx, ry, ang=0):
    m = np.zeros((H * SS, W * SS), np.uint8)
    cv2.ellipse(m, (int(cx * SS), int(cy * SS)), (int(rx * SS), int(ry * SS)), ang, 0, 360, 255, -1, cv2.LINE_AA)
    return cv2.resize(m, (W, H), interpolation=cv2.INTER_AREA).astype(np.float32) / 255.0

def bez(p0, p1, p2, p3, n=40):
    t = np.linspace(0, 1, n)[:, None]
    return ((1 - t) ** 3) * p0 + 3 * ((1 - t) ** 2) * t * p1 + 3 * (1 - t) * t * t * p2 + t ** 3 * p3

# ---------------------------------------------------------------------------
def body_layers(rage):
    light = (0.55, -0.6, 0.6) if not rage else (0.0, -0.9, 0.35)
    skinB, skinS, skinH = (col('f8dccd'), col('d79c8e'), col('fff1e8')) if not rage else (col('6a3036'), col('1c060a'), col('a84a50'))
    dressB, dressS, dressH = (col('3d4a9a'), col('171c46'), col('6f82d8')) if not rage else (col('2a0812'), col('070103'), col('5a1424'))

    rgb = np.zeros((H, W, 3), np.float32); a = np.zeros((H, W), np.float32)

    # arms: tapered, slight elbow bend, hands tucked behind the hips
    for s_ in (-1, 1):
        x = lambda v: CX + s_ * v
        outer = list(bez(np.array([x(168), 705]), np.array([x(208), 760]), np.array([x(206), 930]), np.array([x(196), 1060])))
        outer += list(bez(np.array([x(196), 1060]), np.array([x(188), 1180]), np.array([x(176), 1300]), np.array([x(150), 1400])))
        inner = list(bez(np.array([x(110), 1395]), np.array([x(140), 1300]), np.array([x(150), 1180]), np.array([x(152), 1060])))
        inner += list(bez(np.array([x(152), 1060]), np.array([x(156), 930]), np.array([x(150), 800]), np.array([x(130), 730])))
        m = poly_mask(outer + inner)
        c = lit(m, skinB, skinS, skinH, light, soft=10)
        rgb = over(rgb, c, m); a = np.maximum(a, m)

    # neck + shoulders (skin)
    sh = [(CX - 40, 560), (CX + 40, 560), (CX + 48, 650)]
    sh += list(bez(np.array([CX + 48, 650]), np.array([CX + 110, 668]), np.array([CX + 165, 680]), np.array([CX + 185, 730])))
    sh += [(CX + 150, 800), (CX - 150, 800)]
    sh += list(bez(np.array([CX - 185, 730]), np.array([CX - 165, 680]), np.array([CX - 110, 668]), np.array([CX - 48, 650])))
    m = poly_mask(sh)
    c = lit(m, skinB, skinS, skinH, light, soft=12)
    # shoulder blades hint
    rgb = over(rgb, c, m); a = np.maximum(a, m)

    # dress back: fitted, with a laced opening and soft folds
    dr = list(bez(np.array([CX - 160, 760]), np.array([CX - 80, 735]), np.array([CX + 80, 735]), np.array([CX + 160, 760])))
    dr += list(bez(np.array([CX + 160, 760]), np.array([CX + 150, 950]), np.array([CX + 112, 1080]), np.array([CX + 118, 1160])))
    dr += list(bez(np.array([CX + 118, 1160]), np.array([CX + 125, 1260]), np.array([CX + 150, 1380]), np.array([CX + 158, 1480])))
    dr += list(bez(np.array([CX - 158, 1480]), np.array([CX - 150, 1380]), np.array([CX - 125, 1260]), np.array([CX - 118, 1160])))
    dr += list(bez(np.array([CX - 118, 1160]), np.array([CX - 112, 1080]), np.array([CX - 150, 950]), np.array([CX - 160, 760])))
    m = poly_mask(dr)
    c = lit(m, dressB, dressS, dressH, light, spec_pow=30, spec_amt=0.12, soft=16)
    fabric = fbm(H, W, 301, octaves=6, base=30, persistence=0.6)
    folds = fbm(H // 8, W, 302, octaves=5, base=12); folds = cv2.resize(folds, (W, H))
    c *= (0.9 + 0.12 * fabric[..., None]) * (0.85 + 0.3 * folds[..., None])
    rgb = over(rgb, c, m); a = np.maximum(a, m)

    # satin waist ribbon
    rib = poly_mask([(CX - 125, 1120), (CX + 125, 1120), (CX + 122, 1170), (CX - 122, 1170)])
    ribB, ribS, ribH = (col('ff8fc4'), col('b2417a'), col('ffe0f0')) if not rage else (col('a0001c'), col('2a0006'), col('ff5060'))
    c = lit(rib, ribB, ribS, ribH, light, spec_pow=40, spec_amt=0.35, soft=4)
    rgb = over(rgb, c, rib); a = np.maximum(a, rib)
    return rgb, a

# ---------------------------------------------------------------------------
def simulate_strands(seed, n=4600):
    """Each strand: crown -> over the back of the head -> falls down the back, blown right."""
    r = rng(seed)
    u = np.clip(r.normal(0, 0.55, n), -1.05, 1.05)          # lateral position across the head
    u += r.normal(0, 0.02, n)
    steps = 120
    t = np.linspace(0, 1, steps)[None, :, None]
    # part 1: over the scalp (quadratic bezier)
    p0 = np.stack([CROWN[0] + r.normal(0, 4, n), CROWN[1] + r.normal(0, 3, n)], 1)[:, None, :]
    ex = CX + u * HR * 0.9
    ey = HYc + np.sqrt(np.clip(1 - u ** 2, 0, 1)) * HR * 0.55 + 10
    p2 = np.stack([ex, ey], 1)[:, None, :]
    p1 = np.stack([CX + u * HR * 1.08, HYc - HR * 0.1 + np.abs(u) * 10], 1)[:, None, :]
    head = ((1 - t) ** 2) * p0 + 2 * (1 - t) * t * p1 + (t ** 2) * p2
    # part 2: falling (cubic bezier) with volume, wind and waves
    endy = r.uniform(1180, 1380, n) - np.abs(u) * 80
    wind = r.uniform(40, 110, n)
    q0 = p2[:, 0]
    q1 = np.stack([q0[:, 0] + u * 30, q0[:, 1] + 180], 1)
    q2 = np.stack([CX + u * HR * 1.55 + wind * 0.5, endy - 260], 1)
    q3 = np.stack([CX + u * HR * 1.7 + wind + r.normal(0, 12, n), endy], 1)
    tt = t[..., 0]
    fall = (((1 - tt) ** 3)[..., None] * q0[:, None] + (3 * (1 - tt) ** 2 * tt)[..., None] * q1[:, None]
            + (3 * (1 - tt) * tt ** 2)[..., None] * q2[:, None] + (tt ** 3)[..., None] * q3[:, None])
    phase = r.uniform(0, 6.28, n)[:, None]
    fall[..., 0] += np.sin(fall[..., 1] / 55.0 + phase) * 6 * tt
    pts = np.concatenate([head, fall[:, 1:]], 1).astype(np.float32)
    return pts, u

def hair_layer(rage):
    pts, u = simulate_strands(7)
    n = len(pts)
    clump = np.interp(u, np.linspace(-1.1, 1.1, 300), fbm1d(300, 31, octaves=5, base=12))
    r = rng(8)
    big = np.zeros((H * SS, W * SS, 3), np.uint8)
    mask = np.zeros((H * SS, W * SS), np.uint8)

    if not rage:
        deep, base, light_c, sheen = col('4f5577'), col('aeb4d0'), col('dfe3f2'), col('ffffff')
    else:
        deep, base, light_c, sheen = col('12040a'), col('3a0c18'), col('6a1a28'), col('ff6a6a')

    def to_bgr(c):
        c = np.clip(c, 0, 1)
        return (int(c[2] * 255), int(c[1] * 255), int(c[0] * 255))

    # rounded skull silhouette under the strands
    cv2.ellipse(big, (int(CX * SS), int((HYc - 6) * SS)), (int(HR * 1.04 * SS), int(HR * 1.1 * SS)), 0, 0, 360,
                to_bgr(deep), -1, cv2.LINE_AA)
    cv2.ellipse(mask, (int(CX * SS), int((HYc - 6) * SS)), (int(HR * 1.04 * SS), int(HR * 1.1 * SS)), 0, 0, 360, 255, -1, cv2.LINE_AA)
    order = r.permutation(n)
    for layer, (thick, frac) in enumerate([(9, 1.0), (4, 0.75), (2, 0.6)]):
        for idx in order[: int(n * frac)]:
            line = pts[idx]
            # lighting: lit from the upper right (calm) / backlit (rage); outer layers brighter
            side = np.clip((line[:, 0].mean() - CX) / 220, -1, 1)
            lightness = 0.3 + 0.22 * layer + 0.3 * (clump[idx] - 0.5) + (0.15 * side if not rage else 0.0) + r.normal(0, 0.06)
            c = lerp(deep, light_c, np.clip(lightness, 0, 1)) if layer else deep
            if layer == 2:
                c = lerp(base, light_c, np.clip(lightness, 0, 1))
            p = np.round(line * SS).astype(np.int32)
            cv2.polylines(big, [p], False, to_bgr(c), thick if layer == 0 else max(1, thick - 1), cv2.LINE_AA)
            if layer == 0:
                cv2.polylines(mask, [p], False, 255, thick + 2, cv2.LINE_AA)

    # strands on the top of the head radiate from the crown over the dome
    for i in range(1400):
        ang = r.uniform(-3.0, 3.0)
        rr = HR * r.uniform(0.9, 1.04)
        p0 = np.array([CX + r.normal(0, 3), HYc - 20 + r.normal(0, 3)])
        tip = np.array([CX + np.sin(ang) * rr, HYc - 6 - np.cos(ang) * rr * 1.06 * (1 if abs(ang) < 1.6 else -1)])
        ctrl = (p0 + tip) / 2 + (tip - p0)[::-1] * np.array([1, -1]) * 0.15
        seg = bez(p0, ctrl, ctrl, tip, 20)
        lt = 0.4 + 0.25 * np.exp(-((np.cos(ang) * rr * 1.06 - 45) / 18) ** 2) + 0.3 * (0.5 + 0.5 * np.sin(ang * 3 + i)) + (0.15 * np.sin(ang) if not rage else 0)
        cv2.polylines(big, [np.round(seg * SS).astype(np.int32)], False, to_bgr(lerp(base, light_c, np.clip(lt, 0, 1))), 2, cv2.LINE_AA)

    # glossy sheen bands: the angel ring on the head + a wavy band down the length
    for idx in order[: int(n * 0.55)]:
        line = pts[idx]
        if len(line) < 3: continue
        x = line[:, 0]; y = line[:, 1]
        ring_y = HYc - 40 + 0.005 * (x - CX) ** 2 + r.normal(0, 6)
        band2 = 880 + 40 * np.sin(x / 70.0 + r.normal(0, 0.3)) + r.normal(0, 18)
        w1 = np.abs(y - ring_y) < (10 + 6 * r.random())
        w2 = np.abs(y - band2) < (6 + 14 * r.random())
        for sel, strength in ((w1, 0.9), (w2, 0.45)):
            if sel.sum() > 1:
                seg = np.round(line[sel] * SS).astype(np.int32)
                cv2.polylines(big, [seg], False, to_bgr(lerp(light_c, sheen, strength * (0.6 + 0.4 * r.random()))), 2, cv2.LINE_AA)

    # flyaway strands
    for i in range(70):
        idx = order[i]
        line = pts[idx].copy()
        line[:, 0] += np.linspace(0, r.uniform(-40, 60), len(line)) * np.linspace(0, 1, len(line)) ** 2
        cv2.polylines(big, [np.round(line * SS).astype(np.int32)], False, to_bgr(light_c), 1, cv2.LINE_AA)

    rgbb = cv2.resize(big, (W, H), interpolation=cv2.INTER_AREA).astype(np.float32) / 255.0
    rgb = rgbb[..., ::-1].copy()
    a = cv2.resize(mask, (W, H), interpolation=cv2.INTER_AREA).astype(np.float32) / 255.0
    rgb = np.where(a[..., None] > 0.01, rgb / np.maximum(a[..., None], 0.01), 0)  # un-premultiply edges
    rgb = np.clip(rgb, 0, 1)
    # ambient occlusion toward the inside of the mass
    occl = blur(a, 20)
    rgb *= (0.75 + 0.25 * smoothstep(1.0, 0.6, occl))[..., None]
    return rgb, a

# ---------------------------------------------------------------------------
def bow_layer(rage):
    light = (0.55, -0.6, 0.6) if not rage else (0.0, -0.9, 0.35)
    B, S, Hh = (col('c8a6ff'), col('6a48b8'), col('f4ecff')) if not rage else (col('8a0018'), col('1c0004'), col('ff4a5a'))
    rgb = np.zeros((H, W, 3), np.float32); a = np.zeros((H, W), np.float32)
    bx, by = CX + 8, 600
    parts = []
    # tails blowing to the right
    for s, off in ((-1, 0), (1, 14)):
        tail = list(bez(np.array([bx + s * 6, by + 8]), np.array([bx + s * 20 + 30, by + 120]), np.array([bx + s * 30 + 80, by + 220]), np.array([bx + s * 18 + 120 + off, by + 330])))
        tail2 = list(bez(np.array([bx + s * 18 + 90 + off, by + 320]), np.array([bx + s * 14 + 50, by + 200]), np.array([bx + s * 4 + 10, by + 110]), np.array([bx - s * 8, by + 10])))
        parts.append((poly_mask(tail + [(bx + s * 18 + 110 + off, by + 300)] + tail2), 8))
    for sgn in (-1, 1):
        x = lambda v: bx + sgn * v
        loop = list(bez(np.array([x(10), by - 4]), np.array([x(40), by - 70]), np.array([x(120), by - 80]), np.array([x(140), by - 30])))
        loop += list(bez(np.array([x(140), by - 30]), np.array([x(150), by + 10]), np.array([x(90), by + 28]), np.array([x(10), by + 8])))
        parts.append((poly_mask(loop), 6))
    parts.append((ellipse_mask(bx, by - 2, 22, 26), 4))
    for m, soft in parts:
        c = lit(m, B, S, Hh, light, spec_pow=60, spec_amt=0.15, soft=soft)
        # satin folds
        folds = fbm(H, W, 900 + soft, octaves=4, base=20)
        c *= (0.85 + 0.25 * folds[..., None])
        rgb = over(rgb, c, m); a = np.maximum(a, m)
    # fold creases toward the knot
    for sgn in (-1, 1):
        for k in range(3):
            pth = bez(np.array([bx + sgn * 18, by - 2]), np.array([bx + sgn * 60, by - 30 + k * 16]), np.array([bx + sgn * 100, by - 40 + k * 18]), np.array([bx + sgn * 128, by - 34 + k * 20]), 30)
            cm = np.zeros((H, W), np.uint8)
            cv2.polylines(cm, [np.round(pth).astype(np.int32)], False, 255, 3, cv2.LINE_AA)
            cmf = blur(cm.astype(np.float32) / 255.0, 2) * a
            rgb = over(rgb, S * 0.9, cmf * 0.6)
    return rgb, a

def paint(rage):
    rgb, a = body_layers(rage)
    hr, ha = hair_layer(rage)
    rgb = over(rgb, hr, ha); a = np.maximum(a, ha)
    br, ba = bow_layer(rage)
    rgb = over(rgb, br, ba); a = np.maximum(a, ba)
    if rage:   # red moon rim light hugging the silhouette
        m8 = (a > 0.5).astype(np.uint8)
        d = cv2.distanceTransform(m8, cv2.DIST_L2, 5)
        rim = np.exp(-d / 5.0) * a
        top = smoothstep(1100, 400, np.arange(H, dtype=np.float32))[:, None]
        rgb += (rim * (0.5 + 0.5 * top))[..., None] * col('ff3020') * 1.1
    else:      # soft sky rim light on the right edge
        m8 = (a > 0.5).astype(np.uint8)
        d = cv2.distanceTransform(m8, cv2.DIST_L2, 5)
        gx = cv2.Sobel(blur(a, 3), cv2.CV_32F, 1, 0, ksize=5)
        rim = np.exp(-d / 4.0) * a * np.clip(-gx * 0.05, 0, 1)
        rgb += rim[..., None] * col('fff4e0') * 0.8
    rgb += grain(H, W, 12 + rage, 0.02)
    return np.clip(rgb, 0, 1), a

if __name__ == '__main__':
    for rage, name in ((False, 'girl_calm'), (True, 'girl_rage')):
        rgb, a = paint(rage)
        save_rgba(f'{OUT}/{name}.png', rgb, a)
    print('girl done')
