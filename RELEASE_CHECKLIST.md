# Release Checklist for GitHub

This checklist will guide you through pushing the TTS Player Queue Service to https://github.com/miPwn/tts-player-queue-service as an initial release.

## ✅ Pre-Release Checklist

### 1. Repository Setup
- [ ] Create repository on GitHub: https://github.com/miPwn/tts-player-queue-service
- [ ] Set repository visibility (Public/Private)
- [ ] Add repository description: "High-performance C++ microservice for text-to-speech audio playback with RabbitMQ queueing, Redis caching, and REST API"
- [ ] Add topics: `cpp`, `tts`, `rabbitmq`, `redis`, `pulseaudio`, `microservice`, `audio`, `queue`

### 2. Local Repository Preparation
```bash
# Initialize git (if not already done)
git init

# Add all files
git add .

# Create initial commit
git commit -m "Initial release: TTS Player Queue Service v1.0.0

- High-performance C++ microservice for TTS audio playback
- REST API with multipart/form-data support
- RabbitMQ queue for async job processing
- Redis LRU cache for WAV files
- PulseAudio synchronous playback
- Docker multi-stage build
- Comprehensive test suite (57 tests, 100% coverage)
- GitHub Actions CI/CD pipeline with clang-tidy
- Complete documentation suite"

# Add remote
git remote add origin https://github.com/miPwn/tts-player-queue-service.git

# Push to GitHub
git branch -M main
git push -u origin main
```

### 3. GitHub Repository Configuration

#### Branch Protection Rules
- [ ] Go to Settings → Branches → Add rule
- [ ] Branch name pattern: `main`
- [ ] Enable: "Require a pull request before merging"
- [ ] Enable: "Require status checks to pass before merging"
  - [ ] Select: `build-and-test`
  - [ ] Select: `code-quality`
  - [ ] Select: `security-scan`
- [ ] Enable: "Require branches to be up to date before merging"
- [ ] Enable: "Include administrators"

#### GitHub Actions Permissions
- [ ] Go to Settings → Actions → General
- [ ] Workflow permissions: "Read and write permissions"
- [ ] Enable: "Allow GitHub Actions to create and approve pull requests"

#### Security Settings
- [ ] Go to Settings → Security → Code security and analysis
- [ ] Enable: "Dependency graph"
- [ ] Enable: "Dependabot alerts"
- [ ] Enable: "Dependabot security updates"
- [ ] Enable: "Code scanning" (already configured via Trivy)

### 4. Repository Secrets (if needed for future deployment)
- [ ] Go to Settings → Secrets and variables → Actions
- [ ] Add secrets as needed:
  - `DOCKER_USERNAME` (for Docker Hub)
  - `DOCKER_PASSWORD` (for Docker Hub)
  - `KUBECONFIG` (for Kubernetes deployment)

### 5. Documentation Review
- [ ] Verify README.md displays correctly
- [ ] Check all documentation links work
- [ ] Verify badges show correct status
- [ ] Review API documentation
- [ ] Check Kubernetes manifests

## 📋 Post-Release Tasks

### 1. Verify CI/CD Pipeline
- [ ] Go to Actions tab
- [ ] Verify all 4 jobs run successfully:
  - [ ] Build and Test (~10-27 min)
  - [ ] Docker Build (~3-10 min)
  - [ ] Code Quality (~1 min)
  - [ ] Security Scan (~2 min)
- [ ] Check test results artifact
- [ ] Review clang-tidy output
- [ ] Check security scan results in Security tab

### 2. Create Initial Release
```bash
# Tag the release
git tag -a v1.0.0 -m "Release v1.0.0: Initial public release

Features:
- REST API for TTS audio playback
- RabbitMQ queue with guaranteed ordering
- Redis LRU cache
- PulseAudio synchronous playback
- Docker containerization
- Kubernetes deployment manifests
- 57 integration tests with 100% coverage
- CI/CD pipeline with clang-tidy linting"

# Push tag
git push origin v1.0.0
```

- [ ] Go to Releases → Create a new release
- [ ] Choose tag: `v1.0.0`
- [ ] Release title: `v1.0.0 - Initial Release`
- [ ] Description: Copy from tag message
- [ ] Attach artifacts (optional):
  - [ ] Compiled binary
  - [ ] Docker image
- [ ] Publish release

### 3. Update Documentation
- [ ] Add "Getting Started" section to README if needed
- [ ] Create GitHub Wiki pages (optional)
- [ ] Add examples directory with sample code
- [ ] Create video demo (optional)

### 4. Community Setup
- [ ] Enable Discussions
- [ ] Create discussion categories:
  - [ ] Announcements
  - [ ] General
  - [ ] Ideas
  - [ ] Q&A
  - [ ] Show and tell
- [ ] Pin important discussions
- [ ] Create issue templates (already done ✅)

### 5. Monitoring Setup
- [ ] Set up GitHub notifications
- [ ] Configure email alerts for failed builds
- [ ] Set up Slack/Discord integration (optional)
- [ ] Monitor Security tab for vulnerabilities

## 🎯 First Contribution

### Test the Contribution Workflow
- [ ] Create a test branch
- [ ] Make a small change
- [ ] Open a pull request
- [ ] Verify PR template appears
- [ ] Verify CI/CD runs on PR
- [ ] Verify branch protection works
- [ ] Merge PR
- [ ] Verify main branch protection

## 📊 Success Metrics

After release, monitor:
- [ ] CI/CD pipeline success rate (target: >95%)
- [ ] Build time (target: <15 min with cache)
- [ ] Test pass rate (target: 100%)
- [ ] Code coverage (target: 100%)
- [ ] Security vulnerabilities (target: 0 critical/high)
- [ ] Community engagement (stars, forks, issues)

## 🔧 Troubleshooting

### If CI/CD Fails on First Run

**Issue**: Dependencies not cached
- **Solution**: Wait for first build to complete (~27 min), subsequent builds will be faster

**Issue**: Tests fail with connection errors
- **Solution**: Check service health in workflow logs, may need to adjust health check timeouts

**Issue**: clang-tidy warnings
- **Solution**: Review warnings, fix critical issues, adjust `.clang-tidy` config if needed

**Issue**: Docker build fails
- **Solution**: Check Dockerfile, verify all dependencies are available

### If Badges Don't Show

**Issue**: CI/CD badge shows "no status"
- **Solution**: Wait for first workflow run to complete, then badge will update

**Issue**: Badge shows wrong status
- **Solution**: Check workflow name matches badge URL exactly

## 📝 Additional Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Branch Protection Rules](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/defining-the-mergeability-of-pull-requests/about-protected-branches)
- [GitHub Security Features](https://docs.github.com/en/code-security)
- [Creating Releases](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository)

## ✨ You're Ready!

Once all items are checked, your repository is production-ready with:
- ✅ Professional CI/CD pipeline
- ✅ Comprehensive testing
- ✅ Code quality enforcement
- ✅ Security scanning
- ✅ Complete documentation
- ✅ Community templates
- ✅ Developer tooling

**Happy releasing! 🚀**

---

**Last Updated**: October 13, 2025
**Version**: 1.0.0
**License**: AGPL-3.0
