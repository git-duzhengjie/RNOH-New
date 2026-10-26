#pragma once

#include <cxxreact/JSBigString.h>
#include <rawfile/raw_file_manager.h>

namespace rnoh::JSBigStringHelpers {

std::unique_ptr<facebook::react::JSBigString const> fromBuffer(
    std::vector<uint8_t> buffer);

std::unique_ptr<facebook::react::JSBigString const> fromRawFilePath(
    std::string const& rawfilePath,
    NativeResourceManager* NativeResourceManager);

std::unique_ptr<facebook::react::JSBigString const> fromFilePath(
    std::string const& filePath);

} // namespace rnoh::JSBigStringHelpers
