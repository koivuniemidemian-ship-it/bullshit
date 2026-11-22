"""Paints wet, glossy blood splatter and drip sprites from metaball height fields."""
import numpy as np, cv2, sys
from paintlib import *

OUT = sys.argv[1] if len(sys.argv) > 1 else '.'

def shade_liquid(field, thresh=0.5):
    a = smoothstep(thresh - 0.04, thresh + 0.04, field)
    h = np.clip(field - thresh, 0, None).astype(np.float32)
    h = blur(h, 1.5)
    gx = cv2.Sobel(h, cv2.CV_32F, 1, 0, ksize=3) * 6
    gy = cv2.Sobel(h, cv2.CV_32F, 0, 1, ksize=3) * 6
    n = np.dstack([-gx, -gy, np.ones_like(gx)]); n /= np.linalg.norm(n, axis=2, keepdims=True)
    L = np.array([-0.5, -0.7, 0.6], np.float32); L /= np.linalg.norm(L)
    lam = np.clip((n * L).sum(2), 0, 1)
    Hh = L + np.array([0, 0, 1], np.float32); Hh /= np.linalg.norm(Hh)
    spec = np.clip((n * Hh).sum(2), 0, 1) ** 28
    thick = smoothstep(0.0, 1.6, h)
    base = lerp(col('c8061c'), col('4a0006'), thick)          # thin = bright red, thick = deep
    c = base * (0.55 + 0.6 * lam[..., None])
    c += spec[..., None] * 0.9                                # wet highlight
    edge = np.exp(-h / 0.02) * a
    c = lerp(c, col('2a0004'), edge * 0.6)                    # dark meniscus at the rim
    return np.clip(c, 0, 1), a

def gauss(xx, yy, x, y, r):
    return np.exp(-((xx - x) ** 2 + (yy - y) ** 2) / (r * r))

def splat(seed, S=320):
    r = rng(seed)
    yy, xx = np.mgrid[0:S, 0:S].astype(np.float32)
    c = S / 2
    f = gauss(xx, yy, c, c, S * 0.17) * 1.1
    for _ in range(10):   # lobes
        a = r.uniform(0, 6.28); d = r.uniform(0.05, 0.16) * S
        f += gauss(xx, yy, c + np.cos(a) * d, c + np.sin(a) * d, S * r.uniform(0.05, 0.1)) * 0.8
    for _ in range(r.integers(6, 10)):   # streaks = chains of shrinking blobs
        a = r.uniform(0, 6.28); L = r.uniform(0.22, 0.45) * S
        for k in range(14):
            t = k / 13
            rad = S * (0.035 * (1 - t) + 0.008)
            f += gauss(xx, yy, c + np.cos(a) * L * t, c + np.sin(a) * L * t, rad) * 0.9
        f += gauss(xx, yy, c + np.cos(a) * (L + 8), c + np.sin(a) * (L + 8), S * 0.018) * 1.2   # droplet at the tip
    for _ in range(30):   # spray droplets
        a = r.uniform(0, 6.28); d = r.uniform(0.2, 0.47) * S
        f += gauss(xx, yy, c + np.cos(a) * d, c + np.sin(a) * d, S * r.uniform(0.006, 0.016)) * 1.3
    return shade_liquid(f)

def drip(seed, Wd=48, Hd=640):
    r = rng(seed)
    yy, xx = np.mgrid[0:Hd, 0:Wd].astype(np.float32)
    c = Wd / 2
    wob = np.interp(yy[:, 0], np.linspace(0, Hd, 12), r.normal(0, 1.2, 12))[:, None]
    w = (5.5 + 2.5 * np.sin(yy / 90 + seed)) * smoothstep(Hd, Hd * 0.2, yy) + 4
    f = np.exp(-((xx - c - wob) ** 2) / (w * w)) * 1.1
    f += gauss(xx, yy, c + wob[-30, 0], Hd - 26, 15) * 1.3      # heavy bulb at the bottom
    return shade_liquid(f)

if __name__ == '__main__':
    for i in range(6):
        rgb, a = splat(100 + i); save_rgba(f'{OUT}/splat{i}.png', rgb, a)
    for i in range(3):
        rgb, a = drip(200 + i); save_rgba(f'{OUT}/drip{i}.png', rgb, a)
    print('blood done')
