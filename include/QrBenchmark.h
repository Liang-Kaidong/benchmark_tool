#ifndef QR_BENCHMARK_H
#define QR_BENCHMARK_H
#include <string>
#include <vector>
#include <filesystem>

enum class QrTaskType
{
    CHARGER_QR,
    AREA_QR
};
struct QrSampleResult
{
    std::string imageName;
    std::string logFileName;
    bool isSuccess;
};
enum class QrGroupAnalysisResult
{
    ERROR_SKIPPED,
    GROUP_OK
};
class QrBenchmark {
public:
    QrBenchmark();
    void showSubMenu();
private:
    static const std::string BENCHMARK_ROOT;
    static const std::string CHARGER_QR_BENCHMARK_DIR;
    static const std::string AREA_QR_BENCHMARK_DIR;

    void initializeDirectories();
    int countImagesInDirectory(const std::string& directoryPath);
    std::string getCurrentTimestamp();

    void selectTaskAndRun(QrTaskType task);
    std::string getTaskBenchRoot(QrTaskType task);
    std::string getTaskReportRoot(QrTaskType task);
    bool runSingleGroup(const std::string& groupPath);
    QrGroupAnalysisResult analyzeGroupLevel(const std::string& groupPath, QrTaskType task, bool isArchiveMode);
    void batchRunAllGroups(QrTaskType task);
    void runSingleGroupDebug(QrTaskType task);

    bool extractImageName(const std::string& line, std::string& outImageName);
    bool parseChargerQrLine(const std::string& line, int& outVal);
    bool parseAreaQrLine(const std::string& line, int& outVal);
    std::vector<QrSampleResult> parseGroupGridLog(const std::string& groupGridDir, QrTaskType task);

    std::string testVersion_;
    std::string testTimestamp_;
    std::string currentReportDir_;
    std::string currentFailExtractDir_;
};
#endif // QR_BENCHMARK_H
