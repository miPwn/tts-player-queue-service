# Release Checklist

Use this document for tagged releases and repository hygiene. The old top-level release checklist has been moved here so the maintained documentation lives under `docs/`.

## Pre-release

- verify README and docs links
- run the test suite
- confirm Docker build still succeeds
- confirm Kubernetes manifests still match runtime expectations
- review open issues and release notes

## Git Steps

```bash
git add .
git commit -m "release: prepare vX.Y.Z"
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push origin main --tags
```

## Repository Checks

- branch protections still enforce required checks
- GitHub Actions permissions are correct
- security scanning is still enabled
- required secrets are present for any publish steps

## Post-release

- verify CI completed successfully for the tag
- publish release notes
- confirm badges and docs reflect the new version
- smoke-test the container image and API health endpoint
