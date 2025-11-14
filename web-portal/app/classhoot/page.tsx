"use client";

import { useState } from "react";
import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Label } from "@/components/ui/label";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Plus, Trash2, Save } from "lucide-react";
import { useRouter } from "next/navigation";
import { Question } from "@/types/types";

export default function ClassHootPage() {
  const router = useRouter();
  const [title, setTitle] = useState("");
  const [questions, setQuestions] = useState<Question[]>([
    {
      text: "",
      choice1: "",
      choice2: "",
      choice3: "",
      choice4: "",
      correctChoice: 1,
      timeLimit: 10,
    },
  ]);
  const [isSubmitting, setIsSubmitting] = useState(false);

  const addQuestion = () => {
    setQuestions([
      ...questions,
      {
        text: "",
        choice1: "",
        choice2: "",
        choice3: "",
        choice4: "",
        correctChoice: 1,
        timeLimit: 10,
      },
    ]);
  };

  const removeQuestion = (index: number) => {
    if (questions.length > 1) {
      setQuestions(questions.filter((_, i) => i !== index));
    }
  };

  const updateQuestion = (
    index: number,
    field: keyof Question,
    value: string | number
  ) => {
    const updated = [...questions];
    updated[index] = { ...updated[index], [field]: value };
    setQuestions(updated);
  };

  const handleSubmit = async () => {
    if (!title.trim()) {
      alert("Please enter a quiz title");
      return;
    }

    const hasEmptyFields = questions.some(
      (q) =>
        !q.text.trim() ||
        !q.choice1.trim() ||
        !q.choice2.trim() ||
        !q.choice3.trim() ||
        !q.choice4.trim()
    );

    if (hasEmptyFields) {
      alert("Please fill in all question fields");
      return;
    }

    setIsSubmitting(true);

    try {
      const response = await fetch("/api/classhoot/gameroom", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ title, questions }),
      });

      if (!response.ok) throw new Error("Failed to create game room");

      const data = await response.json();
      router.push(`/classhoot/${data.code}`);
    } catch (error) {
      console.error(error);
      alert("Error creating game room");
    } finally {
      setIsSubmitting(false);
    }
  };

  return (
    <section className="">
      <div className="">
        <h1 className="text-4xl font-bold mb-4">Class Hoot</h1>
        <p className="text-lg text-gray-600 mb-12">
          Create an interactive classroom quiz game
        </p>
      </div>

      <div className="space-y-6 ">
        <div className="space-y-2">
          <Label htmlFor="title">Quiz Title</Label>
          <Input
            id="title"
            value={title}
            onChange={(e) => setTitle(e.target.value)}
            placeholder="Enter quiz title"
          />
        </div>

        <div className="space-y-4">
          <div className="flex items-center justify-between">
            <h2 className="text-2xl font-semibold">Questions</h2>
            <Button
              type="button"
              onClick={addQuestion}
              variant="outline"
              size="sm"
            >
              <Plus className="w-4 h-4 mr-2" />
              Add Question
            </Button>
          </div>

          {questions.map((question, index) => (
            <Card key={index}>
              <CardHeader>
                <div className="flex items-center justify-between">
                  <CardTitle className="text-lg">
                    Question {index + 1}
                  </CardTitle>
                  {questions.length > 1 && (
                    <Button
                      type="button"
                      onClick={() => removeQuestion(index)}
                      variant="ghost"
                      size="sm"
                    >
                      <Trash2 className="w-4 h-4 text-red-500" />
                    </Button>
                  )}
                </div>
              </CardHeader>
              <CardContent className="space-y-4">
                <div className="space-y-2">
                  <Input
                    id={`question-${index}`}
                    value={question.text}
                    onChange={(e) =>
                      updateQuestion(index, "text", e.target.value)
                    }
                    placeholder="Enter your question"
                  />
                </div>

                <div className="grid grid-cols-2 gap-4">
                  {[1, 2, 3, 4].map((choiceNum) => (
                    <div key={choiceNum} className="space-y-2">
                      <Label htmlFor={`choice${choiceNum}-${index}`}>
                        Choice {choiceNum}
                        {question.correctChoice === choiceNum && (
                          <span className="ml-2 text-xs text-green-600 font-semibold">
                            ✓ Correct
                          </span>
                        )}
                      </Label>
                      <Input
                        id={`choice${choiceNum}-${index}`}
                        value={
                          question[
                            `choice${choiceNum}` as keyof Question
                          ] as string
                        }
                        onChange={(e) =>
                          updateQuestion(
                            index,
                            `choice${choiceNum}` as keyof Question,
                            e.target.value
                          )
                        }
                        placeholder={`Enter choice ${choiceNum}`}
                      />
                    </div>
                  ))}
                </div>

                <div className="grid grid-cols-2 gap-4">
                  <div className="space-y-2">
                    <Label htmlFor={`correct-${index}`}>Correct Answer</Label>
                    <select
                      id={`correct-${index}`}
                      value={question.correctChoice}
                      onChange={(e) =>
                        updateQuestion(
                          index,
                          "correctChoice",
                          parseInt(e.target.value)
                        )
                      }
                      className="w-full h-10 rounded-md border border-input bg-background px-3 py-2 text-sm"
                    >
                      <option value={1}>Choice 1</option>
                      <option value={2}>Choice 2</option>
                      <option value={3}>Choice 3</option>
                      <option value={4}>Choice 4</option>
                    </select>
                  </div>

                  <div className="space-y-2">
                    <Label htmlFor={`time-${index}`}>
                      Time Limit (seconds)
                    </Label>
                    <Input
                      id={`time-${index}`}
                      type="number"
                      min="5"
                      max="120"
                      value={question.timeLimit}
                      onChange={(e) =>
                        updateQuestion(
                          index,
                          "timeLimit",
                          parseInt(e.target.value)
                        )
                      }
                    />
                  </div>
                </div>
              </CardContent>
            </Card>
          ))}
        </div>

        <Button
          onClick={handleSubmit}
          className="w-full"
          size="lg"
          disabled={isSubmitting}
        >
          <Save className="w-4 h-4 mr-2" />
          {isSubmitting ? "Creating..." : "Create Quiz"}
        </Button>
      </div>
    </section>
  );
}
