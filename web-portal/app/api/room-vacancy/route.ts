import { NextResponse } from "next/server";
import prisma from "@/lib/db";
import { pusherServer } from "@/lib/pusher";

export async function PATCH(request: Request) {
  try {
    const { roomId, status } = await request.json();

    if (!roomId || !["UNKNOWN", "OCCUPIED", "VACANT"].includes(status)) {
      return NextResponse.json(
        { error: "Invalid roomId or status" },
        { status: 400 }
      );
    }

    const updatedRoom = await prisma.classrooms.update({
      where: { id: roomId },
      data: {
        vacancy: status,
        updatedAt: new Date(),
      },
    });

    await pusherServer.trigger("room-channel", "room-updated", updatedRoom);

    return NextResponse.json(updatedRoom, { status: 200 });
  } catch (error) {
    console.error("Error updating room vacancy status:", error);
    return NextResponse.json(
      { error: "Internal Server Error" },
      { status: 500 }
    );
  }
}
