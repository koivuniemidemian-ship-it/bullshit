"""Small procedural painting toolkit: noise, blending, lighting."""
import numpy as np
import cv2

def rng(seed):
    return np.random.default_rng(seed)

def fbm(h, w, seed, octaves=6, base=4, persistence=0.5, lacunarity=2.0):
    """Fractal value noise in [0,1], shape (h,w)."""
    r = rng(seed)
    out = np.zeros((h, w), np.float32)
    amp, total = 1.0, 0.0
    freq = base
    for _ in range(octaves):
        gh, gw = max(2, int(h * freq / max(h, w)) + 2), max(2, int(w * freq / max(h, w)) + 2)
        grid = r.random((gh, gw)).astype(np.float32)
        layer = cv2.resize(grid, (w, h), interpolation=cv2.INTER_CUBIC)
        out += layer * amp
        total += amp
        amp *= persistence
        freq *= lacunarity
    out /= total
    out -= out.min(); out /= max(out.max(), 1e-6)
    return out

def fbm1d(n, seed, octaves=6, base=3, persistence=0.55):
    return fbm(1, n, seed, octaves, base, persistence)[0]

def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)

def lerp(a, b, t):
    t = np.asarray(t, np.float32)
    if t.ndim == 2 and np.ndim(a) >= 1 and np.shape(a)[-1] == 3:
        t = t[..., None]
    return a + (b - a) * t

def col(hexstr):
    hexstr = hexstr.lstrip('#')
    return np.array([int(hexstr[i:i+2], 16) / 255.0 for i in (0, 2, 4)], np.float32)

def vgrad(h, w, stops):
    """Vertical gradient from list of (pos, colour)."""
    ys = np.linspace(0, 1, h, dtype=np.float32)
    out = np.zeros((h, 3), np.float32)
    for c in range(3):
        out[:, c] = np.interp(ys, [p for p, _ in stops], [cc[c] for _, cc in stops])
    return np.repeat(out[:, None, :], w, axis=1)

def blur(img, sigma):
    if sigma <= 0: return img
    k = int(sigma * 3) * 2 + 1
    return cv2.GaussianBlur(img, (k, k), sigma)

def save_rgba(path, rgb, alpha=None):
    rgb = np.clip(rgb, 0, 1)
    if alpha is None:
        alpha = np.ones(rgb.shape[:2], np.float32)
    a = np.clip(alpha, 0, 1)
    out = np.dstack([rgb[..., 2], rgb[..., 1], rgb[..., 0], a])  # BGRA for cv2
    cv2.imwrite(path, (out * 255 + 0.5).astype(np.uint8))

def over(dst_rgb, src_rgb, src_a):
    a = src_a[..., None]
    return dst_rgb * (1 - a) + src_rgb * a

def grain(h, w, seed, amount=0.02):
    return (rng(seed).random((h, w, 1)).astype(np.float32) - 0.5) * amount
