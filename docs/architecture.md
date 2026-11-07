# StateTap architecture — L1 through L5 working draft

This document records the design agreed in conversation. L5 is in progress: the decisions below guide the MVP, while the final section lists contracts still to define.

## L1 — Shape

StateTap is a C++ instrumentation library and live browser dashboard. Developers place taps on recurring values in a program; selected values are streamed to a TypeScript dashboard. Common value types will be supported directly, and custom types will need an explicit representation. StateTap targets continuous observation, so a value that occurs only once may never be displayed.

## L2 — Method

The dashboard selects topics and rates. Each reached tap checks local subscription settings before capturing a value. Accepted values travel through an in-process Control Center and Transporter to the Dashboard Server. The server supplies live updates to the interface and sends configuration changes back to the C++ process. The program's own execution determines when a tap is reached; the dashboard does not remotely read an arbitrary live variable.

Speed is the primary architectural motivation. The MVP should keep capture, queueing, encoding, transport, server ingest, and display separable and measurable so a bottleneck can be improved without redesigning the entire tool. More threads, Transporters, or processes are justified by measurements rather than added up front.

## L3 — Members and ownership

1. **Wrapper or tap site:** lives in the developer's program. It names and identifies a value, checks its local active/rate settings when reached, and submits an eligible sample. The developer's program owns the underlying value and its lifetime. A tap's code-defined identity and type do not change during a run; its active setting and rate may change.
2. **Control Center:** one per instrumented C++ process. It owns tap registrations and applied settings, not the wrapper objects or developer values. It accepts eligible samples; it does not continuously scan every wrapper. It validates configuration before applying it locally.
3. **Transporter:** one long-lived instance per instrumented C++ process in the MVP. It owns the server connection and pending outbound data, sends telemetry, and carries configuration back. It is not spawned for each value. Encoding one value may be stateless, but the Transporter itself has connection and batching state.
4. **Dashboard Server:** one separate process in the MVP. It is authoritative for user-configurable telemetry settings, the catalog of known tap metadata, and the bounded set of current values it serves to the interface. It validates user requests, including topic existence and allowed rates. Tap metadata originates in the instrumented process and is registered with the server.
5. **Dashboard Interface:** a TypeScript browser view. It sends user intent to the server and renders server-provided data. It does not own authoritative telemetry or configuration. The MVP has one interface; more viewers are optional later.

The first implementation targets one instrumented C++ process, one Control Center, one Transporter, one Dashboard Server, and one Dashboard Interface. Multiple instrumented processes are supported as a future extension for applications that already have multiple programs; they are not required to make StateTap faster.

## L4 — Flow

1. The instrumented program starts StateTap and the Transporter connects to the Dashboard Server.
2. **On first execution of a tap site**, the tap registers a descriptor with the Control Center and receives a lightweight source handle. The descriptor identifies the source, name, type, and code location. The descriptor reaches the server's searchable topic catalog. A tap that has never executed is not yet discoverable.
3. The user selects topics and rates in the interface. The server validates the request, then sends the resulting configuration through the Transporter to the Control Center. The Control Center applies it to the relevant local registration.
4. Each time program execution reaches the tap, its handle checks the local active/rate settings. If capture is due, it submits the value to the Control Center. The Control Center does not poll or harvest wrappers.
5. The Control Center passes accepted samples to the Transporter. The Transporter sends them to the server, which updates its current values and pushes relevant updates to the interface.
6. Interface changes repeat the configuration flow. Changing telemetry settings never changes the developer's underlying program value.

## L5 — Decisions made so far

### Registration and handles

Registration means creating a description of a tap, not transferring ownership of its C++ value. The first execution of a tap site performs registration once; subsequent executions reuse its handle. A handle is a small local reference to that registration. It may hold a non-owning pointer to the Control Center or to a stable registration slot, plus a source ID. The wrapper calls through the handle because the developer's program reached the tap. The pointer dependency is acceptable; the Control Center must outlive every handle that can use it. The exact handle representation remains open to benchmarking and lifetime design.

### Change boundary

The dashboard may change telemetry behavior such as whether a source is active and its rate. A tap's identity, type, and location are fixed by the developer's code for a run. The interface does not mutate the instrumented program's variables.

### Performance and evidence

An inactive or not-due tap should avoid value capture, encoding, and transfer. The eligibility check still occurs when program execution reaches the tap, so its cost must be measured. Benchmarks should separately measure capture overhead, accepted events per second, queue depth, dropped events, server processing, and end-to-end delay. The MVP starts with one Transporter and no process sharding. Batch size and worker parallelism can be tuned after profiling.

## L5 — Contracts still open

- Exact public C++ syntax and guarantees for expression taps versus persistent wrappers.
- Source ID generation, duplicate names, and registration/unregistration for dynamic wrapper instances.
- Type support, custom encoders, and the event/message format. Source metadata should not need to be repeated in full on every value.
- Precise rate semantics and clock choice, including concurrent calls to one tap site.
- Buffer capacity, batching policy, overload/drop behavior, and disconnect handling.
- Transport protocol and shutdown/lifetime guarantees for the Control Center and handles.
- Server cache retention, dashboard rendering policy, and any later history/recording feature.

