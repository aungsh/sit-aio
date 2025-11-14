"use client";

import { useEffect, useState } from "react";
import Pusher from "pusher-js";

interface Player {
  id: string;
  name: string;
}

interface PlayersListProps {
  roomCode: string;
  initialPlayers: Player[];
}

export default function PlayersList({
  roomCode,
  initialPlayers,
}: PlayersListProps) {
  const [players, setPlayers] = useState<Player[]>(initialPlayers);

  useEffect(() => {
    // Initialize Pusher client
    const pusher = new Pusher(process.env.NEXT_PUBLIC_PUSHER_KEY!, {
      cluster: "ap1",
    });

    // Subscribe to the game room channel
    const channel = pusher.subscribe(`game-room-${roomCode}`);

    // Listen for player-joined events
    channel.bind("player-joined", (data: { player: Player }) => {
      setPlayers((prevPlayers) => {
        // Check if player already exists to avoid duplicates
        const playerExists = prevPlayers.some((p) => p.id === data.player.id);
        if (playerExists) {
          return prevPlayers;
        }
        return [...prevPlayers, data.player];
      });
    });

    // Cleanup on unmount
    return () => {
      channel.unbind_all();
      channel.unsubscribe();
      pusher.disconnect();
    };
  }, [roomCode]);

  return (
    <div className="">
      <p className="text-sm text-gray-600 mb-3">Players ({players.length})</p>

      {players.length === 0 ? (
        <p className="text-gray-400 italic">Waiting for players to join...</p>
      ) : (
        <div className="space-y-2">
          {players.map((player) => (
            <div key={player.id} className="flex items-center gap-2">
              <div className="w-8 h-8 bg-blue-500 rounded-full flex items-center justify-center text-white font-semibold">
                {player.name.charAt(0).toUpperCase()}
              </div>
              <span className="font-medium text-gray-800">{player.name}</span>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
