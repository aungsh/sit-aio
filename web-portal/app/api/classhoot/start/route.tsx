import { NextResponse } from "next/server";
import prisma from "@/lib/db";

export async function POST(req: Request) {
  try {
    const { gameRoomCode } = await req.json();

    const room = await prisma.gameRoom.findUnique({
      where: { code: gameRoomCode },
      include: {
        questions: {
          orderBy: { index: "asc" },
        },
      },
    });

    if (!room) {
      return NextResponse.json(
        { error: "Game room not found" },
        { status: 404 }
      );
    }

    // If game ended send response
    if (room.status === "ENDED") {
      return NextResponse.json({ status: "ENDED" });
    }

    // If still waiting, tell player to wait
    if (room.status === "WAITING") {
      return NextResponse.json({ status: "WAITING" });
    }

    // If ONGOING: return current question
    const question = room.questions[room.currentIndex];
    if (!question) {
      return NextResponse.json({ status: "ENDED" });
    }

    return NextResponse.json({
      status: "ONGOING",
      questionIndex: room.currentIndex + 1,
      totalQuestions: room.totalQuestions,
      question: question.text,
      option1: question.choice1,
      option2: question.choice2,
      option3: question.choice3,
      option4: question.choice4,
      correctOption: question.correctChoice,
      timeLimit: question.timeLimit,
    });
  } catch (error) {
    console.error(error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}
