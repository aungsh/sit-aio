import AttendanceViewer from "@/components/attendance/AttendanceViewer";

export default function AttendancePage() {
  return (
    <div className="">
      <h1 className="text-4xl font-bold mb-4">Attendance Records</h1>
      <p className="text-lg text-gray-600 mb-12">
        Select a date range to view attendance records.
      </p>

      <AttendanceViewer />
    </div>
  );
}
