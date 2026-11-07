import { createServer } from "node:http";
import { WebSocketServer } from "ws";

const port = Number(process.env.STATETAP_PORT ?? 8787);
const viewers = new WebSocketServer({ noServer: true });

const server = createServer((request, response) => {
  if (request.url === "/health") {
    response.writeHead(200, { "content-type": "application/json" });
    response.end(JSON.stringify({ status: "ok", telemetry: "not-implemented" }));
    return;
  }
  response.writeHead(404);
  response.end();
});

server.on("upgrade", (request, socket, head) => {
  if (request.url !== "/live") {
    socket.destroy();
    return;
  }
  viewers.handleUpgrade(request, socket, head, (connection) => {
    viewers.emit("connection", connection, request);
  });
});

viewers.on("connection", (connection) => {
  connection.send(JSON.stringify({ kind: "server.ready", telemetry: false }));
});

server.listen(port, "127.0.0.1", () => {
  console.log(`StateTap scaffold server: http://127.0.0.1:${port}/health`);
});

