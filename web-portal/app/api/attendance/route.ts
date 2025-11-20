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

export async function GET(req: Request) {
  const { searchParams } = new URL(req.url);

  const date = searchParams.get("date");
  const start = searchParams.get("start"); // optional
  const end = searchParams.get("end"); // optional

  if (!date) {
    return NextResponse.json([], { status: 200 });
  }

  // Build timestamps
  const startTime = start ? `${date}T${start}:00` : `${date}T00:00:00`;
  const endTime = end ? `${date}T${end}:59` : `${date}T23:59:59`;

  const records = await prisma.attendance.findMany({
    where: {
      joinedAt: {
        gte: new Date(startTime),
        lte: new Date(endTime),
      },
    },
    include: {
      student: true,
      room: true,
    },
    orderBy: { joinedAt: "desc" },
  });

  return NextResponse.json(records);
}
