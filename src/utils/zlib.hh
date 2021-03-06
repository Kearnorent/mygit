#pragma once

#include <string>
#include <iostream>

#include <zlib.h>

namespace utils
{
    std::string CompressString(const std::string &str,
                               int compressionlevel = Z_BEST_COMPRESSION);

    std::string DecompressString(const std::string &str);
}