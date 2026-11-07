# Architecture through L4

## L1 — Shape

StateTap is a C++ instrumentation library and live browser dashboard. Developers place taps on recurring values in a program; selected values are streamed to a TypeScript dashboard. Common value types will be supported directly, and custom types will need an explicit representation.

## L2 — Method

The dashboard selects topics and rates. Each reached tap checks local subscription settings before capturing a value. Accepted values travel through an in-process Control Center and Transporter to the Dashboard Server. The server supplies live updates to the interface and sends configuration changes back to the C++ process. The program's own execution determines when a tap is reached.

## L3 — Members

1. **Wrapper or tap site:** names a value, identifies its type, and checks whether capture is due.
2. **Control Center:** one per instrumented C++ process; holds registrations and local settings and accepts eligible values.
3. **Transporter:** one per instrumented C++ process; sends accepted values and receives configuration updates.
4. **Dashboard Server:** receives telemetry and routes configuration for one or more instrumented programs.
5. **Dashboard Interface:** lets a user select topics and rates and view live values.

The first implementation targets one instrumented process, one server, and one interface. Additional instrumented processes are optional.

## L4 — Flow

1. On startup, taps become known to the Control Center, and the Transporter connects to the Dashboard Server.
2. The user selects topics and rates in the Dashboard Interface.
3. The server sends those settings through the Transporter to the Control Center.
4. When the program reaches a tap, the tap checks its local settings. If active and due, it captures the value and hands it to the Control Center.
5. The Transporter sends accepted values to the server; the server sends relevant updates to the interface.
6. Changes to the user's selection repeat the configuration flow.

## L5 — Deferred decisions

- Public C++ API and wrapper guarantees, including types and short-lived values.
- Topic identity and registration lifecycle.
- Exact rate and delivery policies, clock rules, and overload behavior.
- Event representation and C++↔server protocol.
- Buffer ownership, disconnect behavior, and shutdown guarantees.
- Dashboard data retention and rendering policy.

