# StateTap

StateTap is a planned C++ telemetry library with a TypeScript server and browser dashboard. This repository is an **architecture scaffold**: the pieces build, but the C++ tap does not yet transmit data.

## Layout

- `cpp/`: public C++ API, in-process Control Center and Transporter placeholders, and a small example.
- `server/`: TypeScript Dashboard Server with a health endpoint and a viewer WebSocket endpoint.
- `dashboard/`: TypeScript browser interface with connection status.
- `docs/architecture.md`: agreed L1–L4 design and L5 questions for later.

## Build the C++ library

```sh
cmake -S cpp -B build/cpp
cmake --build build/cpp
./build/cpp/statetap_example
```

## Run the TypeScript pieces

From the repository root:

```sh
npm install
npm run build
npm run dev:server
```

In another terminal:

```sh
npm run dev:dashboard
```

Open the URL printed by Vite. The interface reports its connection to the server. It does not show telemetry yet.

