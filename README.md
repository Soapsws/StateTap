# StateTap

StateTap is a planned C++ telemetry library with a TypeScript server and browser dashboard. The C++ tap can now register with an injected receiver, check activity and rate settings, and submit eligible values. The Control Center and network transport are still placeholders.

## Layout

- `cpp/`: isolated C++ tap module, an example and test, plus Control Center and Transporter placeholders.
- `server/`: TypeScript Dashboard Server with a health endpoint and a viewer WebSocket endpoint.
- `dashboard/`: TypeScript browser interface with connection status.
- `docs/architecture.md`: agreed L1–L4 design and L5 questions for later.

## Quick start

```sh
make       # build and run the C++ tap example
make test  # run the isolated tap test
```

The Makefile calls CMake for the C++ build. You can still use CMake directly if you need build options later.

## Run the TypeScript pieces

From the repository root:

```sh
make setup
make web-build
make server
```

In another terminal:

```sh
make dashboard
```

Open the URL printed by Vite. The interface reports its connection to the server. It does not show telemetry yet.

## Tap module boundary

`statetap::Tap<T, Sink, Clock>` observes an existing value; `statetap::Watched<T, Sink, Clock>` owns a value and emits after `set()`. A sink implements `register_source<T>(SourceInfo) -> SourceHandle` and `submit(SourceId, const T&, timestamp_ns)`. It owns the `TapSettings` slot returned in the handle and must outlive the tap. The example uses a console sink; the isolation test uses a fake sink and clock. The future Control Center will implement this sink contract.

