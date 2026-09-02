#pragma once

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <regex>
#include <string>
#include <vector>

namespace timeout_collector {

namespace fs = std::filesystem;

constexpr double kTimeoutThresholdMs = 200.0;

struct TimeoutSample {
    std::string imageName;
    fs::path resultTxtPath;
    double totalMs = 0.0;
};

struct TimeoutResult {
    size_t totalCount = 0;
    std::vector<TimeoutSample> timeoutSamples;
};

inline TimeoutResult collect(const fs::path& resultGridDir)
{
    TimeoutResult result;

    if (!fs::exists(resultGridDir) || !fs::is_directory(resultGridDir)) {
        return result;
    }

    const std::regex imagePattern(R"(\[image=([^\]]+)\])");
    const std::regex totalPattern(R"(total=([0-9]+(?:\.[0-9]+)?)ms)");

    for (const auto& entry : fs::directory_iterator(resultGridDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".txt") {
            continue;
        }

        std::ifstream input(entry.path());
        if (!input.is_open()) {
            continue;
        }

        std::string line;
        while (std::getline(input, line)) {
            std::smatch imageMatch;
            std::smatch totalMatch;

            if (!std::regex_search(line, imageMatch, imagePattern) ||
                !std::regex_search(line, totalMatch, totalPattern)) {
                continue;
            }

            ++result.totalCount;

            const double totalMs = std::stod(totalMatch[1].str());
            if (totalMs > kTimeoutThresholdMs) {
                TimeoutSample sample;
                sample.imageName = imageMatch[1].str();
                sample.resultTxtPath = entry.path();
                sample.totalMs = totalMs;
                result.timeoutSamples.push_back(sample);
            }
        }
    }

    return result;
}

inline void saveTimeoutSamples(
    const fs::path& sourceImageDir,
    const fs::path& outputDir,
    const std::string& groupName,
    const TimeoutResult& timeoutResult)
{
    std::error_code ec;
    fs::create_directories(outputDir, ec);

    std::ofstream summary(outputDir / (groupName + "_timeout_summary.txt"));
    if (!summary.is_open()) {
        return;
    }

    summary << "group=" << groupName << "\n";
    summary << "timeout_threshold_ms=" << kTimeoutThresholdMs << "\n";
    summary << "total_time_log_count=" << timeoutResult.totalCount << "\n";
    summary << "timeout_count=" << timeoutResult.timeoutSamples.size() << "\n\n";

    for (const auto& sample : timeoutResult.timeoutSamples) {
        const fs::path sourceImagePath = sourceImageDir / sample.imageName;

        // 每个超时样本独立保存对应 txt，避免同一 txt 被不同图片覆盖。
        const std::string uniquePrefix =
            groupName + "_" + fs::path(sample.imageName).stem().string();

        const fs::path outputImagePath =
            outputDir / (uniquePrefix + fs::path(sample.imageName).extension().string());

        const fs::path outputTxtPath =
            outputDir / (uniquePrefix + "_" + sample.resultTxtPath.filename().string());

        if (fs::exists(sourceImagePath, ec)) {
            fs::copy_file(
                sourceImagePath,
                outputImagePath,
                fs::copy_options::overwrite_existing,
                ec);

            if (ec) {
                summary << "[WARN] copy image failed: "
                        << sourceImagePath.string()
                        << ", error=" << ec.message() << "\n";
                ec.clear();
            }
        } else {
            summary << "[WARN] source image not found: "
                    << sourceImagePath.string() << "\n";
        }

        if (fs::exists(sample.resultTxtPath, ec)) {
            fs::copy_file(
                sample.resultTxtPath,
                outputTxtPath,
                fs::copy_options::overwrite_existing,
                ec);

            if (ec) {
                summary << "[WARN] copy txt failed: "
                        << sample.resultTxtPath.string()
                        << ", error=" << ec.message() << "\n";
                ec.clear();
            }
        } else {
            summary << "[WARN] result txt not found: "
                    << sample.resultTxtPath.string() << "\n";
        }

        summary << "image=" << sample.imageName
                << ", total_ms=" << std::fixed << std::setprecision(2)
                << sample.totalMs
                << ", result_txt=" << sample.resultTxtPath.filename().string()
                << "\n";
    }
}

} // namespace timeout_collector