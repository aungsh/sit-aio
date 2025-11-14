import { NextResponse } from "next/server";
import prisma from "@/lib/db";

export async function PATCH(req: Request) {
  const { code } = await req.json();

  const room = await prisma.gameRoom.update({
    where: { code },
    data: { status: "ONGOING" },
  });

  return NextResponse.json(room);
}
