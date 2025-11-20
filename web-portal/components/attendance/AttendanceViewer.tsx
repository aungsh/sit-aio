"use client";

import { useState, useEffect, useCallback } from "react";
import { AttendanceRecord } from "@/types/types";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";

import {
  Select,
  SelectTrigger,
  SelectContent,
  SelectItem,
  SelectValue,
} from "@/components/ui/select";

import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";

export default function AttendanceViewer() {
  const today = new Date().toISOString().split("T")[0];

  const [date, setDate] = useState(today);
  const [startTime, setStartTime] = useState("");
  const [endTime, setEndTime] = useState("");

  // ❗️ Use undefined instead of empty string
  const [selectedRoom, setSelectedRoom] = useState<string | undefined>(
    undefined
  );

  const [records, setRecords] = useState<AttendanceRecord[]>([]);
  const [rooms, setRooms] = useState<{ id: number; name: string }[]>([]);
  const [loading, setLoading] = useState(false);

  // Fetch rooms
  useEffect(() => {
    async function loadRooms() {
      const res = await fetch("/api/classrooms");
      const data = await res.json();
      setRooms(data);
    }
    loadRooms();
  }, []);

  // Fetch attendance
  const fetchAttendance = useCallback(async () => {
    if (!date) return;

    setLoading(true);

    const params = new URLSearchParams();
    params.set("date", date);

    if (startTime) params.set("start", startTime);
    if (endTime) params.set("end", endTime);
    if (selectedRoom) params.set("roomId", selectedRoom);

    const res = await fetch(`/api/attendance?${params.toString()}`);
    const data = await res.json();

    setRecords(data);
    setLoading(false);
  }, [date, startTime, endTime, selectedRoom]);

  useEffect(() => {
    Promise.resolve().then(() => {
      fetchAttendance();
    });
  }, [fetchAttendance]);

  return (
    <div className="space-y-6">
      {/* Filters */}
      <div>
        <h2 className="text-lg font-semibold mb-4">Filters</h2>

        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-5 gap-4">
          {/* Date */}
          <div className="flex flex-col space-y-1.5">
            <label className="text-sm font-medium">Date</label>
            <Input
              type="date"
              value={date}
              required
              onChange={(e) => setDate(e.target.value)}
            />
          </div>

          {/* Start Time */}
          <div className="flex flex-col space-y-1.5">
            <label className="text-sm font-medium">Start Time</label>
            <Input
              type="time"
              value={startTime}
              onChange={(e) => setStartTime(e.target.value)}
            />
          </div>

          {/* End Time */}
          <div className="flex flex-col space-y-1.5">
            <label className="text-sm font-medium">End Time</label>
            <Input
              type="time"
              value={endTime}
              onChange={(e) => setEndTime(e.target.value)}
            />
          </div>

          {/* Room Select */}
          <div className="flex flex-col space-y-1.5">
            <label className="text-sm font-medium">Room</label>
            <Select
              value={selectedRoom}
              onValueChange={(v) => setSelectedRoom(v)}
            >
              <SelectTrigger className="w-full">
                <SelectValue placeholder="All rooms" />{" "}
                {/* no empty SelectItem needed */}
              </SelectTrigger>
              <SelectContent>
                {rooms.map((room) => (
                  <SelectItem key={room.id} value={String(room.id)}>
                    {room.name}
                  </SelectItem>
                ))}
              </SelectContent>
            </Select>
          </div>

          {/* View Button */}
          <div className="flex items-end">
            <Button
              onClick={fetchAttendance}
              disabled={loading}
              className="w-full"
            >
              {loading ? "Loading..." : "View"}
            </Button>
          </div>
        </div>
      </div>

      {/* Table */}
      <div className="border rounded-lg p-4">
        {records.length === 0 ? (
          <p className="text-muted-foreground text-sm">No records found.</p>
        ) : (
          <Table>
            <TableHeader>
              <TableRow>
                <TableHead>Student</TableHead>
                <TableHead>Room</TableHead>
                <TableHead>Time</TableHead>
              </TableRow>
            </TableHeader>
            <TableBody>
              {records.map((rec) => (
                <TableRow key={rec.id}>
                  <TableCell>{rec.student?.name ?? rec.studentId}</TableCell>
                  <TableCell>{rec.room?.name ?? rec.roomId}</TableCell>
                  <TableCell>
                    {new Date(rec.joinedAt).toLocaleString()}
                  </TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        )}
      </div>
    </div>
  );
}
