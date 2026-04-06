# Deployment Guardrails

This repo now includes `deployment-contract.json` to document where the playback service actually lives and which deployment-facing files are expected to move together.

## Covered Targets

- Falcon K3s deployment in `tts-production`
- Docker Desktop replica path for Windows-side development/testing
- HTTP API boundary used by `mercury-tts`
- dotmatrix spool boundary used by `hal-display`
- RabbitMQ and Redis runtime dependencies

## Local Validation

Run:

```bash
python3 scripts/validate_deployment_contract.py
```

The validator checks:

- contract structure
- referenced repo paths
- changed-file guardrails when CI provides a base revision

## Operator Workflow

When you change K3s manifests, Docker packaging, or the host-mounted spool contract:

1. Update the source files.
2. Update `deployment-contract.json` if the cluster target, environment role, integration point, or companion files changed.
3. Run the validator locally.
4. Make sure the cross-repo consumer or producer still matches the contract.

Examples:

- If you change `/api/tts/play`, revisit the `mercury-tts` caller assumptions.
- If you change `/tmp/halo-dotmatrix`, revisit `hal-display` and the host-path mount assumptions.
- If you change K3s namespace or service naming, update the contract so reviews can catch runtime drift.

## Why The Contract Matters Here

This service sits at a boundary between generation, playback, caching, queueing, and display signaling.
Small local code changes can break a remote runtime path on Falcon or a Windows/Docker Desktop replica assumption.
The contract makes those dependencies explicit enough to review.

## Release Bundles

This repo now includes `scripts/build_release_bundle.py` and a manual or tag-driven workflow at `.github/workflows/release-bundles.yml`.

Build a deterministic source bundle locally with:

```bash
python3 scripts/build_release_bundle.py
```
