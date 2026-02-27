# GitHub Pages Deployment Review — PR #30

**Date:** 2026-02-18  
**Branch:** `fix/pages-workflow`  
**PR:** https://github.com/dlorp/second-movement/pull/30

## 🔴 VERDICT: Will NOT work as-is. One config change needed.

### The Problem

The workflow in PR #30 uses `peaceiris/actions-gh-pages@v3`, which deploys by pushing content to the `gh-pages` branch. **However, the repo's GitHub Pages settings are configured for `build_type: workflow`**, which expects the newer `actions/upload-pages-artifact` + `actions/deploy-pages` Actions deployment pipeline — it ignores the `gh-pages` branch entirely.

Current Pages config (from API):
```
build_type: workflow
source.branch: main
source.path: /
```

This mismatch is why you're getting 404s. The `gh-pages` branch already has valid content (companion-app/index.html, builder/index.html, etc.) from a previous deploy, but Pages isn't serving from that branch.

### Fix: Option A (Simplest — change repo settings)

Change Pages source from "GitHub Actions" to "Deploy from a branch":

```bash
gh api repos/dlorp/second-movement/pages -X PUT \
  -f build_type=legacy \
  -f source[branch]=gh-pages \
  -f source[path]=/
```

Then the existing workflow in PR #30 will work as-is.

### Fix: Option B (Change the workflow instead)

Rewrite the workflow to use the newer Actions deployment method:

```yaml
name: GitHub Pages

on:
  push:
    branches: [main]

permissions:
  contents: read
  pages: write
  id-token: write

concurrency:
  group: "pages"
  cancel-in-progress: false

jobs:
  deploy:
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Build deploy directory
        run: |
          mkdir -p _deploy
          cp -r companion-app/ _deploy/companion-app/
          cp -r builder/ _deploy/builder/
          printf '<!DOCTYPE html><html><head><title>second-movement</title><meta http-equiv="refresh" content="0; url=companion-app/"></head><body><p><a href="companion-app/">Companion App</a> | <a href="builder/">Firmware Builder</a></p></body></html>' > _deploy/index.html
          touch _deploy/.nojekyll
      - uses: actions/configure-pages@v4
      - uses: actions/upload-pages-artifact@v3
        with:
          path: _deploy/
      - id: deployment
        uses: actions/deploy-pages@v4
```

### Other Findings

| Check | Status |
|-------|--------|
| `companion-app/` exists on main | ✅ |
| `builder/` exists on main | ✅ |
| `companion-app/index.html` exists | ✅ |
| `builder/index.html` exists | ✅ |
| `gh-pages` branch has content | ✅ (from prior deploy) |
| `.nojekyll` on gh-pages | ✅ (already present) |
| Workflow `cp -r` commands will work | ✅ |
| `peaceiris/actions-gh-pages@v3` config correct | ✅ |
| **Pages source matches deploy method** | **❌ MISMATCH** |

### Recommendation

**Option A** is the quickest fix — just run the `gh api` command above to switch Pages to serve from the `gh-pages` branch. No code changes needed to the PR.

**Option B** is more "modern" and doesn't require changing repo settings, but requires updating the workflow file in the PR.

Either way, the workflow logic (copy dirs, create index.html) is correct. The only issue is the Pages source configuration mismatch.
