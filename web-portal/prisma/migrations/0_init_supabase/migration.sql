-- CreateEnum
CREATE TYPE "Vacancy" AS ENUM ('UNKNOWN', 'OCCUPIED', 'VACANT');

-- CreateTable
CREATE TABLE "Classrooms" (
    "id" SERIAL NOT NULL,
    "name" TEXT NOT NULL,
    "vacancy" "Vacancy" NOT NULL DEFAULT 'UNKNOWN',
    "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updatedAt" TIMESTAMP(3) NOT NULL,

    CONSTRAINT "Classrooms_pkey" PRIMARY KEY ("id")
);

