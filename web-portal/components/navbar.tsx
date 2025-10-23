"use client";

import { useState } from "react";
import Link from "next/link";
import { Menu, X } from "lucide-react";

const navLinks = [
  { name: "Home", href: "/" },
  { name: "Room Vacancy", href: "/room-vacancy" },
  { name: "Attendance", href: "/attendance" },
  { name: "Hoot Ka", href: "/hoot-ka" },
  { name: "Documentation", href: "/documentation" },
  { name: "About Us", href: "/about" },
];

const info = ["Singapore Institute of Technology", "ICT1011", "Group P2N"];

export default function Navbar() {
  const [menuOpen, setMenuOpen] = useState(false);

  return (
    <nav className="fixed top-0 left-0 right-0 z-50 bg-white/80 backdrop-blur-md border-b border-gray-100">
      <div className="max-w-7xl mx-auto px-4 sm:px-6">
        <div className="flex justify-between items-center h-16">
          {/* Left: Info */}
          <div className="flex flex-col sm:flex-row sm:items-center sm:space-x-4 text-xs sm:text-sm text-gray-600">
            <span className="font-semibold text-gray-800 tracking-tight">
              {info[0]}
            </span>
            <span className="hidden lg:inline text-gray-400">|</span>
            <span className="hidden lg:inline">{info[1]}</span>
            <span className="hidden lg:inline text-gray-400">|</span>
            <span className="hidden lg:inline">{info[2]}</span>
          </div>

          {/* Right: Nav links (desktop) */}
          <div className="hidden lg:flex items-center space-x-8">
            {navLinks.map((link) => (
              <Link key={link.name} href={link.href}>
                <span className="text-gray-700 font-medium tracking-tight hover:text-[#ff006e] transition-colors duration-150 cursor-pointer">
                  {link.name}
                </span>
              </Link>
            ))}
          </div>

          {/* Mobile Menu Button */}
          <button
            className="lg:hidden text-gray-700 p-2"
            onClick={() => setMenuOpen(!menuOpen)}
            aria-label="Toggle menu"
          >
            {menuOpen ? <X size={24} /> : <Menu size={24} />}
          </button>
        </div>
      </div>

      {/* Mobile Menu Dropdown */}
      <div
        className={`lg:hidden bg-white border-t border-gray-100 overflow-hidden transition-all duration-300 ${
          menuOpen ? "max-h-40 opacity-100" : "max-h-0 opacity-0"
        }`}
      >
        <div className="flex flex-col items-center space-y-3 py-3">
          {navLinks.map((link) => (
            <Link
              key={link.name}
              href={link.href}
              onClick={() => setMenuOpen(false)}
            >
              <span className="text-gray-700 font-medium tracking-tight hover:text-[#ff006e] hover:font-semibold transition-colors duration-150 cursor-pointer">
                {link.name}
              </span>
            </Link>
          ))}
        </div>
      </div>
    </nav>
  );
}
