// api route for students to join classhoot game room
// json
// {
//     "studentId": 2301234,
//     "studentName": "John",
//     "gameRoomCode": "230"
// }

import { NextResponse } from "next/server";
import prisma from "@/lib/db";
import { pusherServer } from "@/lib/pusher";

export async function POST(request: Request) {
  try {
    const { studentId, studentName, gameRoomCode } = await request.json();

    // Find the game room by code
    const gameRoom = await prisma.gameRoom.findUnique({
      where: { code: gameRoomCode },
    });

    if (!gameRoom) {
      return NextResponse.json(
        { message: "Game room not found" },
        { status: 404 }
      );
    }

    // Upsert player (create if not exists)
    const player = await prisma.player.upsert({
      where: { id: studentId },
      update: {}, // do nothing if exists
      create: {
        id: studentId,
        name: studentName,
      },
    });

    // Connect or create playerRoom
    const playerRoom = await prisma.playerRoom.upsert({
      where: {
        playerId_roomId: { playerId: player.id, roomId: gameRoom.id }, // make sure you have a unique compound index
      },
      update: {}, // already exists, do nothing
      create: {
        playerId: player.id,
        roomId: gameRoom.id,
      },
    });

    const message = playerRoom
      ? "Student joined the game room"
      : "Student already joined";

    // Trigger Pusher event to notify others
    await pusherServer.trigger(`game-room-${gameRoom.code}`, "player-joined", {
      player: {
        id: player.id,
        name: player.name,
      },
    });

    return NextResponse.json(
      { message, gameRoom, player, playerRoom },
      { status: 200 }
    );
  } catch (error) {
    console.error("Error joining game room:", error);
    return NextResponse.json(
      { message: "Internal server error" },
      { status: 500 }
    );
  }
}
