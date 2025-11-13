"use client";

import { useEffect, useState } from "react";

type Room = {
  id: number;
  name: string;
  vacancy: "UNKNOWN" | "OCCUPIED" | "VACANT";
  updatedAt: string;
};

export default function RoomVacancyPage() {
  const [rooms, setRooms] = useState<Room[]>([]);

  // Fetch initial data
  useEffect(() => {
    fetch("/api/classrooms")
      .then((res) => res.json())
      .then((data) => setRooms(data));
  }, []);

  // Subscribe to real-time updates via SSE
  useEffect(() => {
    const eventSource = new EventSource("/api/room-events");

    eventSource.onmessage = (event) => {
      if (event.data !== "connected") {
        const updatedRoom: Room = JSON.parse(event.data);
        setRooms((prev) =>
          prev.map((r) => (r.id === updatedRoom.id ? updatedRoom : r))
        );
      }
    };

    return () => eventSource.close();
  }, []);

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
            <p className="text-gray-500 col-span-full">No rooms found.</p>
          )}
        </div>
      </div>
    </section>
  );
}
