import prisma from "@/lib/db";
import { NextResponse } from "next/server";

export async function GET(req: Request) {
  const { searchParams } = new URL(req.url);
  const code = searchParams.get("code");

  if (!code)
    return NextResponse.json({ error: "Missing code" }, { status: 400 });

  const room = await prisma.gameRoom.findUnique({
    where: { code },
    include: {
      players: {
        include: {
          player: true,
        },
        orderBy: { score: "desc" },
      },
    },
  });

  if (!room)
    return NextResponse.json({ error: "Room not found" }, { status: 404 });

  return NextResponse.json(room.players);
}
