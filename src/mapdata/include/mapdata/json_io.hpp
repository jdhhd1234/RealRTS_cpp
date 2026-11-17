#pragma once

#include <filesystem>
#include <string>
#include <string_view>

#include "mapdata/map.hpp"

namespace mapdata
{

// 디버그용 JSON 형식 (2장). 읽을 때는 항상 Validate까지 한다.
// 실패하면 원인을 담은 LoadError를 던진다.
std::string ToJson(const Baked& baked);
Baked FromJson(std::string_view text);

void SaveJson(const Baked& baked, const std::filesystem::path& path);
Baked LoadJson(const std::filesystem::path& path);

}
