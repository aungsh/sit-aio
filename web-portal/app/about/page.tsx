import Image from "next/image";

export default function AboutUsPage() {
  return (
    <>
      <h1 className="text-4xl font-bold mb-4">About Us</h1>
      <div className="space-y-10">
        {/* Header */}
        <header className="space-y-4">
          <p className="text-xl text-gray-600 leading-relaxed">
            We are students from the Singapore Institute of Technology (SIT),
            and this project was created for the module{" "}
            <span className="font-semibold">
              ICT1011 – Computer Organisation and Architecture
            </span>
            .
          </p>
        </header>

        {/* Team */}
        <section className="space-y-4">
          <div className="mt-6">
            <Image
              src="/team.jpeg"
              alt="Our Team"
              width={500}
              height={400}
              className="rounded-xl shadow-lg"
            />
          </div>
        </section>
      </div>
    </>
  );
}
