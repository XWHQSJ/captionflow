#include "caption-file-writer.h"
#include "test-framework.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

struct TempCaptionFile {
	std::string path;

	TempCaptionFile()
	{
		static std::atomic<uint64_t> seq{0};
		const auto id = seq.fetch_add(1, std::memory_order_relaxed);
		const auto tick =
			static_cast<unsigned long long>(std::chrono::steady_clock::now().time_since_epoch().count());
		path = (std::filesystem::temp_directory_path() /
			("captionflow-" + std::to_string(tick) + "-" + std::to_string(id) + ".txt"))
			       .string();
	}

	~TempCaptionFile()
	{
		std::error_code ec;
		std::filesystem::remove(path, ec);
		std::filesystem::remove(path + ".tmp", ec);
	}
};

std::string slurp(const std::string &path)
{
	std::ifstream in(path);
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

} /* namespace */

TEST(CaptionFileWriter, DestructorFlushes)
{
	TempCaptionFile file;
	{
		CaptionFileWriter writer;
		writer.submit(1, file.path, "bye\n");
	}
	EXPECT_STREQ(slurp(file.path), "bye\n");
	EXPECT_FALSE(std::filesystem::exists(file.path + ".tmp"));
}

TEST(CaptionFileWriter, DropsStaleGeneration)
{
	TempCaptionFile file;
	CaptionFileWriter writer;
	writer.submit(2, file.path, "new\n");
	writer.submit(1, file.path, "old\n");
	ASSERT_TRUE(writer.wait_until_idle(2000));
	EXPECT_STREQ(slurp(file.path), "new\n");

	writer.submit(4, file.path, "newer\n");
	ASSERT_TRUE(writer.wait_until_idle(2000));
	writer.submit(3, file.path, "stale\n");
	ASSERT_TRUE(writer.wait_until_idle(2000));
	EXPECT_STREQ(slurp(file.path), "newer\n");
}

TEST(CaptionFileWriter, CoalescesBurst)
{
	TempCaptionFile file;
	CaptionFileWriter writer;
	constexpr int kSubmits = 200;
	for (int i = 1; i <= kSubmits; i++)
		writer.submit(static_cast<uint64_t>(i), file.path, "v" + std::to_string(i));
	ASSERT_TRUE(writer.wait_until_idle(2000));
	EXPECT_STREQ(slurp(file.path), "v200");
	EXPECT_GE(writer.writes_completed(), 1u);
	EXPECT_LT(writer.writes_completed(), 40u);
}

TEST(CaptionFileWriter, EmptyPathSkipsWrite)
{
	TempCaptionFile file;
	CaptionFileWriter writer;
	writer.submit(1, file.path, "keep\n");
	ASSERT_TRUE(writer.wait_until_idle(2000));
	writer.submit(2, "", "nope\n");
	ASSERT_TRUE(writer.wait_until_idle(2000));
	EXPECT_STREQ(slurp(file.path), "keep\n");
	EXPECT_EQ(writer.writes_completed(), 1u);
}
