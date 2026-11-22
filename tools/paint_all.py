"""Regenerates every painted asset into ../art (run: python3 paint_all.py)."""
import os, subprocess, sys
here = os.path.dirname(os.path.abspath(__file__))
out = os.path.join(here, '..', 'art')
os.makedirs(out, exist_ok=True)
for script in ('paint_sky.py', 'paint_girl.py', 'paint_blood.py'):
    subprocess.check_call([sys.executable, os.path.join(here, script), out], cwd=here)
