// app/api/classrooms/route.ts
import { NextResponse } from "next/server";
import prisma from "@/lib/db";

export async function GET() {
  const classrooms = await prisma.classrooms.findMany({
    orderBy: { updatedAt: "desc" },
  });
  return NextResponse.json(classrooms);
}
