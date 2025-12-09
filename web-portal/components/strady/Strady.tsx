"use client";

import { useEffect, useMemo, useState } from "react";
import {
  LineChart,
  Line,
  CartesianGrid,
  XAxis,
  YAxis,
  Tooltip,
  ResponsiveContainer,
} from "recharts";
import { Card, CardContent } from "../ui/card";

type RecentSession = {
  id: number;
  startTime: string;
  durationMin: number;
  name: string;
};

type MultiDaily = {
  date: string;
  [player: string]: string | number;
};

type PlayerStats = {
  name: string;
  totalMinutes: number;
  avgMinutes: number;
  longestSession: number;
  streak: number;
};

export default function Strady() {
  const [recent, setRecent] = useState<RecentSession[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    async function loadStats() {
      const res = await fetch("/api/strady/stats");
      const data = await res.json();
      setRecent(data.recent);
      setLoading(false);
    }
    loadStats();
  }, []);

  // ============================
  // PROCESS PLAYER STATS
  // ============================
  const players = useMemo(() => {
    const map: Record<string, PlayerStats> = {};

    recent.forEach((s) => {
      if (!map[s.name]) {
        map[s.name] = {
          name: s.name,
          totalMinutes: 0,
          avgMinutes: 0,
          longestSession: 0,
          streak: 0,
        };
      }
      map[s.name].totalMinutes += s.durationMin;
      map[s.name].longestSession = Math.max(
        map[s.name].longestSession,
        s.durationMin
      );
    });

    for (const p of Object.values(map)) {
      const count = recent.filter((r) => r.name === p.name).length;
      p.avgMinutes = Math.round(p.totalMinutes / count);
    }

    // streak calc
    const byUser: Record<string, string[]> = {};
    recent.forEach((s) => {
      const d = s.startTime.split("T")[0];
      byUser[s.name] ||= [];
      if (!byUser[s.name].includes(d)) byUser[s.name].push(d);
    });

    for (const name in byUser) {
      const days = byUser[name].sort();
      let streak = 1;
      for (let i = days.length - 1; i > 0; i--) {
        const diff =
          (new Date(days[i]).getTime() - new Date(days[i - 1]).getTime()) /
          (1000 * 60 * 60 * 24);
        if (diff === 1) streak++;
        else break;
      }
      map[name].streak = streak;
    }

    return Object.values(map);
  }, [recent]);

  const topTotal = [...players].sort((a, b) => b.totalMinutes - a.totalMinutes);
  const topLongest = [...players].sort(
    (a, b) => b.longestSession - a.longestSession
  );
  const topAvg = [...players].sort((a, b) => b.avgMinutes - a.avgMinutes);
  const topStreak = [...players].sort((a, b) => b.streak - a.streak);

  // ============================
  // MULTI PERSON DAILY CHART
  // ============================
  const dailyMulti = useMemo(() => {
    const map: Record<string, MultiDaily> = {};
    recent.forEach((s) => {
      const date = s.startTime.split("T")[0];
      if (!map[date]) map[date] = { date };
      map[date][s.name] = ((map[date][s.name] as number) || 0) + s.durationMin;
    });
    return Object.values(map).sort((a, b) => a.date.localeCompare(b.date));
  }, [recent]);

  const playerNames = players.map((p) => p.name);

  if (loading) return <p className="text-center py-10">Loading...</p>;

  // ============================
  // LEADERBOARD ROW (BOOMZ STYLE)
  // ============================
  const BoomRow = ({
    rank,
    name,
    value,
    unit,
    percent,
  }: {
    rank: number;
    name: string;
    value: number;
    unit: string;
    percent: number;
  }) => {
    const glow =
      rank === 1
        ? "from-yellow-400 to-yellow-600 shadow-yellow-400/60"
        : rank === 2
        ? "from-gray-300 to-gray-500 shadow-gray-400/60"
        : rank === 3
        ? "from-orange-400 to-orange-600 shadow-orange-400/60"
        : "from-gray-200 to-gray-300";

    return (
      <div className="relative overflow-hidden rounded-xl bg-gradient-to-r p-[2px] my-2">
        <div className="rounded-xl bg-white px-4 py-3 flex items-center justify-between">
          {/* Rank Badge */}
          <div
            className={`flex items-center justify-center w-10 h-10 rounded-full font-bold text-white bg-gradient-to-br ${glow} shadow-lg`}
          >
            {rank}
          </div>

          {/* Name */}
          <div className="flex items-center gap-3 w-40">
            <div className="w-10 h-10 flex items-center justify-center rounded-full bg-black text-white font-bold">
              {name.charAt(0).toUpperCase()}
            </div>
            <span className="font-semibold text-lg">{name}</span>
          </div>

          {/* Value */}
          <div className="font-bold text-xl pr-4">
            {value} <span className="text-gray-600 text-sm">{unit}</span>
          </div>
        </div>

        {/* Progress bar */}
        <div
          className={`absolute left-0 bottom-0 h-1 bg-gradient-to-r ${glow}`}
          style={{ width: `${percent}%` }}
        ></div>
      </div>
    );
  };

  // ============================
  // MAIN UI
  // ============================
  return (
    <div className="px-4 py-10 space-y-16">
      {/* ================= HERO ================= */}
      <section className="text-center space-y-3">
        <h1 className="text-5xl font-extrabold tracking-tight drop-shadow-sm">
          ⚡ STRADY ARENA ⚡
        </h1>
        <p className="text-gray-600 text-lg">Compete. Grind. Dominate. 📚🔥</p>
      </section>

      {/* ================= PODIUM ================= */}
      <section className="max-w-3xl mx-auto">
        <h2 className="text-3xl font-bold mb-6 text-center">
          🏆 Champions Podium
        </h2>

        <div className="flex items-end justify-center gap-6 mt-8">
          {/* 2nd Place */}
          {topTotal[1] && (
            <div className="flex flex-col items-center">
              <div className="text-4xl">🥈</div>
              <div className="w-20 h-20 rounded-full bg-gray-800 text-white flex items-center justify-center text-3xl font-bold">
                {topTotal[1].name.charAt(0)}
              </div>
              <p className="font-semibold">{topTotal[1].name}</p>
              <p className="text-sm text-gray-500">
                {topTotal[1].totalMinutes} min
              </p>
            </div>
          )}

          {/* 1st Place */}
          {topTotal[0] && (
            <div className="flex flex-col items-center transform -translate-y-8">
              <div className="text-6xl">🥇</div>
              <div className="w-28 h-28 rounded-full bg-yellow-500 text-white flex items-center justify-center text-4xl font-extrabold ring-4 ring-yellow-300 shadow-xl">
                {topTotal[0].name.charAt(0)}
              </div>
              <p className="font-bold text-xl mt-2">{topTotal[0].name}</p>
              <p className="text-gray-700 font-semibold">
                {topTotal[0].totalMinutes} min
              </p>
            </div>
          )}

          {/* 3rd Place */}
          {topTotal[2] && (
            <div className="flex flex-col items-center">
              <div className="text-4xl">🥉</div>
              <div className="w-20 h-20 rounded-full bg-orange-700 text-white flex items-center justify-center text-3xl font-bold">
                {topTotal[2].name.charAt(0)}
              </div>
              <p className="font-semibold">{topTotal[2].name}</p>
              <p className="text-sm text-gray-500">
                {topTotal[2].totalMinutes} min
              </p>
            </div>
          )}
        </div>
      </section>

      {/* ================= BADGE ROW ================= */}
      <section className="grid grid-cols-2 md:grid-cols-4 gap-4">
        <div className="p-4 bg-black text-white rounded-2xl text-center shadow-lg">
          <p className="text-4xl">🔥</p>
          <p className="font-bold">Longest Session</p>
          <p className="text-2xl font-extrabold">
            {topLongest[0]?.longestSession} min
          </p>
          <p className="text-sm text-gray-300">{topLongest[0]?.name}</p>
        </div>

        <div className="p-4 bg-blue-600 text-white rounded-2xl text-center shadow-lg">
          <p className="text-4xl">⚡</p>
          <p className="font-bold">Best Avg Session</p>
          <p className="text-2xl font-extrabold">{topAvg[0]?.avgMinutes} min</p>
          <p className="text-sm text-blue-200">{topAvg[0]?.name}</p>
        </div>

        <div className="p-4 bg-green-600 text-white rounded-2xl text-center shadow-lg">
          <p className="text-4xl">📅</p>
          <p className="font-bold">Longest Streak</p>
          <p className="text-2xl font-extrabold">{topStreak[0]?.streak} days</p>
          <p className="text-sm text-green-200">{topStreak[0]?.name}</p>
        </div>

        <div className="p-4 bg-pink-600 text-white rounded-2xl text-center shadow-lg">
          <p className="text-4xl">🏆</p>
          <p className="font-bold">Total Time King</p>
          <p className="text-2xl font-extrabold">
            {topTotal[0]?.totalMinutes} min
          </p>
          <p className="text-sm text-pink-200">{topTotal[0]?.name}</p>
        </div>
      </section>

      {/* ================= SCROLL LEADERBOARDS ================= */}
      <section className="space-y-12">
        {[
          {
            title: "Total Time Champions",
            data: topTotal,
            key: "totalMinutes",
            unit: "min",
          },
          {
            title: "Longest Single Session",
            data: topLongest,
            key: "longestSession",
            unit: "min",
          },
          {
            title: "Best Average Session",
            data: topAvg,
            key: "avgMinutes",
            unit: "min avg",
          },
          {
            title: "Study Streak Masters",
            data: topStreak,
            key: "streak",
            unit: "days",
          },
        ].map((lb) => (
          <div key={lb.title} className="space-y-3">
            <h2 className="text-2xl font-bold">{lb.title}</h2>

            <div className="flex overflow-x-auto gap-4 pb-3">
              {lb.data.map((p, idx) => (
                <div
                  key={p.name}
                  className="min-w-[260px] p-4 rounded-2xl border bg-white shadow-sm"
                >
                  <p className="text-3xl">
                    {idx === 0
                      ? "👑"
                      : idx === 1
                      ? "🥈"
                      : idx === 2
                      ? "🥉"
                      : "🔥"}
                  </p>
                  <p className="font-bold text-xl">{p.name}</p>
                  <p className="text-gray-600">
                    {p[lb.key as keyof PlayerStats]} {lb.unit}
                  </p>
                </div>
              ))}
            </div>
          </div>
        ))}
      </section>

      {/* ================= MULTI-LINE CHART ================= */}
      <section>
        <h2 className="text-3xl font-bold mb-4">📊 Study Minutes Per Day</h2>
        <Card className="shadow-md">
          <CardContent className="pt-8">
            <ResponsiveContainer width="100%" height={320}>
              <LineChart data={dailyMulti}>
                <CartesianGrid strokeDasharray="3 3" />
                <XAxis dataKey="date" />
                <YAxis />
                <Tooltip />

                {playerNames.map((name, i) => (
                  <Line
                    key={name}
                    type="monotone"
                    dataKey={name}
                    stroke={`hsl(${(i * 70) % 360}, 80%, 40%)`}
                    strokeWidth={3}
                    dot={false}
                  />
                ))}
              </LineChart>
            </ResponsiveContainer>
          </CardContent>
        </Card>
      </section>

      {/* ================= RECENT STUDY SESSIONS ================= */}
      <section>
        <h2 className="text-3xl font-bold mb-4">🕒 Recent Study Sessions</h2>

        <div className="space-y-4">
          {recent.slice(0, 5).map((s) => (
            <div
              key={s.id}
              className="p-4 rounded-xl border bg-white shadow-sm flex items-center justify-between"
            >
              <div>
                <p className="font-bold text-lg">{s.name}</p>
                <p className="text-gray-500 text-sm">
                  {new Date(s.startTime).toLocaleString()}
                </p>
              </div>

              <div className="text-right">
                <p className="text-xl font-extrabold">{s.durationMin} min</p>
                <p className="text-gray-500 text-sm">Duration</p>
              </div>
            </div>
          ))}
        </div>
      </section>
    </div>
  );
}