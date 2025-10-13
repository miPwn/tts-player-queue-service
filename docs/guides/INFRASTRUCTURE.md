# Infrastructure & Deployment Guide

## TTS Playback Service

**Version:** 1.0.0  
**Last Updated:** October 13, 2025

---

## Table of Contents

1. [Overview](#overview)
2. [Docker Deployment](#docker-deployment)
3. [Kubernetes Deployment](#kubernetes-deployment)
4. [Configuration Management](#configuration-management)
5. [Monitoring & Observability](#monitoring--observability)
6. [Scaling Strategy](#scaling-strategy)
7. [Backup & Recovery](#backup--recovery)
8. [Security Hardening](#security-hardening)
9. [Performance Tuning](#performance-tuning)
10. [Troubleshooting](#troubleshooting)

---

## Overview

### Deployment Architecture

The TTS Playback Service is designed for containerized deployment with the following infrastructure requirements:

| Component | Purpose | Persistence | Scaling |
|-----------|---------|-------------|---------|
| TTS Service | Audio playback | Stateless | Vertical only* |
| Redis | WAV caching | In-memory | Vertical |
| RabbitMQ | Job queuing | Disk-backed | Vertical/Cluster |
| PulseAudio | Audio output | None | N/A |

\* *Due to single audio device constraint*

### Infrastructure Requirements

**Minimum**:
- CPU: 1 core
- Memory: 512 MB
- Storage: 10 GB
- Network: 100 Mbps

**Recommended**:
- CPU: 2 cores
- Memory: 1 GB
- Storage: 20 GB
- Network: 1 Gbps

---

## Docker Deployment

### Single Container Deployment

```bash
# Build image
docker build -t tts-playback-service:1.0.0 .

# Run container
docker run -d \
  --name tts-playback \
  -p 8080:8080 \
  -e RABBITMQ_HOST=rabbitmq \
  -e REDIS_HOST=redis \
  -e CACHE_SIZE=100 \
  --device /dev/snd:/dev/snd \
  tts-playback-service:1.0.0
```

### Docker Compose Deployment

**File: `docker-compose.yml`**

```yaml
version: '3.8'

services:
  rabbitmq:
    image: rabbitmq:3.12-management-alpine
    ports:
      - "5672:5672"
      - "15672:15672"
    environment:
      RABBITMQ_DEFAULT_USER: guest
      RABBITMQ_DEFAULT_PASS: guest
    volumes:
      - rabbitmq_data:/var/lib/rabbitmq
    healthcheck:
      test: rabbitmq-diagnostics -q ping
      interval: 10s
      timeout: 5s
      retries: 5

  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"
    command: redis-server --save 60 1 --loglevel warning --maxmemory 256mb --maxmemory-policy allkeys-lru
    volumes:
      - redis_data:/data
    healthcheck:
      test: ["CMD", "redis-cli", "ping"]
      interval: 10s
      timeout: 5s
      retries: 5

  tts-playback:
    build: .
    ports:
      - "8080:8080"
    environment:
      RABBITMQ_HOST: rabbitmq
      RABBITMQ_PORT: 5672
      RABBITMQ_USER: guest
      RABBITMQ_PASSWORD: guest
      REDIS_HOST: redis
      REDIS_PORT: 6379
      CACHE_SIZE: 100
      API_HOST: 0.0.0.0
      API_PORT: 8080
      LOG_LEVEL: 2
    depends_on:
      rabbitmq:
        condition: service_healthy
      redis:
        condition: service_healthy
    devices:
      - /dev/snd:/dev/snd
    volumes:
      - /run/user/1000/pulse:/run/user/1000/pulse:ro
    privileged: true
    restart: unless-stopped

volumes:
  rabbitmq_data:
  redis_data:
```

**Commands**:

```bash
# Start all services
docker-compose up -d

# View logs
docker-compose logs -f tts-playback

# Stop services
docker-compose down

# Remove volumes (clean slate)
docker-compose down -v
```

### Multi-Stage Build Optimization

The Dockerfile uses multi-stage builds to minimize image size:

**Size Comparison**:
- Build stage: ~2 GB (with build tools)
- Final image: ~200 MB (runtime only)

**Stages**:
1. **Builder**: Compile dependencies and application
2. **Runtime**: Copy binaries and minimal dependencies

---

## Kubernetes Deployment

### Prerequisites

- Kubernetes 1.24+
- kubectl configured
- Helm 3+ (optional)

### Namespace Setup

```bash
# Create namespace
kubectl create namespace tts-production

# Set as default
kubectl config set-context --current --namespace=tts-production
```

### ConfigMap

**File: `k8s/configmap.yaml`**

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: tts-config
  namespace: tts-production
data:
  RABBITMQ_HOST: "rabbitmq-service"
  RABBITMQ_PORT: "5672"
  RABBITMQ_USER: "guest"
  RABBITMQ_VHOST: "/"
  RABBITMQ_QUEUE: "tts_playback_queue"
  REDIS_HOST: "redis-service"
  REDIS_PORT: "6379"
  REDIS_PASSWORD: ""
  CACHE_SIZE: "100"
  API_HOST: "0.0.0.0"
  API_PORT: "8080"
  PULSEAUDIO_SINK: ""
  LOG_LEVEL: "2"
```

### Secret

**File: `k8s/secret.yaml`**

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: tts-secrets
  namespace: tts-production
type: Opaque
stringData:
  RABBITMQ_PASSWORD: "your-secure-password"
  REDIS_PASSWORD: "your-secure-password"
```

```bash
# Apply secret
kubectl apply -f k8s/secret.yaml
```

### Deployment

**File: `k8s/deployment.yaml`**

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: tts-playback
  namespace: tts-production
  labels:
    app: tts-playback
    version: "1.0.0"
spec:
  replicas: 1  # Single instance (audio device constraint)
  selector:
    matchLabels:
      app: tts-playback
  template:
    metadata:
      labels:
        app: tts-playback
        version: "1.0.0"
    spec:
      containers:
      - name: tts-service
        image: tts-playback-service:1.0.0
        imagePullPolicy: IfNotPresent
        ports:
        - containerPort: 8080
          name: http
          protocol: TCP
        envFrom:
        - configMapRef:
            name: tts-config
        - secretRef:
            name: tts-secrets
        resources:
          requests:
            cpu: 500m
            memory: 256Mi
          limits:
            cpu: 1000m
            memory: 512Mi
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 3
        readinessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 2
        volumeMounts:
        - name: audio-device
          mountPath: /dev/snd
      volumes:
      - name: audio-device
        hostPath:
          path: /dev/snd
          type: Directory
      securityContext:
        runAsNonRoot: false
        privileged: true  # Required for audio device access
```

### Service

**File: `k8s/service.yaml`**

```yaml
apiVersion: v1
kind: Service
metadata:
  name: tts-playback-service
  namespace: tts-production
  labels:
    app: tts-playback
spec:
  type: ClusterIP
  selector:
    app: tts-playback
  ports:
  - port: 8080
    targetPort: 8080
    protocol: TCP
    name: http
```

### Ingress (Optional)

**File: `k8s/ingress.yaml`**

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: tts-playback-ingress
  namespace: tts-production
  annotations:
    cert-manager.io/cluster-issuer: "letsencrypt-prod"
    nginx.ingress.kubernetes.io/proxy-body-size: "10m"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - tts.example.com
    secretName: tts-tls-secret
  rules:
  - host: tts.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: tts-playback-service
            port:
              number: 8080
```

### Deploy to Kubernetes

```bash
# Apply all resources
kubectl apply -f k8s/

# Verify deployment
kubectl get pods -n tts-production
kubectl get svc -n tts-production

# Check logs
kubectl logs -f deployment/tts-playback -n tts-production

# Port forward for testing
kubectl port-forward svc/tts-playback-service 8080:8080 -n tts-production
```

### Redis StatefulSet

**File: `k8s/redis-statefulset.yaml`**

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: redis
  namespace: tts-production
spec:
  serviceName: redis-service
  replicas: 1
  selector:
    matchLabels:
      app: redis
  template:
    metadata:
      labels:
        app: redis
    spec:
      containers:
      - name: redis
        image: redis:7-alpine
        ports:
        - containerPort: 6379
          name: redis
        command:
        - redis-server
        - --save
        - "60"
        - "1"
        - --loglevel
        - warning
        - --maxmemory
        - 256mb
        - --maxmemory-policy
        - allkeys-lru
        volumeMounts:
        - name: redis-data
          mountPath: /data
        resources:
          requests:
            cpu: 100m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 512Mi
  volumeClaimTemplates:
  - metadata:
      name: redis-data
    spec:
      accessModes: [ "ReadWriteOnce" ]
      resources:
        requests:
          storage: 10Gi
---
apiVersion: v1
kind: Service
metadata:
  name: redis-service
  namespace: tts-production
spec:
  selector:
    app: redis
  ports:
  - port: 6379
    targetPort: 6379
  clusterIP: None
```

### RabbitMQ StatefulSet

**File: `k8s/rabbitmq-statefulset.yaml`**

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: rabbitmq
  namespace: tts-production
spec:
  serviceName: rabbitmq-service
  replicas: 1
  selector:
    matchLabels:
      app: rabbitmq
  template:
    metadata:
      labels:
        app: rabbitmq
    spec:
      containers:
      - name: rabbitmq
        image: rabbitmq:3.12-management-alpine
        ports:
        - containerPort: 5672
          name: amqp
        - containerPort: 15672
          name: management
        env:
        - name: RABBITMQ_DEFAULT_USER
          value: guest
        - name: RABBITMQ_DEFAULT_PASS
          valueFrom:
            secretKeyRef:
              name: tts-secrets
              key: RABBITMQ_PASSWORD
        volumeMounts:
        - name: rabbitmq-data
          mountPath: /var/lib/rabbitmq
        resources:
          requests:
            cpu: 250m
            memory: 256Mi
          limits:
            cpu: 1000m
            memory: 512Mi
  volumeClaimTemplates:
  - metadata:
      name: rabbitmq-data
    spec:
      accessModes: [ "ReadWriteOnce" ]
      resources:
        requests:
          storage: 10Gi
---
apiVersion: v1
kind: Service
metadata:
  name: rabbitmq-service
  namespace: tts-production
spec:
  selector:
    app: rabbitmq
  ports:
  - port: 5672
    targetPort: 5672
    name: amqp
  - port: 15672
    targetPort: 15672
    name: management
```

---

## Configuration Management

### Environment-Based Configuration

**Development** (`.env.dev`):
```bash
RABBITMQ_HOST=localhost
REDIS_HOST=localhost
CACHE_SIZE=10
LOG_LEVEL=1  # Debug
```

**Staging** (`.env.staging`):
```bash
RABBITMQ_HOST=rabbitmq-staging
REDIS_HOST=redis-staging
CACHE_SIZE=50
LOG_LEVEL=2  # Info
```

**Production** (`.env.prod`):
```bash
RABBITMQ_HOST=rabbitmq-prod
REDIS_HOST=redis-prod
CACHE_SIZE=100
LOG_LEVEL=3  # Warn
```

### Secrets Management

**Kubernetes Secrets**:
```bash
# Create from literal
kubectl create secret generic tts-secrets \
  --from-literal=RABBITMQ_PASSWORD='secret' \
  --from-literal=REDIS_PASSWORD='secret'

# Create from file
kubectl create secret generic tts-secrets \
  --from-env-file=.env.prod.secret
```

**External Secrets Operator** (Recommended):
```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: tts-secrets
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: aws-secrets-manager
    kind: ClusterSecretStore
  target:
    name: tts-secrets
  data:
  - secretKey: RABBITMQ_PASSWORD
    remoteRef:
      key: prod/tts/rabbitmq-password
  - secretKey: REDIS_PASSWORD
    remoteRef:
      key: prod/tts/redis-password
```

---

## Monitoring & Observability

### Prometheus Metrics (Future Enhancement)

```yaml
# ServiceMonitor for Prometheus Operator
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: tts-playback
  namespace: tts-production
spec:
  selector:
    matchLabels:
      app: tts-playback
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
```

### Logging Stack

**Fluentd DaemonSet** for log collection:

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluentd-config
data:
  fluent.conf: |
    <source>
      @type tail
      path /var/log/containers/tts-playback*.log
      pos_file /var/log/tts-playback.log.pos
      tag tts.logs
      <parse>
        @type json
      </parse>
    </source>
    
    <match tts.logs>
      @type elasticsearch
      host elasticsearch-service
      port 9200
      logstash_format true
      logstash_prefix tts
    </match>
```

### Health Checks

**Liveness Probe**: Ensures container is running
```yaml
livenessProbe:
  httpGet:
    path: /health
    port: 8080
  initialDelaySeconds: 10
  periodSeconds: 10
  failureThreshold: 3
```

**Readiness Probe**: Ensures container is ready for traffic
```yaml
readinessProbe:
  httpGet:
    path: /health
    port: 8080
  initialDelaySeconds: 5
  periodSeconds: 5
  failureThreshold: 2
```

---

## Scaling Strategy

### Current Limitation

**Single Audio Device**: Only one service instance can access the audio device.

### Scaling Options

#### Option 1: Vertical Scaling (Current)
- Increase CPU/memory for single instance
- Handle burst via RabbitMQ queue
- Limited by single device throughput

#### Option 2: Multi-Zone Deployment (Future)
```
Zone A: tts-playback-a → Audio Device A
Zone B: tts-playback-b → Audio Device B
Zone C: tts-playback-c → Audio Device C

Route requests by zone/location
```

#### Option 3: Network Audio (Future)
- Use PulseAudio network streaming
- Multiple services → single audio server
- Higher latency, but scalable

---

## Backup & Recovery

### Redis Backup

**RDB Snapshots**:
```bash
# Trigger manual snapshot
redis-cli BGSAVE

# Configure automatic snapshots
# In redis.conf:
save 900 1      # After 900s if 1 key changed
save 300 10     # After 300s if 10 keys changed
save 60 10000   # After 60s if 10000 keys changed
```

**Backup Script**:
```bash
#!/bin/bash
DATE=$(date +%Y%m%d_%H%M%S)
kubectl exec -n tts-production redis-0 -- redis-cli BGSAVE
kubectl cp tts-production/redis-0:/data/dump.rdb ./backups/redis-$DATE.rdb
```

### RabbitMQ Backup

```bash
# Export definitions
kubectl exec -n tts-production rabbitmq-0 -- \
  rabbitmqadmin export /tmp/backup.json

# Copy backup
kubectl cp tts-production/rabbitmq-0:/tmp/backup.json \
  ./backups/rabbitmq-definitions-$DATE.json
```

### Disaster Recovery

**Recovery Steps**:
1. Restore Redis from snapshot
2. Restore RabbitMQ definitions
3. Redeploy application
4. Verify health checks

**RTO**: < 15 minutes  
**RPO**: Last backup (15 min intervals recommended)

---

## Security Hardening

### Network Policies

**File: `k8s/network-policy.yaml`**

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tts-network-policy
  namespace: tts-production
spec:
  podSelector:
    matchLabels:
      app: tts-playback
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: nginx-ingress
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: redis
    ports:
    - protocol: TCP
      port: 6379
  - to:
    - podSelector:
        matchLabels:
          app: rabbitmq
    ports:
    - protocol: TCP
      port: 5672
```

### Pod Security Policy

```yaml
apiVersion: policy/v1beta1
kind: PodSecurityPolicy
metadata:
  name: tts-psp
spec:
  privileged: true  # Required for audio device
  allowPrivilegeEscalation: true
  volumes:
  - 'hostPath'
  - 'configMap'
  - 'secret'
  runAsUser:
    rule: 'RunAsAny'
  seLinux:
    rule: 'RunAsAny'
  fsGroup:
    rule: 'RunAsAny'
```

### TLS/SSL

**Ingress TLS**:
```yaml
spec:
  tls:
  - hosts:
    - tts.example.com
    secretName: tts-tls-secret
```

**Generate Certificate**:
```bash
# Using cert-manager
kubectl apply -f - <<EOF
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: tts-cert
  namespace: tts-production
spec:
  secretName: tts-tls-secret
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
  - tts.example.com
EOF
```

---

## Performance Tuning

### Redis Optimization

```conf
# redis.conf
maxmemory 512mb
maxmemory-policy allkeys-lru
save ""  # Disable RDB for pure cache
appendonly no  # Disable AOF for performance
```

### RabbitMQ Optimization

```conf
# rabbitmq.conf
vm_memory_high_watermark.relative = 0.6
disk_free_limit.absolute = 2GB
channel_max = 2048
```

### PulseAudio Optimization

```bash
# Reduce latency
export PULSE_LATENCY_MSEC=20

# Set higher priority
nice -n -10 ./tts_playback_service
```

### Kernel Tuning

```bash
# Increase file descriptors
ulimit -n 65536

# TCP tuning
sysctl -w net.ipv4.tcp_fin_timeout=30
sysctl -w net.core.somaxconn=1024
```

---

## Troubleshooting

### Pod Not Starting

```bash
# Check pod status
kubectl describe pod <pod-name> -n tts-production

# Check logs
kubectl logs <pod-name> -n tts-production

# Common issues:
# - Image pull failures: Check image name/tag
# - Config errors: Verify ConfigMap/Secret
# - Resource limits: Check resource requests
```

### Audio Device Issues

```bash
# Check device availability
kubectl exec -it <pod-name> -n tts-production -- ls -la /dev/snd

# Test audio
kubectl exec -it <pod-name> -n tts-production -- \
  speaker-test -t sine -f 440 -c 2
```

### Performance Issues

```bash
# Check resource usage
kubectl top pod <pod-name> -n tts-production

# Check queue depth
kubectl exec -it rabbitmq-0 -n tts-production -- \
  rabbitmqctl list_queues

# Check cache hit rate
kubectl exec -it redis-0 -n tts-production -- \
  redis-cli INFO stats | grep keyspace_hits
```

---

## Appendix

### Complete Deployment Checklist

- [ ] Create namespace
- [ ] Apply ConfigMap
- [ ] Apply Secrets
- [ ] Deploy Redis StatefulSet
- [ ] Deploy RabbitMQ StatefulSet
- [ ] Deploy TTS Service
- [ ] Create Service
- [ ] Create Ingress (optional)
- [ ] Configure monitoring
- [ ] Set up backups
- [ ] Apply network policies
- [ ] Test health checks
- [ ] Load test

### Useful Commands

```bash
# Scale deployment
kubectl scale deployment tts-playback --replicas=1 -n tts-production

# Rollback deployment
kubectl rollout undo deployment/tts-playback -n tts-production

# Update image
kubectl set image deployment/tts-playback \
  tts-service=tts-playback-service:1.1.0 -n tts-production

# Delete namespace (cleanup)
kubectl delete namespace tts-production
```

---

**Infrastructure managed successfully! 🚀**
