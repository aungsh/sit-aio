import { notFound } from "next/navigation";
import PlayersList from "@/components/pusher/PlayerList";

async function getGameRoom(code: string) {
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

  // Handle case where game room doesn't exist
  if (!gameRoom) {
    notFound();
  }

  return (
    <section className="">
      <div className="">
        <h1 className="text-4xl font-bold mb-4">{gameRoom.title}</h1>
        <p className="text-lg text-gray-600 mb-12">
          Get ready to play! Share the room code to join the game.
        </p>

        <div className="mb-12">
          <p className="text-sm text-gray-600 mb-1">Room Code</p>
          <h1 className="text-9xl font-black">{gameRoom.code}</h1>
        </div>

        {/* Room Status */}
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
          <div className="bg-gray-50 rounded-lg p-4">
            <p className="text-sm text-gray-600 mb-1">Status</p>
            <p
              className={`font-semibold text-lg ${
                gameRoom.status === "WAITING"
                  ? "text-yellow-600"
                  : gameRoom.status === "ONGOING"
                  ? "text-green-600"
                  : "text-gray-600"
              }`}
            >
              {gameRoom.status}
            </p>
          </div>

          <div className="bg-gray-50 rounded-lg p-4">
            <p className="text-sm text-gray-600 mb-1">Total Questions</p>
            <p className="font-semibold text-lg">{gameRoom.totalQuestions}</p>
          </div>
        </div>

        {/* Full Players Section Below */}
        <div className="mt-8">
          <h2 className="text-2xl font-bold mb-4">Players in Room</h2>
          <PlayersList
            roomCode={gameRoom.code}
            initialPlayers={gameRoom.players || []}
          />
        </div>
      </div>
    </section>
  );
}
