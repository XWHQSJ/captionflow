#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

/* Latest-value caption file writer.
 *
 * submit() coalesces: only the newest generation is kept, and an older
 * generation that arrives late is dropped. The write itself (temp file +
 * rename) runs on a background thread so the audio and ASR threads never
 * block on disk.
 *
 * shutdown() flushes the pending caption and joins the thread. The destructor
 * calls shutdown().
 */
class CaptionFileWriter {
public:
	CaptionFileWriter();
	~CaptionFileWriter();

	CaptionFileWriter(const CaptionFileWriter &) = delete;
	CaptionFileWriter &operator=(const CaptionFileWriter &) = delete;

	void submit(uint64_t generation, std::string path, std::string content);
	void shutdown();

	/* Blocks until the queued caption has been written, or the timeout
	 * elapses. Returns false on timeout. */
	bool wait_until_idle(int timeout_ms);

	uint64_t writes_completed() const { return writes_.load(std::memory_order_relaxed); }

private:
	void thread_main();

	std::mutex mu_;
	std::condition_variable cv_;
	std::condition_variable idle_cv_;
	std::string path_;
	std::string content_;
	uint64_t queued_generation_ = 0;
	bool dirty_ = false;
	bool writing_ = false;
	bool stop_ = false;
	std::atomic<uint64_t> writes_{0};
	std::thread thread_;
};
