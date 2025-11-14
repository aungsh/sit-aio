import { NextResponse } from "next/server";
import prisma from "@/lib/db";

export async function GET(req: Request) {
  try {
    const { searchParams } = new URL(req.url);
    const code = searchParams.get("code");
    const indexStr = searchParams.get("index");

    if (!code || !indexStr) {
      return NextResponse.json(
        { error: "Missing code or index" },
        { status: 400 }
      );
    }

    const index = parseInt(indexStr, 10);

    const room = await prisma.gameRoom.findUnique({
      where: { code },
      include: { questions: true },
    });

    if (!room) {
      return NextResponse.json({ error: "Room not found" }, { status: 404 });
    }

    const question = room.questions.find((q) => q.index === index);
    if (!question) {
      return NextResponse.json(
        { error: "Question not found" },
        { status: 404 }
      );
    }

    return NextResponse.json(question);
  } catch (err) {
    console.error(err);
    return NextResponse.json({ error: "Server error" }, { status: 500 });
  }
}
