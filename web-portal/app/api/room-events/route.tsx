import type { Classrooms } from "@prisma/client";
import { NextRequest } from "next/server";

type Client = {
  id: number;
  controller: ReadableStreamDefaultController;
};

let clients: Client[] = [];
let clientId = 0;

export function GET(req: NextRequest) {
  const stream = new ReadableStream({
    start(controller) {
      const id = clientId++;
      const client = { id, controller };
      clients.push(client);

      controller.enqueue(`data: connected\n\n`);

      req.signal.addEventListener("abort", () => {
        clients = clients.filter((c) => c.id !== id);
      });
    },
  });

  return new Response(stream, {
    headers: {
      "Content-Type": "text/event-stream",
      "Cache-Control": "no-cache",
      Connection: "keep-alive",
    },
  });
}

export function broadcastUpdate(data: Classrooms) {
  const json = JSON.stringify(data);
  for (const client of clients) {
    client.controller.enqueue(`data: ${json}\n\n`);
  }
}
