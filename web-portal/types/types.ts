interface Player {
  id: string;
  name: string;
  score?: number;
  joinedAt?: string;
}

interface GameRoom {
  id: number;
  code: string;
  title: string;
  status: string;
  totalQuestions: number;
  players: Player[];
}

interface QuestionInput {
  text: string;
  choice1: string;
  choice2: string;
  choice3: string;
  choice4: string;
  correctChoice: number;
  timeLimit?: number;
}

interface GameRoomPayload {
  title: string;
  questions: QuestionInput[];
}

interface Question {
  text: string;
  choice1: string;
  choice2: string;
  choice3: string;
  choice4: string;
  correctChoice: number;
  timeLimit: number;
}

export type { Player, GameRoom, QuestionInput, GameRoomPayload, Question };
