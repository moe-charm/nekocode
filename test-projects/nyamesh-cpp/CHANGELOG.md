# 📋 NyaMesh C++ Changelog

All notable changes to the NyaMesh C++ implementation will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [2.1.0] - 2025-01-24

### 🎉 Overview
Major release introducing Intent Common Options, Global Debug Logger, and complete JavaScript v2.1 compatibility.

### ✨ Added
- **Intent Common Options** - Unified options for all 14 intent types
  - `broadcast`: Explicit broadcast control
  - `parallelSafe`: Parallel processing hints
  - `debug`: Per-intent debug control
  - `priority`, `timeout`, `retryCount`: Future extensibility
- **Global Debug Logger** (`NyaMeshDebugLogger`)
  - Centralized logging at send() method
  - Intent type filtering
  - File output support
  - Thread-safe implementation
  - Zero-overhead when disabled
- **Intent Interpreter** (`IntentInterpreter`)
  - Autonomous decision helper for receivers
  - Broadcast validity checking
  - Parallel safety analysis
- **New send() API overloads**
  - `send(type, data, IntentCommonOptions)`
  - `send(type, data, to, IntentCommonOptions)`

### 🔄 Changed
- Marked old `send()` API as deprecated with `[[deprecated]]` attribute
- Enhanced debug output format for better readability
- Improved log formatting to prevent character overlap

### 🛠️ Fixed
- Log output formatting issues (character overlap)
- Duplicate logging when using new API

### 📚 Documentation
- Added comprehensive API_v2.1.md
- Created MIGRATION_GUIDE_v2.1.md for smooth transition
- Updated code examples with v2.1 features

### 🔧 Technical Details
- Full backward compatibility maintained
- JavaScript v2.1 API parity achieved
- Thread-safe global logger implementation
- Compile-time deprecation warnings for migration guidance

---

## [2.0.0] - 2025-01-22

### 🎉 Overview
Revolutionary update with JavaScript-compatible API design and enhanced P2P capabilities.

### ✨ Added
- **Unified send() API** - Single method for all communication patterns
- **publish() method** - Local-only message distribution
- **JavaScript compatibility** - Full API compatibility with JS version
- **Enhanced JSON support** - Full nlohmann::json integration
- **P2P Transport Adapter** - Automatic transport selection

### 🔄 Changed
- Consolidated multiple send methods into unified API
- Simplified message routing logic
- Improved thread safety across all operations

### 🗑️ Deprecated
- `sendIntent()`
- `broadcastIntent()`
- `sendIntentToCapability()`

---

## [1.0.0] - 2025-01-15

### 🎉 Overview
Initial release of NyaMesh C++ implementation.

### ✨ Features
- **Core mesh networking** - Basic P2P mesh functionality
- **Intent-based messaging** - 14 fundamental intent types
- **Thread-safe design** - Safe concurrent operations
- **Transport abstraction** - Pluggable transport layer
- **Lifecycle management** - Proper initialization/shutdown

### 📊 Performance
- 1M+ messages/second on single node
- Sub-millisecond latency
- Minimal memory footprint

---

## [Unreleased]

### 🔮 Planned Features
- WebRTC transport support
- Cross-platform mesh discovery
- Advanced routing algorithms
- Mesh visualization tools

### 🐛 Known Issues
- None currently tracked

---

## Migration Notes

### From v2.0 to v2.1
- Existing code continues to work without changes
- Compile-time deprecation warnings guide migration
- See [MIGRATION_GUIDE_v2.1.md](MIGRATION_GUIDE_v2.1.md) for details

### From v1.0 to v2.0
- Replace `sendIntent()` with `send()`
- Update to unified API pattern
- Remove capability-specific sending

---

## Links
- [API Documentation](API_v2.1.md)
- [Migration Guide](MIGRATION_GUIDE_v2.1.md)
- [Design Philosophy](../../docs/voidcore-philosophy/)
- [JavaScript Version](../../nyamesh2.1.javascript/)