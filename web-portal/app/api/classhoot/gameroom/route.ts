import { NextResponse } from "next/server";
import prisma from "@/lib/db";

async function generateRoomCode() {
  while (true) {
    const code = Math.random().toString().slice(2, 5); // "123"
    const exists = await prisma.gameRoom.findUnique({ where: { code } });

    if (!exists) return code;
  }
}

// ---- Types ----
interface QuestionInput {
  text: string;
  choice1: string;
  choice2: string;
  choice3: string;
  choice4: string;
  correctChoice: number;
  timeLimit?: number;
}

interface GameRoomPayload {
  title: string;
  questions: QuestionInput[];
}

export async function POST(req: Request) {
  try {
    const body = (await req.json()) as GameRoomPayload;

    const { title, questions } = body;

    if (!title || !Array.isArray(questions)) {
      return NextResponse.json({ error: "Invalid payload" }, { status: 400 });
    }

    const code = await generateRoomCode();

    // Create game room + nested questions
    const newRoom = await prisma.gameRoom.create({
      data: {
        title,
        code: code,
        status: "WAITING",
        totalQuestions: questions.length,
        questions: {
          create: questions.map((q, index) => ({
            text: q.text,
            index,
            choice1: q.choice1,
            choice2: q.choice2,
            choice3: q.choice3,
            choice4: q.choice4,
            correctChoice: q.correctChoice,
            timeLimit: q.timeLimit ?? 10,
          })),
        },
      },
      include: { questions: true },
    });

    return NextResponse.json(newRoom, { status: 201 });
  } catch (error) {
    console.error("Error creating game room:", error);
    return NextResponse.json({ error: "Server error" }, { status: 500 });
  }
}

export async function GET(req: Request) {
  const { searchParams } = new URL(req.url);
  const code = searchParams.get("code");

  if (!code) {
    return NextResponse.json({ error: "Code is required" }, { status: 400 });
  }

  try {
    const room = await prisma.gameRoom.findUnique({
      where: { code },
      include: { questions: true },
    });

    if (!room) {
      return NextResponse.json(
        { error: "Game room not found" },
        { status: 404 }
      );
    }

    return NextResponse.json(room, { status: 200 });
  } catch (error) {
    console.error("Error fetching game room:", error);
    return NextResponse.json({ error: "Server error" }, { status: 500 });
  }
}
