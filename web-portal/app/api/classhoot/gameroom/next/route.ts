import { NextResponse } from "next/server";
import prisma from "@/lib/db";

export async function POST(req: Request) {
  try {
    const body = await req.json();
    const { code } = body;

    if (!code) {
      return NextResponse.json({ error: "Missing room code" }, { status: 400 });
    }

    const room = await prisma.gameRoom.findUnique({
      where: { code },
      include: { questions: true },
    });

    if (!room) {
      return NextResponse.json({ error: "Room not found" }, { status: 404 });
    }

    // check if last question
    if (room.currentIndex + 1 >= room.questions.length) {
      await prisma.gameRoom.update({
        where: { code },
        data: { status: "ENDED" },
      });
      return NextResponse.json({ message: "Game ended" });
    }

    // increment currentIndex
    const updatedRoom = await prisma.gameRoom.update({
      where: { code },
      data: { currentIndex: { increment: 1 } },
    });

    return NextResponse.json({
      message: "Next question",
      currentIndex: updatedRoom.currentIndex,
    });
  } catch (err) {
    console.error(err);
    return NextResponse.json({ error: "Server error" }, { status: 500 });
  }
}
