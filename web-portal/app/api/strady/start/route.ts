import { NextResponse } from "next/server";
import prisma from "@/lib/db";

export async function POST(req: Request) {
  try {
    const { studentId, studentName } = await req.json();

    if (!studentId || !studentName) {
      return NextResponse.json(
        { error: "studentId and studentName are required" },
        { status: 400 }
      );
    }

    // Ensure Player exists (create if not)
    const player = await prisma.player.upsert({
      where: { id: studentId },
      update: {},
      create: {
        id: studentId,
        name: studentName,
      },
    });

    // Create study session
    const session = await prisma.studySession.create({
      data: {
        studentId: player.id,
      },
    });

    return NextResponse.json({ studySessionId: session.id });
  } catch (error) {
    console.error("Error starting study session:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}
