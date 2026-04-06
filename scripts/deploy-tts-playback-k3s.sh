#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]:-$0}")" && pwd)
REPO_ROOT=$(cd -- "$SCRIPT_DIR/.." && pwd)
IMAGE_NAME="${TTS_PLAYBACK_IMAGE_NAME:-tts-playback-service}"
IMAGE_TAG="${TTS_PLAYBACK_IMAGE_TAG:-falcon-local}"
FULL_IMAGE="${IMAGE_NAME}:${IMAGE_TAG}"
K3S_NAMESPACE="${TTS_PLAYBACK_NAMESPACE:-tts-production}"
K3S_DEPLOYMENT="${TTS_PLAYBACK_DEPLOYMENT:-tts-playback}"
K3S_CONTAINER="${TTS_PLAYBACK_CONTAINER:-tts-service}"

log_step() {
    printf '\n==> %s\n' "$1"
}

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        printf 'Required command not found: %s\n' "$1" >&2
        exit 1
    fi
}

require_command docker
require_command sudo

cd "$REPO_ROOT"

log_step "Building playback image ${FULL_IMAGE}"
docker build -t "$FULL_IMAGE" .

log_step "Importing image into k3s"
docker save "$FULL_IMAGE" | sudo -n k3s ctr images import -

log_step "Applying Kubernetes manifests"
sudo -n k3s kubectl apply -f k8s/

log_step "Pinning deployment image"
sudo -n k3s kubectl set image "deployment/${K3S_DEPLOYMENT}" "${K3S_CONTAINER}=${FULL_IMAGE}" -n "$K3S_NAMESPACE"

log_step "Restarting deployment"
sudo -n k3s kubectl rollout restart "deployment/${K3S_DEPLOYMENT}" -n "$K3S_NAMESPACE"

log_step "Waiting for rollout"
sudo -n k3s kubectl rollout status "deployment/${K3S_DEPLOYMENT}" -n "$K3S_NAMESPACE" --timeout=300s

log_step "Deployment status"
sudo -n k3s kubectl get pods -n "$K3S_NAMESPACE" -l app="$K3S_DEPLOYMENT"
sudo -n k3s kubectl get svc -n "$K3S_NAMESPACE"

printf '\nK3s deployment updated with image %s\n' "$FULL_IMAGE"
