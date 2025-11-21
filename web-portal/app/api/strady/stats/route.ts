import { NextResponse } from "next/server";
import prisma from "@/lib/db";

export async function GET() {
  try {
    // --- Fetch all completed study sessions ---
    const sessions = await prisma.studySession.findMany({
      where: {
        durationMin: { not: null }, // only finished sessions
      },
      orderBy: { startTime: "asc" },
    });

    // Group by day
    const dailyMap: Record<string, number> = {};

    sessions.forEach((s) => {
      const date = s.startTime.toISOString().split("T")[0];
      dailyMap[date] = (dailyMap[date] || 0) + (s.durationMin || 0);
    });

    const daily = Object.entries(dailyMap).map(([date, minutes]) => ({
      date,
      minutes,
    }));

    // Placeholder until you add category/tags to StudySession
    const category = [
      {
        category: "General",
        minutes: daily.reduce((a, b) => a + b.minutes, 0),
      },
    ];

    const recent = await prisma.studySession.findMany({
      where: { durationMin: { not: null } },
      orderBy: { startTime: "desc" },
      take: 100,
      include: {
        student: {
          select: { name: true },
        },
      },
    });

    return NextResponse.json({
      daily,
      category,
      recent: recent.map((s) => ({
        id: s.id,
        startTime: s.startTime,
        durationMin: s.durationMin,
        name: s.student.name,
      })),
    });
  } catch (error) {
    console.error("Error generating study stats:", error);
    return NextResponse.json(
      { error: "Failed to load study stats" },
      { status: 500 }
    );
  }
}
