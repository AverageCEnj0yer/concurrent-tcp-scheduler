# Concurrent TCP Task Scheduler & Order Server

A multithreaded **C++23** TCP server implementing a custom thread-safe task queue, fixed-size worker thread pool, strict schema-validated NDJSON wire protocol, cooperative order cancellation, heartbeat monitoring, and graceful POSIX signal shutdown. Includes a companion **Java** interactive CLI client.

## Architecture Overview

```text
                        ┌──────────────────────────────┐
  TCP Clients /         │       CoffeeShopServer       │
  Providers ──────────▶ │  poll(2) accept loop (main)  │
  (NDJSON over TCP)     └──────────────┬───────────────┘
                                       │ spawns per-session std::jthread
                                       ▼
                        ┌──────────────────────────────┐
                        │  ClientSession Reader Loop   │
                        │  • poll(2) + 3s HEARTBEAT    │
                        │  • NDJSON framing (8 KiB max)│
                        │  • Protocol validation       │
                        └──────────────┬───────────────┘
                                       │ submits async order jobs
                                       ▼
                        ┌──────────────────────────────┐
                        │          ThreadPool          │
                        │  • ThreadSafeQueue<Task>     │
                        │  • submitTask() -> future<T> │
                        │  • std::stop_source cancel   │
                        └──────────────────────────────┘
```

### Key Technical Highlights

- **Concurrency Primitives (`server/include/concurrency/`)**
  - `ThreadSafeQueue<T>`: Blocking FIFO queue built on `std::mutex` and `std::condition_variable` with explicit `close()` shutdown semantics returning `std::nullopt` to unblock waiting consumers cleanly.
  - `ThreadPool`: Worker pool backed by `std::vector<std::jthread>` and `ThreadSafeQueue<std::function<void()>>`, exposing a generic `submitTask(auto&& func)` returning `std::future<ReturnType>` via `std::packaged_task`.
- **Typed NDJSON Protocol (`server/include/protocol/`, `server/src/protocol/`)**
  - Compile-time enum reflection (`EnumTraits<Enum>`, `constexpr to_string` / `from_string` with C++23 `requires` constraints).
  - Strict JSON schema validation via `nlohmann::json` (required/allowed key checks, overflow-safe integer bounds checking, structured `ProtocolException` carrying protocol `ErrorCode` values).
  - Type-safe `std::variant` (`AnyMessage`) deserialization and `std::visit` routing.
  - Full specification in [`docs/protocol.md`](docs/protocol.md).
- **Concurrent TCP Server (`server/include/server/`, `server/src/server/`)**
  - Non-blocking `poll(2)` I/O loops for both the listening socket and client connections, handling partial reads, coalesced frames, CRLF endings, and oversized-line protection (8 KiB limit).
  - Thread-safe inventory reservation (`coffee`, `tea`, `croissant`) with per-item preparation times (`4s`, `3s`, `2s`) scheduled asynchronously on `ThreadPool`.
  - Cooperative mid-flight cancellation using `std::stop_source`, `std::condition_variable_any::wait_for`, and an atomic `finalized` guard so disconnected clients immediately cancel in-flight orders and restore reserved stock exactly once.
  - Role-based provider connections (`PROVIDER_IDENTIFY`), overflow-checked `DELIVERY` restocking, and automatic `LOW_STOCK` notifications whenever stock drops below `5`.
  - 3-second `HEARTBEAT` liveness enforcement and clean `SIGINT` / `SIGTERM` shutdown that tears down active sessions, cancels in-flight work, and joins all threads.

## Project Structure

```text
concurrent-tcp-scheduler/
├── docs/
│   └── protocol.md                        # Wire protocol specification
├── server/                                # C++23 TCP server (CMake)
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── concurrency/
│   │   │   ├── ThreadSafeQueue.h          # Blocking thread-safe queue template
│   │   │   └── ThreadPool.h               # Worker thread pool & future submission
│   │   ├── protocol/
│   │   │   ├── ProtocolTypes.h            # Enums, traits, structs, ProtocolException
│   │   │   ├── ProtocolJson.h             # JSON serializer/deserializer declarations
│   │   │   └── Protocol.h                 # Umbrella protocol header
│   │   └── server/
│   │       ├── ServerConfig.h             # Ports, timeouts, prep times, stock bounds
│   │       ├── NetUtils.h                 # Signal state, CLI port parser, send_all
│   │       ├── OrderRecord.h              # Active order & cooperative stop token
│   │       ├── ClientSession.h            # Thread-safe per-connection session state
│   │       └── CoffeeShopServer.h         # Main TCP server & protocol handlers
│   └── src/
│       ├── concurrency/
│       │   └── ThreadPool.cpp
│       ├── protocol/
│       │   └── ProtocolJson.cpp
│       ├── server/
│       │   ├── NetUtils.cpp
│       │   ├── ClientSession.cpp
│       │   └── CoffeeShopServer.cpp
│       └── main.cpp
└── client/                                # Companion Java CLI client (Maven)
    ├── pom.xml
    ├── README.md
    └── src/main/java/
        ├── Main.java
        ├── Protocol.java
        └── ServerListener.java
```

## Protocol Summary

All messages are UTF-8 JSON objects delimited by `\n` over TCP.

| Direction | Message Type | Purpose |
| --- | --- | --- |
| Client → Server | `MENU_REQUEST` | Query current product availability |
| Client → Server | `ORDER_REQUEST` | Reserve items and schedule asynchronous preparation |
| Client → Server | `HEARTBEAT` | Keep session alive (required at least once every 3s) |
| Provider → Server | `PROVIDER_IDENTIFY` | Register connection as active supplier |
| Provider → Server | `DELIVERY` | Replenish inventory (provider connections only) |
| Server → Client | `MENU_RESPONSE` | Current `coffee`, `tea`, and `croissant` counts |
| Server → Client | `ORDER_ACCEPTED` | Assigned `orderId` and `preparationTime` (seconds) |
| Server → Client | `ORDER_REFUSED` | Insufficient stock with explanation |
| Server → Client | `ORDER_READY` | Asynchronous completion notice for `orderId` |
| Server → Provider | `LOW_STOCK` | Alert when any product drops below 5 units |
| Server → Any | `ERROR` | Structured validation or authorization error |

See [`docs/protocol.md`](docs/protocol.md) for full schemas, validation rules, and error codes.

## Building & Running

### C++23 Server

**Requirements:** CMake ≥ 3.20 and a C++23-capable compiler (e.g. GCC ≥ 13 or Clang ≥ 17) on Linux/POSIX. `nlohmann/json` v3.12.0 is fetched automatically via CMake `FetchContent`.

```bash
cd server
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j
./build/server 8000
```

If no port argument is passed, the server defaults to port `8000`. All targets compile with `-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wsign-conversion -Wnull-dereference`.

### Companion Java CLI Client

**Requirements:** JDK 27 and Maven (see [`client/README.md`](client/README.md)).

```bash
cd client
mvn compile
mvn exec:java -Dexec.mainClass="Main" -Dexec.args="localhost 8000 client"
```

Pass `provider` instead of `client` to connect as a supplier session.

## Example Wire Session

```json
-> {"type":"MENU_REQUEST"}
<- {"items":{"coffee":24,"croissant":18,"tea":31},"type":"MENU_RESPONSE"}

-> {"type":"ORDER_REQUEST","items":{"coffee":2,"tea":1}}
<- {"orderId":1,"preparationTime":11,"type":"ORDER_ACCEPTED"}

-> {"type":"HEARTBEAT"}
...
<- {"orderId":1,"type":"ORDER_READY"}
```
