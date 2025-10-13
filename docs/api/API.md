# API Documentation

## TTS Playback Service REST API

**Version:** 1.0.0  
**Base URL:** `http://localhost:8080`  
**Protocol:** HTTP/1.1

---

## Table of Contents

1. [Overview](#overview)
2. [Authentication](#authentication)
3. [Endpoints](#endpoints)
4. [Request/Response Formats](#requestresponse-formats)
5. [Error Handling](#error-handling)
6. [Rate Limiting](#rate-limiting)
7. [Examples](#examples)
8. [Client Libraries](#client-libraries)

---

## Overview

The TTS Playback Service provides a REST API for submitting text-to-speech audio files for playback. The service handles caching, queuing, and sequential playback automatically.

### Key Features

- **WAV File Upload**: Submit audio files with multipart/form-data
- **Automatic Caching**: Repeated text uses cached audio
- **Async Processing**: Jobs queued for sequential playback
- **Health Monitoring**: Built-in health check endpoint

### API Characteristics

- **Stateless**: No session management required
- **Idempotent**: Same text + WAV can be submitted multiple times
- **Low Latency**: <10ms API response time
- **Reliable**: Jobs persisted in RabbitMQ

---

## Authentication

**Current Version**: No authentication required

**Future Versions** may include:
- API Key authentication (`X-API-Key` header)
- OAuth 2.0
- JWT tokens

### Example (Future)

```bash
curl -H "X-API-Key: your-api-key" \
  http://localhost:8080/api/tts/play
```

---

## Endpoints

### POST /api/tts/play

Submit a WAV audio file with corresponding text for playback.

#### Request

**Method**: `POST`  
**Content-Type**: `multipart/form-data`  
**Endpoint**: `/api/tts/play`

**Form Fields**:

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `text` | string | Yes | Text content used to generate the audio (max 10,000 chars) |
| `wav` | binary | Yes | WAV audio file |

**Example**:
```bash
curl -X POST http://localhost:8080/api/tts/play \
  -F "text=Hello, this is a test sentence." \
  -F "wav=@audio.wav"
```

#### Response

**Success (200 OK)**:
```json
{
  "status": "queued",
  "text": "Hello, this is a test sentence.",
  "size": 44100
}
```

**Response Fields**:

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | Job status (always "queued") |
| `text` | string | Text content (truncated to 100 chars) |
| `size` | integer | WAV file size in bytes |

**Error Responses**:

| Status Code | Description | Example |
|-------------|-------------|---------|
| 400 | Bad Request | Missing field, invalid format |
| 500 | Internal Server Error | Service unavailable |

**Example Error (400)**:
```json
{
  "error": "Missing 'text' field"
}
```

**Example Error (500)**:
```json
{
  "error": "Failed to process request: RabbitMQ connection error"
}
```

---

### GET /health

Service health check endpoint.

#### Request

**Method**: `GET`  
**Endpoint**: `/health`

**Example**:
```bash
curl http://localhost:8080/health
```

#### Response

**Success (200 OK)**:
```json
{
  "status": "healthy",
  "service": "tts-playback-service"
}
```

**Response Fields**:

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | Health status ("healthy" or "unhealthy") |
| `service` | string | Service identifier |

**Use Cases**:
- Kubernetes liveness probes
- Kubernetes readiness probes
- Load balancer health checks
- Monitoring systems

---

## Request/Response Formats

### Content Types

**Request**:
- `multipart/form-data` (for file uploads)

**Response**:
- `application/json` (all responses)

### WAV File Format

**Supported**:
- Format: WAV (RIFF WAVE)
- Sample Rate: Any (44100 Hz recommended)
- Channels: 1 (mono) or 2 (stereo)
- Bit Depth: 8, 16, 24, or 32 bit
- Max Size: Limited by server config (10 MB default)

**Example WAV Header**:
```
RIFF....WAVE
fmt ............
data........
```

### JSON Schema

#### PlaybackResponse
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["status", "text", "size"],
  "properties": {
    "status": {
      "type": "string",
      "enum": ["queued"]
    },
    "text": {
      "type": "string",
      "maxLength": 100
    },
    "size": {
      "type": "integer",
      "minimum": 0
    }
  }
}
```

#### ErrorResponse
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["error"],
  "properties": {
    "error": {
      "type": "string"
    }
  }
}
```

---

## Error Handling

### Error Response Format

All errors return a JSON object with an `error` field:

```json
{
  "error": "Error message description"
}
```

### HTTP Status Codes

| Code | Meaning | When It Occurs |
|------|---------|---------------|
| 200 | OK | Request successful |
| 400 | Bad Request | Invalid input (missing fields, wrong format) |
| 404 | Not Found | Endpoint doesn't exist |
| 500 | Internal Server Error | Service error (RabbitMQ/Redis down) |
| 503 | Service Unavailable | Service starting up or shutting down |

### Common Error Messages

| Error Message | Cause | Solution |
|--------------|-------|----------|
| `Missing 'text' field` | No text in form data | Include text field |
| `Missing 'wav' file` | No WAV file uploaded | Include wav file |
| `Content-Type must be multipart/form-data` | Wrong content type | Use multipart/form-data |
| `Invalid WAV file: too small` | WAV file corrupted | Upload valid WAV file |
| `RabbitMQ connection error` | Queue service down | Check RabbitMQ status |
| `Redis connection error` | Cache service down | Check Redis status |

### Error Handling Best Practices

**Client-side**:
```javascript
async function submitAudio(text, wavFile) {
  const formData = new FormData();
  formData.append('text', text);
  formData.append('wav', wavFile);
  
  try {
    const response = await fetch('http://localhost:8080/api/tts/play', {
      method: 'POST',
      body: formData
    });
    
    const data = await response.json();
    
    if (!response.ok) {
      throw new Error(data.error || 'Unknown error');
    }
    
    return data;
  } catch (error) {
    console.error('Failed to submit audio:', error.message);
    throw error;
  }
}
```

---

## Rate Limiting

**Current Version**: No rate limiting implemented

**Recommendations for Production**:
- Implement rate limiting at API gateway level
- Suggested limits:
  - 100 requests per minute per IP
  - 1000 requests per hour per API key
- Return 429 Too Many Requests when exceeded

**Future Headers**:
```
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 95
X-RateLimit-Reset: 1635724800
```

---

## Examples

### cURL Examples

#### Basic Request
```bash
curl -X POST http://localhost:8080/api/tts/play \
  -F "text=Hello world" \
  -F "wav=@test.wav"
```

#### With Verbose Output
```bash
curl -v -X POST http://localhost:8080/api/tts/play \
  -F "text=Hello world" \
  -F "wav=@test.wav"
```

#### Health Check
```bash
curl http://localhost:8080/health
```

### Python Example

```python
import requests

def submit_tts_audio(text, wav_file_path):
    url = 'http://localhost:8080/api/tts/play'
    
    files = {
        'text': (None, text),
        'wav': ('audio.wav', open(wav_file_path, 'rb'), 'audio/wav')
    }
    
    response = requests.post(url, files=files)
    
    if response.status_code == 200:
        print(f"Success: {response.json()}")
    else:
        print(f"Error: {response.json()}")
    
    return response.json()

# Usage
result = submit_tts_audio("Hello world", "test.wav")
```

### JavaScript Example

```javascript
async function submitTTSAudio(text, wavFile) {
  const formData = new FormData();
  formData.append('text', text);
  formData.append('wav', wavFile);
  
  const response = await fetch('http://localhost:8080/api/tts/play', {
    method: 'POST',
    body: formData
  });
  
  const data = await response.json();
  console.log(data);
  return data;
}

// Usage with file input
const fileInput = document.getElementById('wavFile');
const text = "Hello world";
submitTTSAudio(text, fileInput.files[0]);
```

### Go Example

```go
package main

import (
    "bytes"
    "encoding/json"
    "fmt"
    "io"
    "mime/multipart"
    "net/http"
    "os"
)

type PlaybackResponse struct {
    Status string `json:"status"`
    Text   string `json:"text"`
    Size   int    `json:"size"`
}

func submitTTSAudio(text, wavPath string) (*PlaybackResponse, error) {
    // Create multipart form
    body := &bytes.Buffer{}
    writer := multipart.NewWriter(body)
    
    // Add text field
    writer.WriteField("text", text)
    
    // Add WAV file
    file, err := os.Open(wavPath)
    if err != nil {
        return nil, err
    }
    defer file.Close()
    
    part, err := writer.CreateFormFile("wav", "audio.wav")
    if err != nil {
        return nil, err
    }
    io.Copy(part, file)
    writer.Close()
    
    // Make request
    req, err := http.NewRequest("POST", "http://localhost:8080/api/tts/play", body)
    if err != nil {
        return nil, err
    }
    req.Header.Set("Content-Type", writer.FormDataContentType())
    
    client := &http.Client{}
    resp, err := client.Do(req)
    if err != nil {
        return nil, err
    }
    defer resp.Body.Close()
    
    // Parse response
    var result PlaybackResponse
    json.NewDecoder(resp.Body).Decode(&result)
    
    return &result, nil
}

func main() {
    result, err := submitTTSAudio("Hello world", "test.wav")
    if err != nil {
        fmt.Println("Error:", err)
        return
    }
    fmt.Printf("Success: %+v\n", result)
}
```

### Shell Script Example

```bash
#!/bin/bash

# Configuration
API_URL="http://localhost:8080/api/tts/play"
TEXT="$1"
WAV_FILE="$2"

# Validate arguments
if [ -z "$TEXT" ] || [ -z "$WAV_FILE" ]; then
    echo "Usage: $0 <text> <wav_file>"
    exit 1
fi

# Check if file exists
if [ ! -f "$WAV_FILE" ]; then
    echo "Error: WAV file not found: $WAV_FILE"
    exit 1
fi

# Submit to API
RESPONSE=$(curl -s -X POST "$API_URL" \
    -F "text=$TEXT" \
    -F "wav=@$WAV_FILE")

# Parse response
STATUS=$(echo "$RESPONSE" | jq -r '.status')

if [ "$STATUS" == "queued" ]; then
    echo "✓ Audio queued successfully"
    echo "$RESPONSE" | jq '.'
else
    echo "✗ Error submitting audio"
    echo "$RESPONSE" | jq '.'
    exit 1
fi
```

### Docker Integration Example

```dockerfile
# Client container that submits audio
FROM alpine:latest

RUN apk add --no-cache curl jq

COPY submit.sh /usr/local/bin/
COPY test.wav /data/

CMD ["/usr/local/bin/submit.sh", "Hello from Docker", "/data/test.wav"]
```

---

## Client Libraries

### Official SDKs (Future)

Planned client libraries:
- Python SDK
- JavaScript/TypeScript SDK
- Go SDK
- Java SDK

### Community Libraries

Contributions welcome! Submit your client library via GitHub.

### Building Your Own Client

**Requirements**:
1. HTTP client with multipart/form-data support
2. File I/O for WAV files
3. JSON parser for responses
4. Error handling

**Example Template**:
```
1. Read WAV file into memory
2. Create multipart form with text + WAV
3. POST to /api/tts/play
4. Parse JSON response
5. Handle errors appropriately
```

---

## OpenAPI Specification

Full OpenAPI 3.0 specification available at:
- [openapi.yaml](openapi.yaml)
- Interactive docs (Swagger UI): *Coming soon*

**Generate Client Code**:
```bash
# Install OpenAPI Generator
npm install -g @openapitools/openapi-generator-cli

# Generate Python client
openapi-generator-cli generate \
  -i docs/api/openapi.yaml \
  -g python \
  -o clients/python

# Generate TypeScript client
openapi-generator-cli generate \
  -i docs/api/openapi.yaml \
  -g typescript-fetch \
  -o clients/typescript
```

---

## Versioning

**Current API Version**: v1.0.0

### Versioning Strategy

- **Major version** (v1.x.x → v2.x.x): Breaking changes
- **Minor version** (v1.0.x → v1.1.x): New features (backward compatible)
- **Patch version** (v1.0.0 → v1.0.1): Bug fixes

### API Version Header (Future)

```bash
curl -H "Accept: application/vnd.tts.v1+json" \
  http://localhost:8080/api/tts/play
```

---

## Best Practices

### Client Implementation

1. **Retry Logic**: Implement exponential backoff for 500 errors
2. **Timeouts**: Set reasonable request timeouts (30s recommended)
3. **Error Handling**: Always check response status and parse errors
4. **File Validation**: Validate WAV format before upload
5. **Connection Pooling**: Reuse HTTP connections for better performance

### Performance Tips

1. **Batch Requests**: Submit multiple jobs in sequence
2. **Compress WAV**: Use lower sample rates if acceptable
3. **Cache on Client**: Cache responses to avoid duplicate submissions
4. **Use Keep-Alive**: Enable HTTP keep-alive for connection reuse

### Security Considerations

1. **HTTPS**: Use TLS in production (deploy behind reverse proxy)
2. **Input Validation**: Validate text length and WAV format client-side
3. **File Size Limits**: Respect max file size (10 MB)
4. **Rate Limiting**: Implement client-side rate limiting to be respectful

---

## Changelog

### v1.0.0 (2025-10-13)
- Initial API release
- POST /api/tts/play endpoint
- GET /health endpoint
- Multipart/form-data support
- JSON responses

### Future Versions
- v1.1.0: API key authentication
- v1.2.0: Batch upload support
- v1.3.0: WebSocket streaming
- v2.0.0: GraphQL API

---

## Support

- **Documentation**: [https://docs.example.com](https://docs.example.com)
- **GitHub Issues**: [https://github.com/your-org/tts-playback-service/issues](https://github.com/your-org/tts-playback-service/issues)
- **API Status**: [https://status.example.com](https://status.example.com)

---

**API Version:** 1.0.0  
**Last Updated:** October 13, 2025
