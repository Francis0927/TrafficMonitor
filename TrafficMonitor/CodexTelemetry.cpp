#ifndef CODEX_TELEMETRY_TEST
#include "stdafx.h"
#endif
#include "CodexTelemetry.h"
#include "../third_party/nlohmann/json.hpp"
#include <windows.h>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <set>
#include <charconv>

namespace codex
{
using Json = nlohmann::json;
namespace
{
Json Member(const Json& value, const char* key)
{
    auto it = value.is_object() ? value.find(key) : value.end();
    return it == value.end() ? Json() : *it;
}
std::string Text(const Json& value)
{
    return value.is_string() ? value.get<std::string>() : std::string();
}
std::optional<double> Number(const Json& value)
{
    if (!value.is_number()) return {};
    double number = value.get<double>();
    return std::isfinite(number) ? std::optional<double>(number) : std::nullopt;
}
// Codex emits ISO 8601 timestamps; offsets and fractional seconds are supported.
std::optional<double> Timestamp(const Json& value)
{
    auto text = Text(value);
    if (text.size() < 20 || text[4] != '-' || text[7] != '-' || text[10] != 'T' || text[13] != ':' || text[16] != ':') return {};
    int y, mo, d, h, mi, s;
    auto parse = [&](size_t start, size_t length, int& result) {
        for (size_t i = start; i < start + length; ++i) if (text[i] < '0' || text[i] > '9') return false;
        return std::from_chars(text.data() + start, text.data() + start + length, result).ec == std::errc();
    };
    if (!parse(0, 4, y) || !parse(5, 2, mo) || !parse(8, 2, d) || !parse(11, 2, h) || !parse(14, 2, mi) || !parse(17, 2, s)) return {};
    using namespace std::chrono;
    year_month_day date{year{y}, month{unsigned(mo)}, day{unsigned(d)}};
    if (!date.ok() || y < 1970 || y > 2100 || h < 0 || h > 23 || mi < 0 || mi > 59 || s < 0 || s > 59) return {};
    size_t pos = 19;
    double fraction{}, scale = 0.1;
    if (pos < text.size() && text[pos] == '.')
    {
        ++pos;
        if (pos == text.size() || text[pos] < '0' || text[pos] > '9') return {};
        while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9')
        { fraction += (text[pos++] - '0') * scale; scale *= 0.1; }
    }
    int offset{};
    if (pos < text.size() && text[pos] == 'Z') ++pos;
    else if (pos + 6 == text.size() && (text[pos] == '+' || text[pos] == '-') && text[pos + 3] == ':')
    {
        for (size_t i : {pos + 1, pos + 2, pos + 4, pos + 5}) if (text[i] < '0' || text[i] > '9') return {};
        int oh = (text[pos + 1] - '0') * 10 + text[pos + 2] - '0';
        int om = (text[pos + 4] - '0') * 10 + text[pos + 5] - '0';
        if (oh > 23 || om > 59) return {};
        offset = (oh * 60 + om) * 60 * (text[pos] == '+' ? 1 : -1); pos += 6;
    }
    else return {};
    if (pos != text.size()) return {};
    return duration<double>(sys_days{date}.time_since_epoch()).count() + h * 3600 + mi * 60 + s + fraction - offset;
}
std::filesystem::path DefaultHome()
{
    wchar_t buffer[32768];
    DWORD size = GetEnvironmentVariableW(L"CODEX_HOME", buffer, 32768);
    if (size && size < 32768) return buffer;
    size = GetEnvironmentVariableW(L"USERPROFILE", buffer, 32768);
    return size && size < 32768 ? std::filesystem::path(buffer) / L".codex" : std::filesystem::path();
}
}
Collector::Collector(std::filesystem::path home) : m_home(home.empty() ? DefaultHome() : std::move(home)) {}
Collector::~Collector() { Stop(); }
void Collector::Start()
{
    Stop(); m_stop = false;
    m_worker = std::thread([this] {
        while (!m_stop)
        {
            Poll(Now());
            std::unique_lock lock(m_wait_mutex);
            m_wake.wait_for(lock, std::chrono::seconds(2), [this] { return m_stop.load(); });
        }
    });
}
void Collector::Stop()
{
    m_stop = true; m_wake.notify_all();
    if (m_worker.joinable()) m_worker.join();
    m_stop = false;
}
double Collector::Now()
{
    return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
}
Snapshot Collector::GetSnapshot() const
{
    std::lock_guard lock(m_mutex); return m_snapshot;
}
void Collector::Discover()
{
    std::set<std::filesystem::path> present;
    if (m_home.empty()) return;
    for (const auto* folder : {L"sessions", L"archived_sessions"})
    {
        std::error_code ec;
        auto root = m_home / folder;
        const auto attributes = GetFileAttributesW(root.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            if (GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND) m_error = true;
            continue;
        }
        if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
        std::filesystem::recursive_directory_iterator it(root, ec), end;
        if (ec) { m_error = true; continue; }
        while (it != end && !m_stop)
        {
            auto path = it->path();
            auto attr = GetFileAttributesW(path.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_REPARSE_POINT)) it.disable_recursion_pending();
            else if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY) && path.extension() == L".jsonl") present.insert(path);
            it.increment(ec);
            if (ec) { m_error = true; break; }
        }
    }
    // Retain known files on a partial/failed traversal; retry discovery next poll.
    if (!m_error)
        for (auto it = m_files.begin(); it != m_files.end();)
            if (!present.count(it->first)) it = m_files.erase(it); else ++it;
    for (const auto& path : present) m_files.try_emplace(path);
}
void Collector::Read(const std::filesystem::path& path, File& file, double now)
{
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE) { m_error = true; return; }
    struct Close { HANDLE h; ~Close() { CloseHandle(h); } } close{handle};
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(handle, &info)) { m_error = true; return; }
    if (info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) return;
    uint64_t size = (uint64_t(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    uint64_t identity = (uint64_t(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
    // The handle identity and write time catch rotation, truncation and same-size replacement.
    auto modified = std::filesystem::file_time_type(std::filesystem::file_time_type::duration(
        (uint64_t(info.ftLastWriteTime.dwHighDateTime) << 32) | info.ftLastWriteTime.dwLowDateTime));
    if (file.identity != identity || size < file.offset || (size == file.offset && modified != file.modified)) file = File{};
    file.identity = identity; file.modified = modified;
    LARGE_INTEGER offset; offset.QuadPart = file.offset;
    if (!SetFilePointerEx(handle, offset, nullptr, FILE_BEGIN)) { m_error = true; return; }
    char buffer[65536];
    // Limit one file's work per poll; retained offsets resume large initial histories.
    uint64_t target = (std::min)(size, file.offset + 4 * 1024 * 1024);
    while (file.offset < target && !m_stop)
    {
        DWORD read{};
        if (!ReadFile(handle, buffer, DWORD((std::min)(uint64_t(sizeof(buffer)), target - file.offset)), &read, nullptr)) { m_error = true; break; }
        if (!read) break;
        file.offset += read;
        for (DWORD i = 0; i < read; ++i)
        {
            if (buffer[i] == '\n')
            {
                ++file.line;
                if (!file.discard_line) Accept(file, file.pending, now);
                file.pending.clear(); file.discard_line = false;
            }
            else if (!file.discard_line)
            {
                if (file.pending.size() < 4 * 1024 * 1024) file.pending += buffer[i];
                else { file.pending.clear(); file.discard_line = true; }
            }
        }
    }
    m_loading |= file.offset < size;
    auto prune = [now](std::vector<Event>& events) {
        events.erase(std::remove_if(events.begin(), events.end(), [now](const Event& event) { return event.time < now - 600; }), events.end());
    };
    prune(file.records); prune(file.counters);
}
void Collector::Accept(File& file, const std::string& line, double now)
{
    if (line.find("token_count") == std::string::npos && line.find("token_usage_record") == std::string::npos && line.find("session_meta") == std::string::npos) return;
    auto entry = Json::parse(line, nullptr, false);
    if (!entry.is_object()) return;
    auto type = Text(Member(entry, "type"));
    auto payload = Member(entry, "payload");
    if (type == "session_meta") { file.session = Text(Member(payload, "id")); return; }
    auto timestamp = Timestamp(Member(entry, "timestamp"));
    if (!timestamp || *timestamp > now + 300) return;
    if (type == "event_msg" && Text(Member(payload, "type")) == "token_count")
    {
        auto limits = Member(payload, "rate_limits");
        auto id = Member(limits, "limit_id");
        if (limits.is_object() && (id.is_null() || Text(id) == "codex"))
        {
            Snapshot quota; quota.quota_updated_at = *timestamp;
            for (auto key : {"primary", "secondary"})
            {
                auto window = Member(limits, key);
                auto used = Number(Member(window, "used_percent"));
                auto minutes = Number(Member(window, "window_minutes"));
                if (!used || !minutes || (*minutes != 300 && *minutes != 10080)) continue;
                QuotaWindow value; value.remaining = std::clamp(100 - *used, 0.0, 100.0);
                auto reset = Number(Member(window, "resets_at"));
                if (reset && *reset > 0 && *reset <= 4133980800.0) value.resets_at = *reset;
                (*minutes == 300 ? quota.five_hour : quota.weekly) = value;
            }
            if ((quota.five_hour || quota.weekly) && quota.quota_updated_at >= file.quota.quota_updated_at) file.quota = quota;
        }
        auto usage = Member(Member(payload, "info"), "total_token_usage");
        auto output = Number(Member(usage, "output_tokens"));
        auto input = Number(Member(usage, "input_tokens"));
        if (!output || !input || *output < 0 || *input < 0) return;
        double delta = file.previous_output && *output >= *file.previous_output ? *output - *file.previous_output : *output;
        file.previous_output = *output;
        if (delta > 0 && *timestamp >= now - 600)
            file.counters.push_back({*timestamp, delta, std::to_string(*timestamp) + ":" + std::to_string(*output)});
    }
    else if (type == "token_usage_record")
    {
        auto usage = Member(payload, "usage");
        auto output = Number(Member(usage, "output_tokens"));
        auto input = Number(Member(usage, "input_tokens"));
        if (!output || !input || *output < 0 || *input < 0) return;
        file.has_records = true; // response records take precedence over counters in this file
        if (*timestamp >= now - 600)
        {
            auto id = Text(Member(payload, "response_id"));
            if (id.empty()) id = "line:" + std::to_string(file.line);
            file.records.push_back({*timestamp, *output, std::move(id)});
        }
    }
}
void Collector::Publish(double now)
{
    Snapshot snapshot; snapshot.ready = !m_files.empty(); snapshot.error = m_error; snapshot.loading = m_loading;
    std::set<std::string> seen;
    std::set<std::string> record_sessions;
    for (const auto& [path, file] : m_files)
        if (file.has_records && !file.session.empty()) record_sessions.insert(file.session);
    for (const auto& [path, file] : m_files)
    {
        if (file.quota.quota_updated_at > snapshot.quota_updated_at)
        {
            snapshot.quota_updated_at = file.quota.quota_updated_at;
            snapshot.five_hour = file.quota.five_hour; snapshot.weekly = file.quota.weekly;
        }
        const auto session = file.session.empty() ? path.filename().u8string() : std::u8string(file.session.begin(), file.session.end());
        const std::string session_key(session.begin(), session.end());
        const auto& events = file.has_records ? file.records : file.counters;
        if (!file.has_records && !file.session.empty() && record_sessions.count(file.session)) continue;
        for (const auto& event : events)
        {
            // response IDs are globally unique; fallback IDs/counters are session scoped.
            auto id = file.has_records && event.id.rfind("line:", 0) != 0 ? "record:" + event.id : session_key + ":" + event.id;
            if (event.time <= now && seen.insert(id).second) snapshot.speed += event.output / 15 * std::exp(-(now - event.time) / 15);
        }
    }
    if (snapshot.speed < 0.5 || m_error) snapshot.speed = 0;
    std::lock_guard lock(m_mutex); m_snapshot = snapshot;
}
void Collector::Poll(double now)
{
    if (now >= m_last_poll && now - m_last_poll < 2) return;
    m_last_poll = now; m_error = false; m_loading = false;
    try
    {
        if (now < m_last_discovery || now - m_last_discovery >= 30)
        { Discover(); m_last_discovery = m_error ? -1e30 : now; }
        for (auto& [path, file] : m_files) { if (m_stop) break; Read(path, file, now); }
    }
    catch (const std::exception&) { m_error = true; }
    if (m_error) m_last_discovery = -1e30;
    Publish(now);
}
}
