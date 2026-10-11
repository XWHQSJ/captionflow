#include "subtitle-manager.h"

#include <algorithm>
#include <cstdint>

void SubtitleManager::set_max_lines(int value)
{
	max_lines_ = std::max(1, value);
	trim_history();
}

void SubtitleManager::set_clear_timeout_sec(double value)
{
	clear_timeout_sec_ = value;
}

void SubtitleManager::set_partial_write_throttle_ms(uint64_t value)
{
	partial_write_throttle_ms_ = value;
}

void SubtitleManager::on_partial(const std::string &text, uint64_t now_ms)
{
	partial_ = text;
	last_text_time_ms_ = now_ms;
	has_pending_partial_update_ = true;
}

void SubtitleManager::on_final(const std::string &text, uint64_t now_ms)
{
	partial_.clear();
	last_text_time_ms_ = now_ms;
	force_emit_next_update_ = true;
	has_pending_partial_update_ = false;
	if (text.empty())
		return;

	finals_.push_back(text);
	trim_history();
}

SubtitleSnapshot SubtitleManager::snapshot(uint64_t now_ms) const
{
	SubtitleSnapshot result;

	const bool is_stale = last_text_time_ms_ > 0 && clear_timeout_sec_ > 0.0 &&
			      (now_ms - last_text_time_ms_) > static_cast<uint64_t>(clear_timeout_sec_ * 1000.0);
	if (is_stale)
		return result;

	result.finals.assign(finals_.begin(), finals_.end());
	result.partial = partial_;
	return result;
}

std::string SubtitleManager::compose_output(uint64_t now_ms) const
{
	const SubtitleSnapshot current = snapshot(now_ms);
	std::string output;
	for (const auto &line : current.finals)
		output += line + "\n";
	if (!current.partial.empty())
		output += current.partial + "\n";
	return output;
}

bool SubtitleManager::clear_timeout_due(uint64_t now_ms) const
{
	if (last_emitted_content_.empty() || last_text_time_ms_ == 0 || clear_timeout_sec_ <= 0.0)
		return false;
	const auto timeout_ms = static_cast<uint64_t>(clear_timeout_sec_ * 1000.0);
	return now_ms > last_text_time_ms_ + timeout_ms;
}

void SubtitleManager::recompute_next_check(uint64_t now_ms)
{
	if (force_emit_next_update_ || has_pending_partial_update_) {
		if (has_pending_partial_update_ && !force_emit_next_update_ && partial_write_throttle_ms_ > 0 &&
		    now_ms < last_partial_write_ms_ + partial_write_throttle_ms_) {
			next_file_check_ms_ = last_partial_write_ms_ + partial_write_throttle_ms_;
		} else {
			next_file_check_ms_ = now_ms;
		}
		return;
	}

	if (!last_emitted_content_.empty() && last_text_time_ms_ > 0 && clear_timeout_sec_ > 0.0) {
		const auto timeout_ms = static_cast<uint64_t>(clear_timeout_sec_ * 1000.0);
		next_file_check_ms_ = last_text_time_ms_ + timeout_ms + 1;
		return;
	}

	next_file_check_ms_ = UINT64_MAX;
}

bool SubtitleManager::should_emit_file_update(uint64_t now_ms)
{
	const bool timeout_due = clear_timeout_due(now_ms);
	if (!force_emit_next_update_ && !has_pending_partial_update_ && !timeout_due) {
		recompute_next_check(now_ms);
		return false;
	}

	if (!force_emit_next_update_ && has_pending_partial_update_ && partial_write_throttle_ms_ > 0 &&
	    now_ms < last_partial_write_ms_ + partial_write_throttle_ms_ && !timeout_due) {
		recompute_next_check(now_ms);
		return false;
	}

	const std::string content = compose_output(now_ms);
	if (content == last_emitted_content_) {
		/* Identical text must not leave the pending flag set, or the
		 * audio thread recomposes this string on every callback. */
		force_emit_next_update_ = false;
		has_pending_partial_update_ = false;
		recompute_next_check(now_ms);
		return false;
	}

	if (force_emit_next_update_) {
		last_emitted_content_ = content;
		force_emit_next_update_ = false;
		last_partial_write_ms_ = now_ms;
		has_pending_partial_update_ = false;
		recompute_next_check(now_ms);
		return true;
	}

	last_emitted_content_ = content;
	last_partial_write_ms_ = now_ms;
	has_pending_partial_update_ = false;
	recompute_next_check(now_ms);
	return true;
}

void SubtitleManager::trim_history()
{
	while (static_cast<int>(finals_.size()) > max_lines_)
		finals_.pop_front();
}
