"use client";

import { useState, useEffect } from "react";
import Pusher from "pusher-js";
import { Question } from "@/types/types";

type GameOngoingProps = {
  questions: Question[];
  currentIndex: number;
  code: string;
  onNextQuestion: () => void;
};

type StudentAnswer = {
  studentId: number;
  studentName: string;
  optionSelected: number;
};

// Helper: get first letter for avatar
const getInitial = (name: string) => name.charAt(0).toUpperCase();

export default function GameOngoing({
  questions,
  currentIndex,
  code,
  onNextQuestion,
}: GameOngoingProps) {
  const [showAnswer, setShowAnswer] = useState(false);
  const [answers, setAnswers] = useState<StudentAnswer[]>([]);
  const question = questions[currentIndex];

  useEffect(() => {
    const pusher = new Pusher(process.env.NEXT_PUBLIC_PUSHER_KEY!, {
      cluster: "ap1",
    });

    const channel = pusher.subscribe(`game-${code}-${currentIndex}`);
    channel.bind("answer-submitted", (data: StudentAnswer) => {
      setAnswers((prev) => [...prev, data]);
    });

    return () => {
      channel.unbind_all();
      channel.unsubscribe();
    };
  }, [code, currentIndex]);

  if (!question) return <div>No question found</div>;

  const choices = [
    question.choice1,
    question.choice2,
    question.choice3,
    question.choice4,
  ];

  const handleShowAnswer = () => setShowAnswer(true);
  const handleNextQuestion = () => {
    setShowAnswer(false);
    setAnswers([]);
    onNextQuestion();
  };

  const answersByChoice = [[], [], [], []] as StudentAnswer[][];
  answers.forEach((a) => {
    answersByChoice[a.optionSelected - 1].push(a);
  });

  return (
    <div className="">
      {/* QUESTION */}
      <div className="mb-12">
        <h1 className="text-4xl font-bold tracking-tight leading-tight mb-3">
          {question.text}
        </h1>
        <p className="text-base text-gray-500">Question {currentIndex + 1}</p>
      </div>

      {/* ANSWER GRID */}
      <div className="grid grid-cols-2 gap-2 mb-16">
        {choices.map((choice, idx) => {
          const choiceNumber = idx + 1;
          const isCorrect =
            showAnswer && choiceNumber === question.correctChoice;

          return (
            <div
              key={idx}
              className={`
                flex flex-col w-full min-h-50 justify-between transition-all border-2 rounded-xl p-2 cursor-pointer
                ${
                  showAnswer
                    ? isCorrect
                      ? "bg-green-100 border-green-400"
                      : "bg-gray-100 border-gray-300"
                    : "hover:bg-gray-50 hover:border-gray-400 bg-white border-gray-300"
                }
              `}
            >
              {/* CenteredChoice text */}
              <div className="flex justify-center items-center h-full">
                <span className="text-lg font-semibold block mb-4">
                  {choice}
                </span>
              </div>

              {/* Student picks */}
              {answersByChoice[idx].length > 0 && (
                <div className="flex flex-wrap gap-3">
                  {answersByChoice[idx].map((a) => (
                    <div
                      key={a.studentId}
                      className="flex items-center gap-2 bg-white px-3 py-1.5 rounded-xl border border-gray-200"
                    >
                      {/* Avatar */}
                      <div className="w-8 h-8 rounded-full bg-blue-600 text-white flex items-center justify-center text-sm font-bold">
                        {getInitial(a.studentName)}
                      </div>

                      {/* Name */}
                      <span className="text-sm font-medium text-gray-700">
                        {a.studentName}
                      </span>
                    </div>
                  ))}
                </div>
              )}
            </div>
          );
        })}
      </div>

      {/* CONTROL BUTTON */}
      {!showAnswer ? (
        <button
          onClick={handleShowAnswer}
          className="px-8 py-4 text-xl font-semibold bg-blue-600 text-white rounded-xl shadow hover:bg-blue-700 transition"
        >
          Show Correct Answer
        </button>
      ) : (
        <button
          onClick={handleNextQuestion}
          className="px-8 py-4 text-xl font-semibold bg-indigo-600 text-white rounded-xl shadow hover:bg-indigo-700 transition"
        >
          Next Question
        </button>
      )}
    </div>
  );
}
