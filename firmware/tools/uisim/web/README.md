# ROZA Web Screen Viewer

This is a lightweight, installable web gallery for the existing UI simulator. It does not duplicate firmware rendering: the GitHub Actions job runs `tools/uisim/build.sh`, which compiles the real drawing sources from `firmware/src/` and exports the PNG captures.

## Publish on GitHub Pages

1. In the repository, open **Settings → Pages**.
2. Under **Build and deployment → Source**, select **GitHub Actions**.
3. Push changes to `main` (or manually run **ROZA Web Screen Viewer** in Actions).

The workflow installs the pinned PlatformIO dependencies and SDL2, renders the screens, assembles the PWA and deploys it. The resulting URL is:

`https://yoyote84.github.io/Roza/`

The page can be installed from the Android browser using **Add to Home screen**. It is a screen viewer: it shows saved simulator captures and provides touch navigation/filtering; it does not emulate the Cardputer keyboard or live firmware execution.

## Local site assembly

After running the existing simulator and generating `tools/uisim/out/*.png`:

```sh
cd firmware/tools/uisim
./web/build-site.sh ./site
```

The generated static site is written to `site/`. No real API keys are required; CI supplies a simulator-only placeholder for the N2YO page and all network responses remain mocked by the existing simulator.
