import { NextResponse } from "next/server";
import prisma from "@/lib/db";
import { pusherServer } from "@/lib/pusher";

export async function POST(req: Request) {
  try {
    const body = await req.json();
    const {
      studentId,
      gameRoomCode,
      questionIndex,
      optionSelected,
      timeTaken,
    } = body;

    // get player
    const player = await prisma.player.findUnique({
      where: { id: studentId },
    });

    if (!player)
      return NextResponse.json({ error: "Player not found" }, { status: 404 });

    // get room
    const room = await prisma.gameRoom.findUnique({
      where: { code: gameRoomCode },
      include: { players: true },
    });

    if (!room)
      return NextResponse.json({ error: "Room not found" }, { status: 404 });

    // find question
    const question = await prisma.question.findFirst({
      where: {
        gameRoomId: room.id,
        index: questionIndex,
      },
    });

    if (!question)
      return NextResponse.json(
        { error: "Question not found" },
        { status: 404 }
      );

    // has answered already?
    const existingAnswer = await prisma.answer.findFirst({
      where: { playerId: studentId, questionId: question.id },
    });

    if (existingAnswer) {
      return NextResponse.json({ message: "Already answered" });
    }

    const isCorrect = optionSelected === question.correctChoice;

    // create answer record
    await prisma.answer.create({
      data: {
        playerId: studentId,
        questionId: question.id,
        choiceId: optionSelected,
        correct: isCorrect,
        timeMs: timeTaken,
      },
    });

    // update score if correct
    let earnedScore = 0;
    if (isCorrect) {
      earnedScore = 1000 - timeTaken * 10;
      await prisma.playerRoom.update({
        where: { playerId_roomId: { playerId: studentId, roomId: room.id } },
        data: { score: { increment: earnedScore } },
      });
    }

    // get updated score
    const updatedPlayerRoom = await prisma.playerRoom.findUnique({
      where: { playerId_roomId: { playerId: studentId, roomId: room.id } },
    });

    await pusherServer.trigger(
      `game-${gameRoomCode}-${questionIndex}`,
      "answer-submitted",
      {
        studentId,
        studentName: player.name,
        optionSelected,
      }
    );

    return NextResponse.json({
      result: isCorrect ? "correct" : "wrong",
      gameStatus: room.status,
      score: updatedPlayerRoom?.score ?? 0,
    });
  } catch (error) {
    console.error(error);
    return NextResponse.json({ error: "Server error" }, { status: 500 });
  }
}
