# RBX Protocol Server (ESP-IDF)

Lightweight TCP server component for ESP-IDF that implements a reverse-engineered protocol compatible with the Rabbit 2.0 mobile application.

## Disclaimer

This project is an independent implementation and is **not affiliated with, endorsed by, or connected to any official Rabbit products or companies**.

"Rabbit" and related names may be trademarks of their respective owners.
This project is intended for **interoperability, research, and educational purposes only**.

---

## Features

- TCP server for client connections  
- Frame-based protocol implementation  
- Compatible with Rabbit 2.0 mobile app  
- Simple API to send events:  
  - Pulse events  
  - Button events  
- Lightweight and suitable for embedded devices (ESP32)

---

## Installation

Run the following command in the project's root directory:

```bash
idf.py add-dependency --git https://github.com/FuraRally/rbx-protocol-server.git
```

Or add it as a dependency to the project's `idf_component.yml` file:

```yml
dependencies:
  idf:
    version: '>=5.0.0'
  # New dependency
  rbx-protocol-server:
    git: https://github.com/FuraRally/rbx-protocol-server.git
```

Or add it to your project by cloning the repository into the `components` directory:

```bash
cd components
git clone https://github.com/FuraRally/rbx-protocol-server.git
```

---

## Usage

### Start the server

```c
rb_start();
```

### Send a pulse frame

```c
int s1 = 1; // Example sensor 1 value
int s2 = 2; // Example sensor 2 value
unsigned int timestamp = 123456; // Example timestamp
rb_frame_t frame = create_pulse_frame(s1, s2, timestamp);
rb_send_frame(&frame);
```

### Send a button event

```c
int s1 = 1; // Example sensor 1 value
int s2 = 2; // Example sensor 2 value
rb_frame_t frame = create_button_frame(RB_BUTTON_UP, s1, s2);
rb_send_frame(&frame);
```

### Stop the server

```c
rb_stop();
```

---

## Protocol Overview

Frames are encoded as ASCII messages:

### Pulse

```
pulse=s1;s2;timestamp;00|\n
```

The last `00` have an unknown purpose, it is always `00` in the observed traffic, maybe it is a flags field for when the 
sensor counter or time overflows. Currently it is hardcoded to `00` in the implementation.

### Button events

```
btUp=s1;s2|\n
btDown=s1;s2|\n
btUp10=s1;s2|\n
btDown10=s1;s2|\n
lap=s1;s2|\n
ckm=s1;s2|\n
```

The `ckm` message is the one sent when the button that triggers the `lap` event is long-pressed, by itself it does not 
trigger any event in the app, but it is always sent shorty after the `lap` event.
Curently when sending a `ckm` event, a `lap` event must be sent shortly before, otherwise the app will ignore the `ckm` event.
For future improvements, the `lap` event could be sent automatically by the component when a `ckm` event is sent, to avoid the user having to send both events.

### Multiple messages per frame

More than one message can be sent in the same frame, they can be concatenated, this is the reason of the `|` character at the end of each message, it is used as a separator between messages. Only after the last message of the frame there is a `\n` character after the last message `|` character to indicate the end of the frame.

So a frame with a pulse and a button event would look like this:

```
pulse=s1;s2;timestamp;00|btUp=s1;s2|\n
```

---

## 🧩 API

### Enums

Message types:
```c
typedef enum
{
    RB_PULSE,
    RB_BUTTON_UP,
    RB_BUTTON_DOWN,
    RB_BUTTON_UP_10,
    RB_BUTTON_DOWN_10,
    RB_LAP,
    RB_CKM,
    MAX_RB_MSG_TYPE,
} rb_msg_type_t;
```

### Types

Frame structure:
```c
typedef struct {
    rb_msg_type_t type;
    int s1;
    int s2;
    unsigned int timestamp;
} rb_frame_t;
```

### Functions

* `rb_start()` → Initialize TCP server
* `rb_stop()` → Stop TCP server
* `create_pulse_frame(...)`
* `create_button_frame(...)`
* `rb_send_frame(...)`

---

## Legal Notes

* This project does **not** include any original firmware, code, or assets from third-party products.
* The protocol was implemented through independent analysis for interoperability.
* Do not use this project in a way that may violate trademarks or imply official support.

---

## License

GPL-3.0 License

See `LICENSE` file for details.

---

## Future Improvements

* Configurable port
* Configurable maximum number of clients
* Support to send multiple messages in the same frame
* Send `lap` event automatically when sending a `ckm` event, to avoid the user having to send both events.

---

## Contributing

Contributions are welcome! Feel free to open issues or pull requests.