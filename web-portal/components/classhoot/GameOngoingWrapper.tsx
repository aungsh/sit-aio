"use client";

import { useState, useEffect } from "react";
import GameOngoing from "@/components/classhoot/GameOngoing";
import { Question } from "@/types/types";

type GameOngoingWrapperProps = {
  code: string; // Game room code
};

export default function GameOngoingWrapper({ code }: GameOngoingWrapperProps) {
  const [questions, setQuestions] = useState<Question[]>([]);
  const [currentIndex, setCurrentIndex] = useState(0);
  const [loading, setLoading] = useState(true);

  // Fetch the initial question list
  useEffect(() => {
    async function fetchQuestions() {
      try {
        const res = await fetch(
          `/api/classhoot/gameroom/question?code=${code}&index=0`
        );
        const firstQuestion = await res.json();
        setQuestions([firstQuestion]);
      } catch (err) {
        console.error(err);
      } finally {
        setLoading(false);
      }
    }
    fetchQuestions();
  }, [code]);

  const handleNextQuestion = async () => {
    try {
      // Move the currentIndex in the backend
      const res = await fetch("/api/classhoot/gameroom/next", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ code }),
      });
      const data = await res.json();

      if (data.message === "Game ended") {
        alert("Game has ended!");
        return;
      }

      // Fetch the next question
      const nextIndex = data.currentIndex;
      const questionRes = await fetch(
        `/api/classhoot/gameroom/question?code=${code}&index=${nextIndex}`
      );
      const nextQuestion = await questionRes.json();

      setQuestions((prev) => [...prev, nextQuestion]);
      setCurrentIndex(nextIndex);
    } catch (err) {
      console.error(err);
    }
  };

  if (loading) return <div>Loading question...</div>;
  if (questions.length === 0) return <div>No questions found</div>;

  return (
    <GameOngoing
      code={code}
      questions={questions}
      currentIndex={currentIndex}
      onNextQuestion={handleNextQuestion}
    />
  );
}
