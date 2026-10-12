#pragma once
// Independent of MFC so the log reader can be tested without a desktop session.
#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace codex
{
struct QuotaWindow
{
    double remaining{};
    std::optional<double> resets_at; // Unix seconds, UTC
};
struct Snapshot
{
    double speed{}; // output tokens/s, 15 second EMA
    double quota_updated_at{};
    std::optional<QuotaWindow> five_hour, weekly;
    bool ready{}, error{}, loading{};
};

class Collector
{
public:
    explicit Collector(std::filesystem::path home = {});
    ~Collector();
    void Start();
    void Stop();
    void Poll(double now); // Called only by the monitor worker, at most every 2s.
    Snapshot GetSnapshot() const;
    static double Now();
private:
    struct Event { double time{}, output{}; std::string id; };
    struct File
    {
        uint64_t offset{}, identity{}, line{};
        std::filesystem::file_time_type modified{};
        std::string pending, session;
        bool discard_line{}, has_records{};
        std::optional<double> previous_output;
        std::vector<Event> records, counters;
        Snapshot quota;
    };
    void Discover();
    void Read(const std::filesystem::path& path, File& file, double now);
    void Accept(File& file, const std::string& line, double now);
    void Publish(double now);
    std::filesystem::path m_home;
    std::map<std::filesystem::path, File> m_files;
    double m_last_poll{-1e30}, m_last_discovery{-1e30};
    bool m_error{}, m_loading{};
    mutable std::mutex m_mutex;
    Snapshot m_snapshot;
    std::thread m_worker;
    std::atomic<bool> m_stop{false};
    std::mutex m_wait_mutex;
    std::condition_variable m_wake;
};
}
