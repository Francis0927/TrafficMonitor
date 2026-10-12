#pragma once
#include "CodexTelemetry.h"

class CCodexTelemetryView
{
public:
    static CString SpeedText(const codex::Snapshot& snapshot);
    static CString Tooltip(const codex::Snapshot& snapshot);
};
