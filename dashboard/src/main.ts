import "./style.css";

const app = document.querySelector<HTMLDivElement>("#app");
if (!app) throw new Error("Missing dashboard root");

app.innerHTML = `
  <main class="shell">
    <header><span class="brand-mark">◉</span><span>StateTap</span><span class="tag">architecture scaffold</span></header>
    <section class="hero">
      <p class="eyebrow">LIVE PROGRAM STATE</p>
      <h1>See what your program is doing.</h1>
      <p>Connect a C++ program and select the values that matter. Telemetry capture comes after L5.</p>
      <div class="status"><span id="status-dot" class="dot"></span><span id="status-text">Connecting to dashboard server…</span></div>
    </section>
    <section class="panel"><h2>Tracked topics</h2><p>No telemetry sources are connected yet.</p></section>
  </main>`;

const statusText = document.querySelector<HTMLElement>("#status-text");
const statusDot = document.querySelector<HTMLElement>("#status-dot");
const socket = new WebSocket("ws://127.0.0.1:8787/live");

socket.addEventListener("open", () => {
  if (statusText) statusText.textContent = "Dashboard server connected · telemetry pending";
  statusDot?.classList.add("online");
});
socket.addEventListener("close", () => {
  if (statusText) statusText.textContent = "Dashboard server disconnected";
  statusDot?.classList.remove("online");
});
socket.addEventListener("error", () => {
  if (statusText) statusText.textContent = "Dashboard server unavailable";
});

