import { notFound } from "next/navigation";
import GameWaiting from "@/components/classhoot/GameWaiting";
import GameOngoingWrapper from "@/components/classhoot/GameOngoingWrapper";
import { GameRoom } from "@/types/types";

async function getGameRoom(code: string): Promise<GameRoom | null> {
  try {
    const baseUrl = process.env.NEXT_PUBLIC_APP_URL || "http://localhost:3000";
    const response = await fetch(
      `${baseUrl}/api/classhoot/gameroom?code=${code}`,
      { cache: "no-store" }
    );

    if (!response.ok) {
      return null;
    }

    return await response.json();
  } catch (error) {
    console.error("Error fetching game room:", error);
    return null;
  }
}

export default async function ClassHootRoomPage({
  params,
}: {
  params: { id: string };
}) {
  const { id } = await params;
  const gameRoom = await getGameRoom(id);

  if (!gameRoom) notFound();

  if (gameRoom.status === "WAITING") {
    return <GameWaiting gameRoom={gameRoom} />;
  } else if (gameRoom.status === "ONGOING") {
    // Pass only questions to the client wrapper
    return <GameOngoingWrapper code={gameRoom.code} />;
  } else {
    return <div>Game Ended</div>;
  }
}
