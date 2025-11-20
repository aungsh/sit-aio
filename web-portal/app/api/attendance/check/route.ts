import { NextResponse } from "next/server";
import prisma from "@/lib/db";

export async function POST(req: Request) {
  try {
    const { studentId, classroomName } = await req.json();

    if (!studentId || !classroomName) {
      return NextResponse.json(
        { error: "Missing studentId or classroomName" },
        { status: 400 }
      );
    }

    // Find classroom
    const classroom = await prisma.classrooms.findFirst({
      where: { name: classroomName },
    });

    if (!classroom) {
      return NextResponse.json(
        { error: "Classroom not found" },
        { status: 404 }
      );
    }

    // Time 1 hour ago
    const oneHourAgo = new Date(Date.now() - 60 * 60 * 1000);

    // Check attendance within last 1 hour
    const existing = await prisma.attendance.findFirst({
      where: {
        studentId: Number(studentId),
        roomId: classroom.id,
        joinedAt: {
          gte: oneHourAgo,
        },
      },
      orderBy: { joinedAt: "desc" },
    });

    if (existing) {
      return NextResponse.json({
        taken: true,
        attendance: existing,
      });
    }

    return NextResponse.json({ taken: false });
  } catch (err) {
    console.error(err);
    return NextResponse.json({ error: "Server error" }, { status: 500 });
  }
}
