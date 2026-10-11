#include "caption-file-writer.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <utility>

namespace {

bool write_caption_file(const std::string &output_path, const std::string &content)
{
	const std::string tmp_path = output_path + ".tmp";
	{
		std::ofstream out(tmp_path, std::ios::trunc);
		if (!out) {
			std::fprintf(stderr, "[CaptionFlow] Failed to open caption temp file '%s'\n", tmp_path.c_str());
			return false;
		}
		out << content;
		if (!out.good()) {
			std::fprintf(stderr, "[CaptionFlow] Failed to write caption temp file '%s'\n",
				     tmp_path.c_str());
			out.close();
			std::remove(tmp_path.c_str());
			return false;
		}
	}
#ifdef _WIN32
	/* rename() on Windows fails if the destination exists. */
	if (std::remove(output_path.c_str()) != 0 && errno != ENOENT) {
		std::fprintf(stderr, "[CaptionFlow] Failed to remove caption file '%s': %s\n", output_path.c_str(),
			     std::strerror(errno));
	}
#endif
	if (std::rename(tmp_path.c_str(), output_path.c_str()) != 0) {
		std::fprintf(stderr, "[CaptionFlow] Failed to rename caption file '%s' -> '%s': %s\n", tmp_path.c_str(),
			     output_path.c_str(), std::strerror(errno));
		std::remove(tmp_path.c_str());
		return false;
	}
	return true;
}

} /* namespace */

CaptionFileWriter::CaptionFileWriter() : thread_(&CaptionFileWriter::thread_main, this) {}

CaptionFileWriter::~CaptionFileWriter()
{
	shutdown();
}

void CaptionFileWriter::submit(uint64_t generation, std::string path, std::string content)
{
	{
		std::lock_guard<std::mutex> lock(mu_);
		if (stop_ || generation < queued_generation_)
			return;
		queued_generation_ = generation;
		if (path.empty()) {
			path_.clear();
			content_.clear();
			dirty_ = false;
			idle_cv_.notify_all();
			return;
		}
		path_ = std::move(path);
		content_ = std::move(content);
		dirty_ = true;
	}
	cv_.notify_one();
}

void CaptionFileWriter::shutdown()
{
	{
		std::lock_guard<std::mutex> lock(mu_);
		if (stop_)
			return;
		stop_ = true;
	}
	cv_.notify_one();
	if (thread_.joinable())
		thread_.join();
}

bool CaptionFileWriter::wait_until_idle(int timeout_ms)
{
	std::unique_lock<std::mutex> lock(mu_);
	return idle_cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return !dirty_ && !writing_; });
}

void CaptionFileWriter::thread_main()
{
	for (;;) {
		std::string path;
		std::string content;
		{
			std::unique_lock<std::mutex> lock(mu_);
			cv_.wait(lock, [this] { return stop_ || dirty_; });
			if (!dirty_) {
				if (stop_)
					break;
				continue;
			}
			path = path_;
			content = content_;
			dirty_ = false;
			writing_ = true;
		}

		const bool wrote = write_caption_file(path, content);
		if (wrote)
			writes_.fetch_add(1, std::memory_order_relaxed);

		{
			std::lock_guard<std::mutex> lock(mu_);
			writing_ = false;
			if (!dirty_)
				idle_cv_.notify_all();
		}
	}
}
