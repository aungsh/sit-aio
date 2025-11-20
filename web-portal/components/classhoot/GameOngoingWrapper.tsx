"use client";

import { useState, useEffect } from "react";
import { useRouter } from "next/navigation";
import GameOngoing from "@/components/classhoot/GameOngoing";
import { Question } from "@/types/types";

type GameOngoingWrapperProps = {
  code: string;
};

export default function GameOngoingWrapper({ code }: GameOngoingWrapperProps) {
  const [questions, setQuestions] = useState<Question[]>([]);
  const [currentIndex, setCurrentIndex] = useState<number | null>(null);
  const [loading, setLoading] = useState(true);
  const router = useRouter();

  // Load room state (index + question)
  useEffect(() => {
    async function loadRoom() {
      try {
        const roomRes = await fetch(`/api/classhoot/gameroom?code=${code}`);
        const room = await roomRes.json();

        const idx = room.currentIndex;

        const qRes = await fetch(
          `/api/classhoot/gameroom/question?code=${code}`
        );
        const question = await qRes.json();

        const arr: Question[] = [];
        arr[idx] = question;

        setQuestions(arr);
        setCurrentIndex(idx);
      } catch (err) {
        console.error(err);
      } finally {
        setLoading(false);
      }
    }

    loadRoom();
  }, [code]);

  const handleNextQuestion = async () => {
    try {
      const res = await fetch("/api/classhoot/gameroom/next", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ code }),
      });

      const data = await res.json();

      if (data.message === "Game ended") {
        router.refresh();
        return;
      }

      const newIndex = data.currentIndex;

      const qRes = await fetch(`/api/classhoot/gameroom/question?code=${code}`);
      const nextQuestion = await qRes.json();

      setQuestions((prev) => {
        const updated = [...prev];
        updated[newIndex] = nextQuestion;
        return updated;
      });

      setCurrentIndex(newIndex);
    } catch (err) {
      console.error(err);
    }
  };

  if (loading || currentIndex === null) return <div>Loading question...</div>;
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
