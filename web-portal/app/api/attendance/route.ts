import { NextResponse } from "next/server";
import prisma from "@/lib/db";

export async function POST(req: Request) {
  try {
    const { studentId, studentName, classroomName } = await req.json();

    // find classroom
    const classroom = await prisma.classrooms.findUnique({
      where: { name: classroomName },
    });

    if (!classroom) {
      return NextResponse.json(
        { error: "Classroom not found" },
        { status: 404 }
      );
    }

    // create student if not exists
    let student = await prisma.player.findUnique({
      where: { id: studentId },
    });

    if (!student) {
      student = await prisma.player.create({
        data: { id: studentId, name: studentName },
      });
    }

    // create attendance record
    const attendance = await prisma.attendance.create({
      data: {
        studentId: student.id,
        roomId: classroom.id,
      },
    });

    return NextResponse.json(attendance);
  } catch (err) {
    console.error(err);
    return NextResponse.json({ error: "Server error" }, { status: 500 });
  }
}
