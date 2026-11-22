"""Paints the sky, clouds, mountains and blood moon."""
import numpy as np, cv2, sys
from paintlib import *

W, H = 1400, 1480
OUT = sys.argv[1] if len(sys.argv) > 1 else '.'

def shift(img, dx, dy):
    M = np.float32([[1, 0, dx], [0, 1, dy]])
    return cv2.warpAffine(img, M, (img.shape[1], img.shape[0]), borderMode=cv2.BORDER_REPLICATE)

def clouds(h, w, seed, cover, sun, scale=3.0, squash=2.2):
    """Cloud density stretched horizontally, lit toward `sun`."""
    n = fbm(h, int(w / squash), seed, octaves=7, base=scale, persistence=0.55)
    n = cv2.resize(n, (w, h), interpolation=cv2.INTER_CUBIC)
    detail = fbm(h, w, seed + 1, octaves=5, base=24, persistence=0.6)
    n = n * 0.85 + detail * 0.15
    d = smoothstep(1 - cover, 1 - cover + 0.22, n)
    dx, dy = sun
    toward = shift(blur(d, 5), -dx, -dy)
    light = np.clip(1.0 - np.clip(toward - d * 0.5, 0, 1) * 2.2, 0, 1)
    light = blur(light, 1.5)
    return d, light

def paint_sky(rage):
    if not rage:
        sky = vgrad(H, W, [(0.0, col('1d4fb8')), (0.35, col('4b8fe0')), (0.55, col('9cc6f2')), (0.66, col('f3e3d6')), (1.0, col('f6e7da'))])
        # sun glow upper right
        yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
        dsun = np.sqrt((xx - 1150) ** 2 + (yy - 170) ** 2)
        sky += (np.exp(-dsun / 260.0) * 0.55)[..., None] * col('fff2d0')
        sky += (np.exp(-dsun / 40.0) * 0.6)[..., None] * col('ffffff')
        # high wispy clouds
        d, l = clouds(H, W, 11, 0.3, (14, -10), scale=2.0, squash=5.0)
        d *= smoothstep(0.7, 0.2, np.linspace(0, 1, H)[:, None])
        c = lerp(col('b9c3e6'), col('ffffff'), l)
        sky = over(sky, c, d * 0.75)
        # big cumulus band
        d2, l2 = clouds(H, W, 23, 0.46, (16, -14), scale=3.0, squash=2.0)
        band = smoothstep(0.1, 0.25, np.linspace(0, 1, H))[:, None] * smoothstep(0.62, 0.42, np.linspace(0, 1, H))[:, None]
        d2 *= band
        c2 = lerp(col('8f9cc8'), col('fffdf8'), l2 ** 0.8)
        c2 = lerp(c2, col('ffe9d6'), smoothstep(0.3, 0.6, np.linspace(0, 1, H))[:, None] * 0.35)
        sky = over(sky, c2, np.clip(d2 * 1.1, 0, 1))
    else:
        sky = vgrad(H, W, [(0.0, col('050003')), (0.3, col('1c0008')), (0.5, col('5a0412')), (0.62, col('b0181a')), (0.7, col('e04a20')), (1.0, col('2a0006'))])
        r = rng(5)
        stars = np.zeros((H, W), np.float32)
        for _ in range(500):
            x, y = r.integers(0, W), r.integers(0, int(H * 0.45))
            stars[y, x] = r.random() * 0.9 + 0.1
        stars = blur(stars, 0.8) * 3
        sky += np.clip(stars, 0, 1)[..., None] * col('ffd8d8')
        d, l = clouds(H, W, 23, 0.4, (0, 18), scale=3.0, squash=2.0)
        band = smoothstep(0.05, 0.22, np.linspace(0, 1, H))[:, None] * smoothstep(0.66, 0.45, np.linspace(0, 1, H))[:, None]
        d *= band
        # moon is below/behind -> clouds lit from beneath with red
        c = lerp(col('0e0004'), col('ff3a24'), (1 - l) ** 1.5 * 0.9)
        c = lerp(c, col('2a020a'), 0.35)
        sky = over(sky, c, np.clip(d * 1.15, 0, 1))
    sky += grain(H, W, 3 if rage else 2, 0.025)
    return np.clip(sky, 0, 1)

def ridge_line(n, seed, base, amp, sharp):
    a = fbm1d(n, seed, octaves=7, base=2.5, persistence=0.55)
    ridged = 1 - np.abs(fbm1d(n, seed + 100, octaves=6, base=4, persistence=0.5) * 2 - 1)
    h = a * (1 - sharp) + ridged ** 2 * sharp
    return base - amp * h

def ridged2d(h, w, seed, base):
    n = fbm(h, w, seed, octaves=7, base=base, persistence=0.55)
    return 1 - np.abs(n * 2 - 1)

def paint_mountains(rage):
    rgb = np.zeros((H, W, 3), np.float32)
    alpha = np.zeros((H, W), np.float32)
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    layers = [
        # base, amp, sharp, rock colour, haze, rage colour
        (900, 360, 0.75, col('6f7fa8'), 0.62, col('2a0510')),
        (1010, 260, 0.6, col('4a5578'), 0.35, col('170208')),
        (1130, 150, 0.35, col('3c6e4a'), 0.08, col('0a0103')),
    ]
    light = np.array([0.6, -0.55, 0.58], np.float32)   # from the upper right
    light /= np.linalg.norm(light)
    for i, (base, amp, sharp, c, haze, rc) in enumerate(layers):
        top1 = ridge_line(W, 40 + i * 7, base, amp, sharp)
        top = top1[None, :]
        mask = smoothstep(top - 1.2, top + 1.2, yy)
        depth = np.clip((yy - top) / (amp + 150), 0, 1)
        # terrain height field: ridged rock flowing down from the ridge line
        terr = ridged2d(H, W, 60 + i, 9 + i * 3) * 0.6 + ridged2d(H, W, 80 + i, 30) * 0.25
        terr += (1 - depth) * 0.6
        gx = cv2.Sobel(terr, cv2.CV_32F, 1, 0, ksize=5) * (2.0 - i * 0.5)
        gy = cv2.Sobel(terr, cv2.CV_32F, 0, 1, ksize=5) * (2.0 - i * 0.5)
        nrm = np.dstack([-gx, -gy, np.ones_like(gx)])
        nrm /= np.linalg.norm(nrm, axis=2, keepdims=True)
        lam = np.clip((nrm * light).sum(axis=2), 0, 1)
        if not rage:
            if i < 2:
                rock = c * (0.55 + 0.75 * lam[..., None])
                snowiness = smoothstep(0.55, 0.75, (1 - depth) * 0.9 + terr * 0.35 - (1 - nrm[..., 2]) * 1.5)
                snow = lerp(col('9aa8d6'), col('ffffff'), lam ** 0.7)
                colr = lerp(rock, snow, snowiness)
            else:
                grass = fbm(H, W, 99, octaves=7, base=40, persistence=0.7)
                colr = lerp(col('2a5230'), col('86bb5e'), np.clip(grass * 0.7 + lam * 0.4, 0, 1))
                r = rng(42)
                for _ in range(700):
                    x = int(r.integers(0, W))
                    y = int(top1[x] + r.integers(12, 380))
                    if y < H:
                        fc = [col('ff9fd0'), col('fff17a'), col('ffffff'), col('b89cff')][r.integers(0, 4)]
                        cv2.circle(colr, (x, y), int(r.integers(2, 4)), fc.tolist(), -1, cv2.LINE_AA)
            colr = lerp(colr, col('bccdf0'), haze * (1 - depth * 0.5))
        else:
            colr = rc * (0.6 + 0.9 * lam[..., None])
            rim = np.exp(-np.clip(yy - top, 0, None) / 5.0) * mask
            colr += rim[..., None] * col('ff2a12') * 0.9
            colr = lerp(colr, col('3a0508'), haze * 0.4)
        rgb = over(rgb, colr, mask)
        alpha = np.maximum(alpha, mask)
    rgb += grain(H, W, 9, 0.02)
    return np.clip(rgb, 0, 1), alpha

def paint_moon():
    S = 720
    R = 230
    yy, xx = np.mgrid[0:S, 0:S].astype(np.float32)
    cx = cy = S / 2
    d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
    disc = smoothstep(R + 1.5, R - 1.5, d)
    # surface: maria + craters
    maria = fbm(S, S, 77, octaves=6, base=3)
    tex = fbm(S, S, 78, octaves=8, base=12, persistence=0.6)
    r = rng(79)
    crater = np.zeros((S, S), np.float32)
    for _ in range(60):
        x, y = r.uniform(cx - R, cx + R), r.uniform(cy - R, cy + R)
        rr = r.uniform(4, 34)
        ring = np.exp(-((np.sqrt((xx - x) ** 2 + (yy - y) ** 2) - rr) ** 2) / (rr * 0.25 + 1))
        bowl = np.exp(-((xx - x) ** 2 + (yy - y) ** 2) / (rr * rr * 0.6))
        crater += ring * 0.04 - bowl * 0.1
    limb = np.sqrt(np.clip(1 - (d / R) ** 2, 0, 1))
    bright = (0.55 + 0.25 * tex - 0.25 * smoothstep(0.45, 0.7, maria) + crater) * (0.45 + 0.55 * limb)
    c = lerp(col('5a0008'), col('ff6a3a'), np.clip(bright, 0, 1))
    c = lerp(c, col('ffb070'), np.clip(bright - 0.75, 0, 1) * 2)
    glow = np.exp(-np.maximum(d - R, 0) / 55.0) * 0.85 * (1 - disc) * smoothstep(S / 2, S / 2 - 60, d)
    rgb = c * disc[..., None] + col('ff2a10') * glow[..., None]
    a = np.clip(disc + glow, 0, 1)
    rgb = np.where(a[..., None] > 0, rgb / np.maximum(a[..., None], 1e-4), 0)
    return rgb, a

if __name__ == '__main__':
    save_rgba(f'{OUT}/sky_calm.png', paint_sky(False))
    save_rgba(f'{OUT}/sky_rage.png', paint_sky(True))
    m, a = paint_mountains(False); save_rgba(f'{OUT}/mountains_calm.png', m, a)
    m, a = paint_mountains(True);  save_rgba(f'{OUT}/mountains_rage.png', m, a)
    rgb, a = paint_moon(); save_rgba(f'{OUT}/moon.png', rgb, a)
    print('sky done')
