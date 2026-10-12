#include "stdafx.h"
#include "UpdateHelper.h"
#include "Common.h"
#include "../third_party/nlohmann/json.hpp"
#include "ReleaseVersion.h"

CUpdateHelper::CUpdateHelper() = default;
CUpdateHelper::~CUpdateHelper() = default;
void CUpdateHelper::SetUpdateSource(UpdateSource) { m_update_source = UpdateSource::GitHubSource; }

bool CUpdateHelper::CheckForUpdate()
{
    m_version.clear(); m_link.clear(); m_link64.clear(); m_link_arm64ec.clear();
    std::string response;
    if (!CCommon::GetURL(L"https://api.github.com/repos/Francis0927/TrafficMonitor/releases/latest", response, L"TrafficMonitor/1.87.0")) return false;
    auto release = nlohmann::json::parse(response, nullptr, false);
    if (!release.is_object()) return false;
    auto tag_value = release.find("tag_name"), draft = release.find("draft"), prerelease = release.find("prerelease");
    if (tag_value == release.end() || !tag_value->is_string() || draft == release.end() || !draft->is_boolean() || draft->get<bool>() || prerelease == release.end() || !prerelease->is_boolean() || prerelease->get<bool>()) return false;
    auto tag = tag_value->get<std::string>();
    if (tag.empty() || tag.front() != 'v' || !ReleaseVersion::Valid(tag.substr(1))) return false;
    auto version = tag.substr(1);
    m_version = CCommon::StrToUnicode(version.c_str(), true);
    auto assets = release.find("assets");
    if (assets == release.end() || !assets->is_array()) return false;
    const std::string prefix = "https://github.com/Francis0927/TrafficMonitor/releases/download/" + tag + "/";
    for (const auto& asset : *assets)
    {
        if (!asset.is_object()) continue;
        auto name = asset.find("name"), url = asset.find("browser_download_url");
        if (name == asset.end() || url == asset.end() || !name->is_string() || !url->is_string()) continue;
        auto asset_name = name->get<std::string>();
        auto asset_url = url->get<std::string>();
        if (asset_url != prefix + asset_name) continue;
        auto link = CCommon::StrToUnicode(asset_url.c_str(), true);
        if (asset_name == "TrafficMonitor-v" + version + "-x64.zip") m_link64 = link;
        if (asset_name == "TrafficMonitor-v" + version + "-x86.zip") m_link = link;
        if (asset_name == "TrafficMonitor-v" + version + "-arm64ec.zip") m_link_arm64ec = link;
    }
    auto page = std::wstring(L"https://github.com/Francis0927/TrafficMonitor/releases/tag/") + CCommon::StrToUnicode(tag.c_str(), true);
    if (m_link.empty()) m_link = page;
    if (m_link64.empty()) m_link64 = page;
    if (m_link_arm64ec.empty()) m_link_arm64ec = page;
    auto body = release.find("body");
    m_contents_en = body != release.end() && body->is_string() ? CCommon::StrToUnicode(body->get<std::string>().c_str(), true) : L"";
    m_contents_zh_cn = m_contents_zh_tw = m_contents_en;
    return true;
}
const std::wstring& CUpdateHelper::GetVersion() const { return m_version; }
const std::wstring& CUpdateHelper::GetLink() const { return m_link; }
const std::wstring& CUpdateHelper::GetLink64() const { return m_link64; }
const std::wstring& CUpdateHelper::GetLinkArm64ec() const { return m_link_arm64ec; }
const std::wstring& CUpdateHelper::GetContentsEn() const { return m_contents_en; }
const std::wstring& CUpdateHelper::GetContentsZhCn() const { return m_contents_zh_cn; }
const std::wstring& CUpdateHelper::GetContentsZhTw() const { return m_contents_zh_tw; }
bool CUpdateHelper::IsRowData() { return true; }