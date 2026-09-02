#ifndef DIRTY_BENCHMARK_H
#define DIRTY_BENCHMARK_H

#include <string>

enum class GroupAnalysisResult {
    CORRECT_DETECT = 0,
    MISS_DETECT = 1,
    ERROR_SKIPPED = 2,
    FALSE_ALARM = 3,
    CORRECT_CLEAN = 4
};

class DirtyBenchmark {
public:
    DirtyBenchmark();

    bool runSingleGroup(
        const std::string& groupPath,
        int groundTruthValue);

    GroupAnalysisResult analyzeGroupLevel(
        const std::string& groupPath,
        bool isArchiveMode);

    void batchRunAllGroups();
    void batchAnalyzeGroups(bool isArchiveMode);
    void runSingleGroupDebug();
    void showSubMenu();

private:
    static const std::string BENCHMARK_ROOT;
    static const std::string DIRTY_BENCHMARK_DIR;
    static const std::string REPORT_ROOT;

    int countImagesInDirectory(const std::string& directoryPath);
    std::string getCurrentTimestamp();
    void initializeDirectories();

    std::string testVersion_;
    std::string testTimestamp_;
    std::string currentReportDir_;
    std::string currentExtractDir_;
};

#endif // DIRTY_BENCHMARK_H