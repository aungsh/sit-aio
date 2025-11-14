"use client";
import Link from "next/link";

const navLinks = [
  { name: "Home", href: "/" },
  { name: "Room Vacancy", href: "/room-vacancy" },
  { name: "Attendance", href: "/attendance" },
  { name: "Class Hoot", href: "/classhoot" },
  { name: "About Us", href: "/about" },
];

const info = ["Singapore Institute of Technology", "ICT1011", "Group P2N"];

export default function Footer() {
  return (
    <footer className="bg-white border-t border-gray-100">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 py-12">
        {/* Top Section */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-8 mb-8">
          {/* Left: Brand */}
          <div>
            <h3 className="text-2xl font-bold tracking-tight text-gray-900 mb-2">
              SIT AIO
            </h3>
            <p className="text-sm text-gray-600 mb-4">SIT All in One</p>
            <p className="text-xs text-gray-500 leading-relaxed">
              Your centralized platform for room vacancy tracking, attendance
              management, and campus information.
            </p>
          </div>

          {/* Middle: Quick Links */}
          <div>
            <h4 className="text-sm font-semibold text-gray-800 tracking-tight uppercase mb-4">
              Quick Links
            </h4>
            <ul className="space-y-2">
              {navLinks.map((link) => (
                <li key={link.name}>
                  <Link href={link.href}>
                    <span className="text-sm text-gray-600 hover:text-[#ff006e] transition-colors duration-150 cursor-pointer">
                      {link.name}
                    </span>
                  </Link>
                </li>
              ))}
            </ul>
          </div>

          {/* Right: Project Info */}
          <div>
            <h4 className="text-sm font-semibold text-gray-800 tracking-tight uppercase mb-4">
              Project Info
            </h4>
            <ul className="space-y-2">
              {info.map((item, index) => (
                <li key={index}>
                  <span className="text-sm text-gray-600">{item}</span>
                </li>
              ))}
            </ul>
          </div>
        </div>

        {/* Bottom Section */}
        <div className="pt-8 border-t border-gray-100">
          <div className="flex flex-col sm:flex-row justify-between items-center gap-4">
            <p className="text-xs text-gray-500">
              © {new Date().getFullYear()} SIT AIO. All rights reserved.
            </p>
            <div className="flex items-center space-x-4 text-xs text-gray-500">
              <span className="font-semibold text-gray-800 tracking-tight">
                {info[0]}
              </span>
              <span className="text-gray-400">|</span>
              <span>{info[1]}</span>
              <span className="text-gray-400">|</span>
              <span>{info[2]}</span>
            </div>
          </div>
        </div>
      </div>
    </footer>
  );
}
