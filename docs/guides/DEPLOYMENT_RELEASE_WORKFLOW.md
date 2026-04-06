# Deployment Release Workflow

This service deploys to Falcon K3s in `tts-production`.

## Falcon K3s Deploy

Run from the Falcon checkout:

```bash
./scripts/deploy-tts-playback-k3s.sh
```

Useful overrides:

```bash
TTS_PLAYBACK_IMAGE_TAG=gh-12345 ./scripts/deploy-tts-playback-k3s.sh
TTS_PLAYBACK_NAMESPACE=tts-production ./scripts/deploy-tts-playback-k3s.sh
```

The script builds the Docker image on Falcon, imports it into the K3s container runtime, reapplies `k8s/`, updates the deployment image explicitly, and waits for rollout.

## GitHub Actions Deploy Workflow

Manual workflow:

- `.github/workflows/deploy-k3s.yml`

Required GitHub secrets:

- `FALCON_SSH_HOST`
- `FALCON_SSH_USER`
- `FALCON_SSH_PRIVATE_KEY`

Optional GitHub variable:

- `FALCON_SSH_PORT`

The workflow syncs the checked-out repository contents into the Falcon repo path you choose and then runs the checked-in deploy script there.

## Release Order

1. Validate the contract and deployment-tool tests.
2. Cut the release or choose the commit to deploy.
3. Run the manual K3s deploy workflow or the Falcon-side script.
4. Wait for rollout in `tts-production`.
5. Smoke-test `POST /api/tts/play` and `/health` against the live service.

## Operational Notes

- The live target is the K3s deployment `tts-production/tts-playback`.
- A built Docker image is not enough. The deployment is only live after `kubectl rollout status` succeeds.
- If you use GitHub Actions, point the workflow at a Falcon checkout that is safe to overwrite with the contents of the selected commit.
