"use client";

import { useEffect, useState } from "react";

type PlayerScore = {
  id: number;
  score: number;
  player: {
    id: number;
    name: string;
  };
};

export default function GameEnded({ code }: { code: string }) {
  const [players, setPlayers] = useState<PlayerScore[]>([]);

  useEffect(() => {
    async function fetchResults() {
      const res = await fetch(`/api/classhoot/gameroom/results?code=${code}`, {
        cache: "no-store",
      });
      const data = await res.json();
      setPlayers(data);
    }
    fetchResults();
  }, [code]);

  return (
    <div className="flex flex-col items-center p-8">
      <h1 className="text-3xl font-bold mb-6">Game Results</h1>

      <div className="w-full max-w-xl space-y-4">
        {players.map((pr) => {
          const initial = pr.player.name.charAt(0).toUpperCase();

          return (
            <div
              key={pr.id}
              className="flex items-center justify-between p-4 bg-gray-100 rounded-xl shadow"
            >
              <div className="flex items-center gap-3">
                <div className="w-10 h-10 rounded-full bg-blue-500 text-white flex items-center justify-center text-lg font-semibold">
                  {initial}
                </div>

                <span className="text-lg font-medium">{pr.player.name}</span>
              </div>

              <span className="text-xl font-bold">{pr.score}</span>
            </div>
          );
        })}
      </div>
    </div>
  );
}
