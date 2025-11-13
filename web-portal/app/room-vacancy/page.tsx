import prisma from "@/lib/db";

export default async function RoomVacancyPage() {
  // Fetch all classrooms from Prisma
  const classrooms = await prisma.classrooms.findMany({
    orderBy: { updatedAt: "desc" },
  });

  return (
    <section className="">
      <div className="">
        {/* Title */}
        <h1 className="text-4xl sm:text-6xl font-bold tracking-tight text-gray-900 mb-4">
          Room Vacancy
        </h1>
        <p className="text-lg sm:text-xl text-gray-600 font-light mb-12">
          Check the current room status and last updated time.
        </p>

        {/* Rooms Grid */}
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-6">
          {classrooms.length > 0 ? (
            classrooms.map((room) => (
              <div
                key={room.id}
                className="p-6 border border-gray-200 rounded-xl bg-white shadow-sm hover:shadow-md transition-all duration-200 text-left group"
              >
                <h3 className="text-xl font-semibold text-gray-800 mb-2 group-hover:text-[#ff006e] transition-colors">
                  {room.name}
                </h3>
                <p
                  className={`text-sm font-medium mb-3 ${
                    room.vacancy === "VACANT"
                      ? "text-green-600"
                      : room.vacancy === "OCCUPIED"
                      ? "text-red-500"
                      : "text-gray-400"
                  }`}
                >
                  {room.vacancy === "VACANT"
                    ? "Vacant"
                    : room.vacancy === "OCCUPIED"
                    ? "Occupied"
                    : "Unknown"}
                </p>
                <p className="text-xs text-gray-500">
                  Last updated:{" "}
                  {new Date(room.updatedAt).toLocaleString("en-SG", {
                    dateStyle: "medium",
                    timeStyle: "short",
                  })}
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
