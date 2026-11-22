# Custom character art (optional)

Put PNG files here and the plugin draws them in place of the code-drawn girl:

| File | Shown when |
|---|---|
| `girl_calm.png` | knob low: crossfades out as you turn it up |
| `girl_rage.png` | knob high: crossfades in (optional; without it the calm image is tinted red) |

- Transparent background, portrait orientation. The character area is 440 × 740 px, so 880 × 1480 looks sharp on Retina.
- The sky, mountains, blood moon and all the blood effects still draw around and over the image.
- Use only art you made or commissioned and have the rights to. Don't use screenshots of existing anime characters.

Commit the files and push. GitHub Actions rebuilds the .pkg with them embedded.
