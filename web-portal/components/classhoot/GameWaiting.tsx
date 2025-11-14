"use client";

import PlayersList from "./PlayerList";
import { GameRoom } from "@/types/types";

export default function GameWaiting({ gameRoom }: { gameRoom: GameRoom }) {
  return (
    <section>
      <div>
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
