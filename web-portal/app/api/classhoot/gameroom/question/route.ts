import { NextResponse } from "next/server";
import prisma from "@/lib/db";

export async function GET(req: Request) {
  try {
    const { searchParams } = new URL(req.url);
    const code = searchParams.get("code");

    if (!code) {
      return NextResponse.json({ error: "Missing room code" }, { status: 400 });
    }

    // Get room with currentIndex
    const room = await prisma.gameRoom.findUnique({
      where: { code },
      include: { questions: true },
    });

    if (!room) {
      return NextResponse.json({ error: "Room not found" }, { status: 404 });
    }

    // Get question based on room.currentIndex
    const currentIndex = room.currentIndex;

    const question = room.questions.find((q) => q.index === currentIndex);

    if (!question) {
      return NextResponse.json(
        { error: "Question not found for currentIndex" },
        { status: 404 }
      );
    }

    return NextResponse.json(question);
  } catch (err) {
    console.error(err);
    return NextResponse.json({ error: "Server error" }, { status: 500 });
  }
}
