#ifndef DIRTY_BENCHMARK_H
#define DIRTY_BENCHMARK_H
#include <string>
enum class GroupAnalysisResult {
    CORRECT_DETECT = 0, // TP
    MISS_DETECT    = 1, // FN
    FALSE_ALARM    = 3, // FP
    CORRECT_CLEAN  = 4, // TN
    ERROR_SKIPPED  = 2  // 跳过/异常
};
class DirtyBenchmark {
public:
    DirtyBenchmark();
    bool runSingleGroup(const std::string& groupPath, int groundTruthValue);
    GroupAnalysisResult analyzeGroupLevel(const std::string& groupPath, bool isArchiveMode);
    void batchRunAllGroups();
    void batchAnalyzeGroups(bool isArchiveMode);
    void runSingleGroupDebug();

    // 子菜单迁移到本模块
    void showSubMenu();

private:
    // 路径常量收拢在此类
    static const std::string BENCHMARK_ROOT;
    static const std::string DIRTY_BENCHMARK_DIR;
    static const std::string REPORT_ROOT;

    // 本模块私有工具，不再全局暴露
    int countImagesInDirectory(const std::string& directoryPath);
    std::string getCurrentTimestamp();
    void initializeDirectories();

    std::string testVersion_;
    std::string testTimestamp_;
    std::string currentReportDir_;
    std::string currentExtractDir_;
};
#endif // DIRTY_BENCHMARK_H
