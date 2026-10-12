#include "../TrafficMonitor/CodexTelemetry.h"
#include "../TrafficMonitor/ReleaseVersion.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <windows.h>

void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void Write(const std::filesystem::path& file, const std::string& value, bool append = false)
{
    std::filesystem::create_directories(file.parent_path());
    std::ofstream stream(file, std::ios::binary | (append ? std::ios::app : std::ios::trunc));
    stream << value;
}
std::string Record(const char* id, int output, const char* time = "2026-10-12T04:00:00Z")
{
    return std::string("{\"timestamp\":\"") + time + "\",\"type\":\"token_usage_record\",\"payload\":{\"response_id\":\"" + id + "\",\"usage\":{\"input_tokens\":100,\"output_tokens\":" + std::to_string(output) + "}}}\n";
}
std::string Counter(int output)
{
    return "{\"timestamp\":\"2026-10-12T04:00:00Z\",\"type\":\"event_msg\",\"payload\":{\"type\":\"token_count\",\"info\":{\"total_token_usage\":{\"input_tokens\":100,\"output_tokens\":" + std::to_string(output) + "}}}}\n";
}
const std::string quota = R"({"timestamp":"2026-10-12T04:00:00Z","type":"event_msg","payload":{"type":"token_count","rate_limits":{"limit_id":"codex","primary":{"used_percent":25.5,"window_minutes":300,"resets_at":1791781200},"secondary":{"used_percent":60,"window_minutes":10080,"resets_at":1792386000}}}})";
int main(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--live")
    {
        codex::Collector collector;
        auto start = GetTickCount64();
        collector.Poll(codex::Collector::Now());
        auto sample = collector.GetSnapshot();
        std::cout << "local logs: ready=" << sample.ready << " error=" << sample.error << " loading=" << sample.loading
                  << " speed=" << sample.speed << " quota=" << bool(sample.five_hour || sample.weekly)
                  << " elapsed_ms=" << GetTickCount64() - start << '\n';
        return sample.error ? 1 : 0;
    }
    auto root = std::filesystem::temp_directory_path() / (L"TrafficMonitor-codex-test-" + std::to_wstring(GetCurrentProcessId()));
    auto file = root / L"sessions" / L"2026" / L"10" / L"12" / L"rollout.jsonl";
    const double now = 1791777600; // 2026-10-12 04:00 UTC
    try
    {
        codex::Collector collector(root);
        collector.Poll(now);
        Check(!collector.GetSnapshot().ready && !collector.GetSnapshot().five_hour, "missing logs must stay unknown");
        Write(file, Counter(30) + Counter(30) + quota + "\nmalformed token_count\n");
        codex::Collector counters(root); counters.Poll(now);
        Check(std::abs(counters.GetSnapshot().speed - 2) < 1e-9, "repeated counters are counted once");
        collector.Poll(now + 30);
        auto sample = collector.GetSnapshot();
        Check(sample.ready && !sample.error, "discovery");
        Check(sample.speed == 0, "small decayed speeds are zero");
        Check(sample.five_hour && sample.five_hour->remaining == 74.5, "five-hour remaining");
        Check(sample.weekly && sample.weekly->remaining == 40, "weekly remaining");
        Write(file, Record("r1", 1500), true);
        collector.Poll(now + 32);
        Check(std::abs(collector.GetSnapshot().speed - 100 * std::exp(-32.0 / 15)) < 1e-9, "response precedence and EMA");
        Write(file, Record("r1", 1500), true);
        collector.Poll(now + 34);
        Check(std::abs(collector.GetSnapshot().speed - 100 * std::exp(-34.0 / 15)) < 1e-9, "response dedup");
        auto archive = root / L"archived_sessions" / L"rollout.jsonl";
        std::filesystem::create_directories(archive.parent_path());
        std::filesystem::copy_file(file, archive);
        collector.Poll(now + 60);
        Check(std::abs(collector.GetSnapshot().speed - 100 * std::exp(-4)) < 1e-9, "archive dedup");
        codex::Collector restarted(root); restarted.Poll(now + 60);
        Check(std::abs(restarted.GetSnapshot().speed - collector.GetSnapshot().speed) < 1e-9, "restart consistency");
        Write(archive, "");
        Write(file, Record("r2", 300, "2026-10-12T04:01:02Z"));
        collector.Poll(now + 62);
        Check(std::abs(collector.GetSnapshot().speed - 20) < 1e-9, "truncation rebuild");
        auto partial = Record("r3", 150, "2026-10-12T12:01:04+08:00");
        Write(file, partial.substr(0, partial.size() / 2), true);
        collector.Poll(now + 64);
        double previous = collector.GetSnapshot().speed;
        Write(file, partial.substr(partial.size() / 2), true);
        collector.Poll(now + 66);
        Check(std::abs(collector.GetSnapshot().speed - (previous + 10) * std::exp(-2.0 / 15)) < 1e-9, "split line and timezone");
        Write(file, Record("bad-date", 99999, "2026-99-12T04:01:08Z") + Record("future", 99999, "2099-10-12T04:01:08Z"), true);
        collector.Poll(now + 68);
        Check(collector.GetSnapshot().speed < 30, "reject invalid/future timestamps");
        const auto newer = R"({"timestamp":"2026-10-12T04:01:10Z","type":"event_msg","payload":{"type":"token_count","rate_limits":{"limit_id":"codex","secondary":{"used_percent":120,"window_minutes":300,"resets_at":null}}}})";
        Write(file, std::string(newer) + "\n", true); collector.Poll(now + 70);
        sample = collector.GetSnapshot();
        Check(sample.five_hour && sample.five_hour->remaining == 0 && !sample.five_hour->resets_at && !sample.weekly, "duration/clamp/missing reset");
        auto unrelated = std::string(newer); unrelated.replace(unrelated.find("\"codex\""), 7, "\"codex_other\"");
        Write(file, unrelated + "\n", true); collector.Poll(now + 72);
        Check(collector.GetSnapshot().quota_updated_at == now + 70, "ignore model quota");
        Write(file, "{\"type\":\"event_msg\",\"timestamp\":\"2026-10-12T04:01:12Z\",\"payload\":\"token_count\"}\n", true);
        collector.Poll(now + 74);
        Check(!collector.GetSnapshot().error, "invalid field types must not throw");
        auto replacement = Record("r4", 300, "2026-10-12T04:01:16Z");
        Write(file, replacement); collector.Poll(now + 76);
        Check(std::abs(collector.GetSnapshot().speed - 20) < 1e-9, "replacement resets previous records");
        Sleep(20);
        auto same_size = Record("r5", 600, "2026-10-12T04:01:18Z");
        Check(replacement.size() == same_size.size(), "same-size fixture");
        Write(file, same_size); collector.Poll(now + 78);
        Check(std::abs(collector.GetSnapshot().speed - 40) < 1e-9, "same-size replacement");
        auto rotated = file; rotated += L".old";
        std::filesystem::rename(file, rotated);
        Write(file, Record("r6", 900, "2026-10-12T04:01:20Z")); collector.Poll(now + 80);
        Check(std::abs(collector.GetSnapshot().speed - 60) < 1e-9, "file identity rotation");
        collector.Poll(now + 600);
        Check(collector.GetSnapshot().speed == 0, "idle zero");
        Check(ReleaseVersion::Newer("1.100.0", "1.87.0") && !ReleaseVersion::Newer("1.9.0", "1.87.0"), "numeric version ordering");
        Check(!ReleaseVersion::Valid("1.87.0-beta") && !ReleaseVersion::Valid("1..0") && !ReleaseVersion::Valid("1.87.0.1"), "stable version validation");
        std::filesystem::remove_all(root);
        std::cout << "Codex telemetry and release version checks passed\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n'; return 1;
    }
}
