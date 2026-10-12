#include "stdafx.h"
#include "CodexTelemetryView.h"
#include "Common.h"
#include <ctime>

namespace
{
CString LocalTime(double timestamp)
{
    auto time = static_cast<__time64_t>(timestamp);
    tm local{};
    if (_localtime64_s(&local, &time) != 0) return CCommon::LoadText(IDS_CODEX_UNKNOWN);
    wchar_t buffer[64];
    wcsftime(buffer, 64, L"%Y-%m-%d %H:%M:%S", &local);
    return buffer;
}
CString WindowText(const wchar_t* label, const std::optional<codex::QuotaWindow>& window)
{
    CString value;
    if (!window) value = CCommon::LoadText(IDS_CODEX_UNKNOWN);
    else
    {
        value.Format(L"%.1f%%", window->remaining);
        value += L" | "; value += CCommon::LoadText(IDS_CODEX_RESET); value += L": ";
        value += window->resets_at ? LocalTime(*window->resets_at) : CCommon::LoadText(IDS_CODEX_UNKNOWN);
        if (window->resets_at && *window->resets_at <= codex::Collector::Now())
        { value += L" ("; value += CCommon::LoadText(IDS_CODEX_EXPIRED); value += L")"; }
    }
    return L"\r\n" + CCommon::LoadText(label) + L": " + value;
}
}
CString CCodexTelemetryView::SpeedText(const codex::Snapshot& snapshot)
{
    if (snapshot.error || !snapshot.ready) return L"-- token/s";
    CString value; value.Format(L"%.1f token/s", snapshot.speed); return value;
}
CString CCodexTelemetryView::Tooltip(const codex::Snapshot& snapshot)
{
    CString text = L"\r\nCodex: " + SpeedText(snapshot);
    text += WindowText(IDS_CODEX_FIVE_HOUR, snapshot.five_hour);
    text += WindowText(IDS_CODEX_WEEKLY, snapshot.weekly);
    text += L"\r\n"; text += CCommon::LoadText(IDS_CODEX_UPDATED); text += L": ";
    text += snapshot.quota_updated_at > 0 ? LocalTime(snapshot.quota_updated_at) : CCommon::LoadText(IDS_CODEX_UNKNOWN);
    if (snapshot.error) { text += L"\r\n"; text += CCommon::LoadText(IDS_CODEX_READ_ERROR); }
    else if (snapshot.loading) { text += L"\r\n"; text += CCommon::LoadText(IDS_CODEX_LOADING); }
    else if (!snapshot.ready) { text += L"\r\n"; text += CCommon::LoadText(IDS_CODEX_WAITING); }
    return text;
}
