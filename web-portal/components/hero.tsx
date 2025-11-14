"use client";
import Link from "next/link";
import { ArrowRight } from "lucide-react";

export default function Hero() {
  return (
    <section className="">
      <div className="">
        {/* Subtitle */}
        <div className="mb-6 sm:mb-8">
          <p className="text-sm sm:text-base text-gray-500 tracking-widest uppercase font-medium">
            Welcome to
          </p>
        </div>

        {/* Main Heading */}
        <h1 className="text-5xl sm:text-7xl md:text-8xl lg:text-9xl font-bold tracking-tight text-gray-900 mb-4 sm:mb-6">
          SIT AIO
        </h1>

        {/* Description */}
        <p className="text-xl sm:text-2xl md:text-3xl text-gray-600 font-light tracking-tight mb-8 sm:mb-12">
          SIT All in One
        </p>

        {/* Tagline */}
        <p className="text-base sm:text-lg md:text-xl text-gray-500 font-normal max-w-2xl mb-12 sm:mb-16 leading-relaxed">
          Your centralized platform for room vacancy tracking, attendance
          management, and campus information—all in one place.
        </p>

        {/* CTA Buttons */}
        <div className="flex flex-col sm:flex-row items-start gap-4 sm:gap-6">
          <Link href="/room-vacancy">
            <button className="group w-full sm:w-auto px-8 py-4 bg-[#ff006e] text-white font-semibold tracking-tight rounded-lg hover:bg-[#e6006a] transition-all duration-200 flex items-center justify-center gap-2">
              Get Started
              <ArrowRight
                size={20}
                className="group-hover:translate-x-1 transition-transform duration-200"
              />
            </button>
          </Link>
          <Link href="/about">
            <button className="w-full sm:w-auto px-8 py-4 border-gray-300 text-gray-700 font-semibold tracking-tight rounded-lg hover:border-gray-400 hover:bg-gray-50 transition-all duration-200">
              Visit Github
            </button>
          </Link>
        </div>

        {/* Quick Links */}
        <div className="mt-16 sm:mt-24 pt-12 sm:pt-16 border-t border-gray-200">
          <p className="text-xs sm:text-sm text-gray-400 uppercase tracking-widest mb-6 sm:mb-8 font-medium">
            Quick Access
          </p>
          <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 sm:gap-6 max-w-3xl mx-auto">
            <Link href="/room-vacancy">
              <div className="p-6 border border-gray-200 rounded-lg hover:border-[#ff006e] hover:shadow-md transition-all duration-200 cursor-pointer group">
                <h3 className="text-lg font-semibold text-gray-800 mb-2 group-hover:text-[#ff006e] transition-colors">
                  Room Vacancy
                </h3>
                <p className="text-sm text-gray-500">
                  Check available rooms in real-time
                </p>
              </div>
            </Link>
            <Link href="/attendance">
              <div className="p-6 border border-gray-200 rounded-lg hover:border-[#ff006e] hover:shadow-md transition-all duration-200 cursor-pointer group">
                <h3 className="text-lg font-semibold text-gray-800 mb-2 group-hover:text-[#ff006e] transition-colors">
                  Attendance
                </h3>
                <p className="text-sm text-gray-500">
                  Track and manage attendance
                </p>
              </div>
            </Link>
            <Link href="/hoot-ka">
              <div className="p-6 border border-gray-200 rounded-lg hover:border-[#ff006e] hover:shadow-md transition-all duration-200 cursor-pointer group">
                <h3 className="text-lg font-semibold text-gray-800 mb-2 group-hover:text-[#ff006e] transition-colors">
                  Class Hoot
                </h3>
                <p className="text-sm text-gray-500">
                  Engage in interactive quizzes and polls
                </p>
              </div>
            </Link>
          </div>
        </div>
      </div>
    </section>
  );
}
