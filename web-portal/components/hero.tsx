"use client";
import Link from "next/link";
import { ArrowRight } from "lucide-react";

export default function Hero() {
  return (
    <section className="">
      <div className="max-w-4xl mx-auto px-4 text-center">
        {/* Subtitle */}
        <div className="mb-4 sm:mb-6">
          <p className="text-sm sm:text-base text-gray-500 tracking-widest uppercase font-medium">
            Welcome to
          </p>
        </div>

        {/* Main Heading */}
        <h1 className="text-6xl sm:text-7xl md:text-9xl font-bold tracking-tight text-gray-900 mb-3 sm:mb-4">
          SIT AIO
        </h1>

        {/* Description */}
        <p className="text-xl sm:text-2xl md:text-3xl text-gray-600 font-light tracking-tight mb-6 sm:mb-8">
          SIT All in One
        </p>

        {/* Tagline */}
        <p className="text-base sm:text-lg md:text-xl text-gray-500 font-normal max-w-2xl mx-auto mb-10 leading-relaxed">
          Your centralized platform for room vacancy tracking, attendance
          management, and campus information—all in one place.
        </p>

        {/* CTA Buttons – Centered */}
        <div className="flex flex-col sm:flex-row items-center justify-center gap-4 sm:gap-6">
          <Link href="/room-vacancy">
            <button className="group px-8 py-4 bg-[#ff006e] text-white font-semibold tracking-tight rounded-lg hover:bg-[#e6006a] transition-all duration-200 flex items-center gap-2">
              Get Started
              <ArrowRight
                size={20}
                className="group-hover:translate-x-1 transition-transform duration-200"
              />
            </button>
          </Link>

          <Link
            href="https://github.com/aungsh/sit-aio"
            target="_blank"
            rel="noopener noreferrer"
          >
            <button className="px-8 py-4 border-gray-300 text-gray-700 font-semibold tracking-tight rounded-lg hover:border-gray-400 hover:bg-gray-50 transition-all duration-200">
              Visit Github
            </button>
          </Link>
        </div>

        {/* YouTube Video – Centered */}
        <div className="my-12 flex justify-center">
          <div className="aspect-video w-full max-w-3xl rounded-xl overflow-hidden shadow-md border">
            <iframe
              src="https://www.youtube.com/embed/_a1zw_Cxuao?start=9"
              title="SIT AIO Demo"
              className="w-full h-full"
              allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture"
              allowFullScreen
            ></iframe>
          </div>
        </div>

        {/* Quick Links – Centered */}
        <div className="my-16 pt-12 border-t border-gray-200">
          <p className="text-xs sm:text-sm text-gray-400 uppercase tracking-widest mb-6 font-medium">
            Quick Access
          </p>

          <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 sm:gap-6 mx-auto max-w-3xl">
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
            <Link href="/classhoot">

              <div className="p-6 border border-gray-200 rounded-lg hover:border-[#ff006e] hover:shadow-md transition-all duration-200 cursor-pointer group">
                <h3 className="text-lg font-semibold text-gray-800 mb-2 group-hover:text-[#ff006e] transition-colors">
                  Class Hoot
                </h3>
                <p className="text-sm text-gray-500">
                  Interactive quizzes and polls
                </p>
              </div>
            </Link>
          </div>
        </div>
      </div>
    </section>
  );
}
