---
description: Describe when these instructions should be loaded
# applyTo: 'Describe when these instructions should be loaded' # when provided, instructions will automatically be added to the request context when the pattern matches an attached file
---

# RBX Protocol Server – Project & Coding Guidelines

## Project Overview
This project is a lightweight TCP server component for ESP-IDF, implementing a reverse-engineered protocol compatible with the Rabbit 2.0 mobile application. It is designed for embedded devices (ESP32), providing a simple API to send protocol events (pulse, button, lap, etc.) to connected clients.

## Directory Structure
- `CMakeLists.txt`: CMake build configuration, registers all `.c` files in `src/` and includes headers from `include/`.
- `idf_component.yml`: ESP-IDF component manifest, specifies dependencies and metadata.
- `LICENSE`: GPL-3.0 license.
- `README.md`: Project documentation, usage, protocol details, and API reference.
- `include/rb.h`: Public API header for the RBX protocol server.
- `src/rb.c`: Main implementation of the protocol logic and API functions.
- `src/tcp.c`: TCP server implementation for client management and data transmission.
- `src/tcp.h`: Internal TCP server API.

## File/Code Organization
- **Headers**: All public API headers are in `include/`. Internal headers (if any) are in `src/`.
- **Source Files**: All implementation files are in `src/`.
- **Naming**: 
	- Files use lowercase with underscores (e.g., `rb.c`, `tcp.c`).
	- Header guards use uppercase with underscores (e.g., `RB_H`, `TCP_H`).
	- Functions and types use `rb_` or `tcp_` prefixes for clarity and namespacing.
- **Formatting**:
	- 4-space indentation.
	- K&R style braces.
	- Doxygen-style comments for public functions.
	- Constants and macros are in uppercase.
	- Enum and struct types use `_t` suffix.

## API Design
- **Enums**: `rb_msg_type_t` defines all protocol message types.
- **Structs**: `rb_frame_t` represents a protocol frame.
- **Functions**:
	- `rb_start()`, `rb_stop()`: Start/stop the TCP server.
	- `create_pulse_frame()`, `create_button_frame()`: Helpers to create protocol frames.
	- `rb_send_frame()`: Send a frame to all connected clients.
	- TCP server functions are internal (`tcp_server_init()`, `tcp_server_send()`, etc.).

## Protocol
- **Frame Format**: ASCII messages, e.g., `pulse=s1;s2;timestamp;00|\n` for pulse, `btUp=s1;s2|\n` for button events.
- **Multiple Messages**: Messages can be concatenated with `|`, ending with `\n`.
- **Unknown Fields**: Some fields (e.g., `00` in pulse) are reserved for future use.

## Coding Guidelines
- Use clear, descriptive names for all symbols.
- Keep all protocol logic in `rb.c`/`rb.h`; keep networking logic in `tcp.c`/`tcp.h`.
- All public API functions must be documented.
- Avoid magic numbers; use macros or constants.
- All new features should be configurable where possible (e.g., port, max clients).
- Maintain compatibility with ESP-IDF 5.0+.
- Follow the existing error handling and logging patterns (use `ESP_LOG*` macros).

## Extensibility
- New protocol features should be added to the enum and handled in the frame formatting logic.
- TCP server parameters (port, max clients) should be made configurable for future improvements.
- Support for multiple messages per frame should be maintained.

## Legal
- The project is GPL-3.0 licensed.
- No original third-party code/assets are included.
- Protocol implementation is based on independent analysis.

---

Use these guidelines for all new features, code reviews, and maintenance tasks to ensure consistency and quality across the codebase.