"use client";

import { useEffect, useState } from "react";
import Pusher from "pusher-js";

type Room = {
  id: number;
  name: string;
  vacancy: "UNKNOWN" | "OCCUPIED" | "VACANT";
  updatedAt: string;
};

// Initialize Pusher client (frontend)
const pusher = new Pusher(process.env.NEXT_PUBLIC_PUSHER_KEY!, {
  cluster: "ap1",
});

export default function RoomVacancyPage() {
  const [rooms, setRooms] = useState<Room[]>([]);

  // 1️⃣ Fetch initial data
  useEffect(() => {
    fetch("/api/classrooms")
      .then((res) => res.json())
      .then((data) => setRooms(data));
  }, []);

  // 2️⃣ Subscribe to Pusher for realtime updates
  useEffect(() => {
    const channel = pusher.subscribe("room-channel");

    channel.bind("room-updated", (updatedRoom: Room) => {
      setRooms((prev) =>
        prev.map((r) => (r.id === updatedRoom.id ? updatedRoom : r))
      );
    });

    return () => {
      channel.unbind_all();
      pusher.unsubscribe("room-channel");
    };
  }, []);

  // 3️⃣ Render UI
  return (
    <section className="">
      <div className="">
        <h1 className="text-4xl font-bold mb-4">Room Vacancy</h1>
        <p className="text-lg text-gray-600 mb-12">
          Realtime updates of room status
        </p>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-6">
          {rooms.length > 0 ? (
            rooms.map((room) => (
              <div
                key={room.id}
                className="p-6 border border-gray-200 rounded-xl bg-white shadow-sm"
              >
                <h3 className="text-xl font-semibold mb-2">{room.name}</h3>
                <p
                  className={`text-sm font-medium mb-3 ${
                    room.vacancy === "VACANT"
                      ? "text-green-600"
                      : room.vacancy === "OCCUPIED"
                      ? "text-red-500"
                      : "text-gray-400"
                  }`}
                >
                  {room.vacancy}
                </p>
                <p className="text-xs text-gray-500">
                  Last updated: {new Date(room.updatedAt).toLocaleString()}
                </p>
              </div>
            ))
          ) : (
            <p className="text-gray-500 col-span-full">Loading Rooms ...</p>
          )}
        </div>
      </div>
    </section>
  );
}
