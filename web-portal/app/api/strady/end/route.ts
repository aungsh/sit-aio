import { NextResponse } from "next/server";
import prisma from "@/lib/db";

export async function POST(req: Request) {
  try {
    const { studySessionId } = await req.json();

    if (!studySessionId) {
      return NextResponse.json(
        { error: "studySessionId is required" },
        { status: 400 }
      );
    }

    const session = await prisma.studySession.findUnique({
      where: { id: studySessionId },
    });

    if (!session) {
      return NextResponse.json(
        { error: "Study session not found" },
        { status: 404 }
      );
    }

    if (session.endTime) {
      return NextResponse.json(
        { error: "Session already ended" },
        { status: 400 }
      );
    }

    const end = new Date();
    const durationMin = Math.floor(
      (end.getTime() - session.startTime.getTime()) / 60000
    );

    // Update session
    const updated = await prisma.studySession.update({
      where: { id: studySessionId },
      data: {
        endTime: end,
        durationMin,
      },
    });

    return NextResponse.json({
      studySessionId: updated.id,
      durationMin: updated.durationMin,
    });
  } catch (error) {
    console.error("Error ending study session:", error);
    return NextResponse.json(
      { error: "Internal server error" },
      { status: 500 }
    );
  }
}
