# Kubernetes Manifests

## TTS Playback Service - Kubernetes Deployment

This directory contains Kubernetes manifest files for deploying the TTS Playback Service.

---

## Files Overview

| File | Description |
|------|-------------|
| `namespace.yaml` | Namespace definition |
| `configmap.yaml` | Configuration settings |
| `secret.yaml` | Sensitive credentials (passwords) |
| `deployment.yaml` | TTS service deployment |
| `service.yaml` | Service definition for TTS |
| `ingress.yaml` | Ingress for external access (optional) |
| `redis-statefulset.yaml` | Redis cache StatefulSet |
| `rabbitmq-statefulset.yaml` | RabbitMQ queue StatefulSet |
| `network-policy.yaml` | Network security policies |

---

## Quick Deployment

### Prerequisites

- Kubernetes 1.24+
- kubectl configured
- StorageClass available for PVCs

### Deploy All Resources

```bash
# Apply all manifests
kubectl apply -f k8s/

# Verify deployment
kubectl get all -n tts-production
```

### Deploy Step-by-Step

```bash
# 1. Create namespace
kubectl apply -f k8s/namespace.yaml

# 2. Create ConfigMap and Secret
kubectl apply -f k8s/configmap.yaml
kubectl apply -f k8s/secret.yaml

# 3. Deploy Redis
kubectl apply -f k8s/redis-statefulset.yaml

# 4. Deploy RabbitMQ
kubectl apply -f k8s/rabbitmq-statefulset.yaml

# 5. Deploy TTS Service
kubectl apply -f k8s/deployment.yaml
kubectl apply -f k8s/service.yaml

# 6. (Optional) Create Ingress
kubectl apply -f k8s/ingress.yaml

# 7. Apply Network Policy
kubectl apply -f k8s/network-policy.yaml
```

---

## Configuration

### Update ConfigMap

Edit `configmap.yaml` to change configuration:

```yaml
data:
  CACHE_SIZE: "200"  # Increase cache size
  LOG_LEVEL: "1"     # Debug logging
```

Apply changes:
```bash
kubectl apply -f k8s/configmap.yaml
kubectl rollout restart deployment/tts-playback -n tts-production
```

### Update Secrets

```bash
# Update secret
kubectl create secret generic tts-secrets \
  --from-literal=RABBITMQ_PASSWORD='new-password' \
  --from-literal=REDIS_PASSWORD='new-password' \
  -n tts-production \
  --dry-run=client -o yaml | kubectl apply -f -

# Restart deployment
kubectl rollout restart deployment/tts-playback -n tts-production
```

---

## Accessing Services

### Port Forward (Local Access)

```bash
# TTS Service
kubectl port-forward svc/tts-playback-service 8080:8080 -n tts-production

# RabbitMQ Management
kubectl port-forward svc/rabbitmq-service 15672:15672 -n tts-production

# Redis
kubectl port-forward svc/redis-service 6379:6379 -n tts-production
```

### Ingress (External Access)

Update `ingress.yaml` with your domain:
```yaml
rules:
- host: your-domain.com
```

---

## Monitoring

### Check Pod Status

```bash
kubectl get pods -n tts-production
kubectl describe pod <pod-name> -n tts-production
kubectl logs -f <pod-name> -n tts-production
```

### Check Services

```bash
kubectl get svc -n tts-production
kubectl describe svc tts-playback-service -n tts-production
```

### Check PersistentVolumeClaims

```bash
kubectl get pvc -n tts-production
```

---

## Scaling

### Vertical Scaling (Resources)

Edit `deployment.yaml`:
```yaml
resources:
  requests:
    cpu: 1000m
    memory: 512Mi
  limits:
    cpu: 2000m
    memory: 1Gi
```

Apply:
```bash
kubectl apply -f k8s/deployment.yaml
```

### Note on Horizontal Scaling

**Current limitation**: Service requires single audio device access.
Multiple replicas would require separate audio devices per pod.

---

## Cleanup

### Delete All Resources

```bash
kubectl delete -f k8s/
```

### Delete Namespace (Complete Cleanup)

```bash
kubectl delete namespace tts-production
```

**Warning**: This deletes all resources and data!

---

## Troubleshooting

### Pod Not Starting

```bash
# Check events
kubectl describe pod <pod-name> -n tts-production

# Check logs
kubectl logs <pod-name> -n tts-production

# Check image pull
kubectl get events -n tts-production
```

### Service Not Accessible

```bash
# Check service endpoints
kubectl get endpoints -n tts-production

# Check network policy
kubectl describe networkpolicy -n tts-production
```

### PVC Stuck Pending

```bash
# Check PVC status
kubectl describe pvc <pvc-name> -n tts-production

# List available storage classes
kubectl get storageclass
```

---

## Security Notes

1. **Update Secrets**: Change default passwords in `secret.yaml`
2. **Network Policy**: Review and adjust `network-policy.yaml`
3. **RBAC**: Add RBAC policies for production
4. **TLS**: Configure TLS in `ingress.yaml`

---

## See Also

- [Infrastructure Guide](../docs/guides/INFRASTRUCTURE.md)
- [Deployment Architecture](../docs/technical/ARCHITECTURE.md#deployment-architecture)
- [Kubernetes Documentation](https://kubernetes.io/docs/)
