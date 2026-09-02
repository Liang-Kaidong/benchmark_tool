#include "QrBenchmark.h"

#include "SdkRuntime.h"
#include "TimeoutCollector.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

const std::string QrBenchmark::BENCHMARK_ROOT = "/userdata/benchmark";

const std::string QrBenchmark::CHARGER_QR_BENCHMARK_DIR =
    BENCHMARK_ROOT + "/charger_qr";

const std::string QrBenchmark::AREA_QR_BENCHMARK_DIR =
    BENCHMARK_ROOT + "/area_qr";

QrBenchmark::QrBenchmark() = default;

void QrBenchmark::initializeDirectories()
{
    const std::vector<std::string> dirsToCreate = {
        CHARGER_QR_BENCHMARK_DIR,
        AREA_QR_BENCHMARK_DIR,
        CHARGER_QR_BENCHMARK_DIR + "/report_output",
        AREA_QR_BENCHMARK_DIR + "/report_output"
    };

    std::error_code ec;

    for (const auto& dirPath : dirsToCreate) {
        fs::create_directories(dirPath, ec);
    }
}

int QrBenchmark::countImagesInDirectory(const std::string& directoryPath)
{
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        return 0;
    }

    int imageCount = 0;

    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string ext = entry.path().extension().string();

        for (auto& ch : ext) {
            ch = static_cast<char>(
                std::tolower(static_cast<unsigned char>(ch)));
        }

        if (ext == ".jpg" || ext == ".jpeg" ||
            ext == ".png" || ext == ".bmp") {
            ++imageCount;
        }
    }

    return imageCount;
}

std::string QrBenchmark::getCurrentTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto timeValue = std::chrono::system_clock::to_time_t(now);

    std::stringstream stream;
    stream << std::put_time(std::localtime(&timeValue), "%Y%m%d_%H%M");

    return stream.str();
}

std::string QrBenchmark::getTaskBenchRoot(QrTaskType task)
{
    if (task == QrTaskType::CHARGER_QR) {
        return CHARGER_QR_BENCHMARK_DIR;
    }

    return AREA_QR_BENCHMARK_DIR;
}

std::string QrBenchmark::getTaskReportRoot(QrTaskType task)
{
    return getTaskBenchRoot(task) + "/report_output";
}

void QrBenchmark::showSubMenu()
{
    bool inSubMenu = true;

    while (inSubMenu) {
        const int ret = std::system("clear");
        (void)ret;

        std::cout << "\n========================================================\n";
        std::cout << "               QR Benchmark 子菜单                      \n";
        std::cout << "========================================================\n\n";
        std::cout << " [1] Charger-QR 基准测试\n";
        std::cout << " [2] Area-QR 基准测试\n";
        std::cout << " [3] 返回上一级菜单\n";
        std::cout << "\n========================================================\n\n";
        std::cout << "请选择子选项 [1-3]: ";

        int option = 0;

        if (!(std::cin >> option)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        switch (option) {
        case 1:
            selectTaskAndRun(QrTaskType::CHARGER_QR);
            break;

        case 2:
            selectTaskAndRun(QrTaskType::AREA_QR);
            break;

        case 3:
            inSubMenu = false;
            break;

        default:
            std::cout << "无效选项" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            break;
        }
    }
}

void QrBenchmark::selectTaskAndRun(QrTaskType task)
{
    std::cout << "\n----------------------------------------\n";
    std::cout << " [1] 批量跑全部 group\n";
    std::cout << " [2] 单组调试（选择序号执行）\n";
    std::cout << "请选择模式 [1/2]: ";

    int mode = 0;

    if (!(std::cin >> mode)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        return;
    }

    if (mode == 1) {
        batchRunAllGroups(task);
    } else if (mode == 2) {
        runSingleGroupDebug(task);
    }
}

bool QrBenchmark::runSingleGroup(const std::string& groupPath)
{
    const std::string groupName = fs::path(groupPath).filename().string();
    const std::string groupSaveResultDir = groupPath + "/results";

    std::cout << "==============================================\n";
    std::cout << "开始执行 group: [" << groupName << "]\n";
    std::cout << "本组图片源目录: " << groupPath << '\n';
    std::cout << "==============================================\n";

    SdkRuntime::killAllSdkProcesses();

    std::error_code cleanError;
    fs::remove(SdkRuntime::SDK_IMAGES_LINK, cleanError);
    fs::remove_all(SdkRuntime::SDK_REAL_IMAGES_DIR, cleanError);
    fs::remove_all(SdkRuntime::SDK_RESULT_DIR, cleanError);

    if (cleanError) {
        std::cerr << "[CLEAN WARN] " << cleanError.message() << std::endl;
    }

    std::cout << ">> 已清理 SDK：images 软链接、真实 images 目录、result 目录残留\n";

    const std::string demoPath =
        SdkRuntime::SDK_BIN_BASE + SdkRuntime::DEMO_NAME;

    const std::string inferPath =
        SdkRuntime::SDK_BIN_BASE + SdkRuntime::INFER_NAME;

    if (!fs::exists(SdkRuntime::SDK_BIN_BASE)) {
        std::cout << "[ERROR] SDK bin 目录不存在: "
                  << SdkRuntime::SDK_BIN_BASE << std::endl;
        return false;
    }

    if (!fs::exists(demoPath) || !fs::exists(inferPath)) {
        std::cout << "[ERROR] SDK 二进制缺失，跳过本组" << std::endl;
        return false;
    }

    const int imageCount = countImagesInDirectory(groupPath);

    if (imageCount == 0) {
        std::cout << "[WARN] group " << groupName
                  << " 无图片，跳过本组" << std::endl;
        return false;
    }

    std::cout << ">> 本组图片总数量：" << imageCount << " 张\n" << std::endl;

    const fs::path originalWorkDir = fs::current_path();
    fs::current_path(SdkRuntime::SDK_BIN_BASE);

    std::cout << ">> 启动 " << SdkRuntime::DEMO_NAME
              << " --enable-all" << std::endl;

    const std::string startDemoCmd =
        "./" + SdkRuntime::DEMO_NAME + " --enable-all &";

    const int demoRet = std::system(startDemoCmd.c_str());
    (void)demoRet;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::error_code ec;
    fs::create_directory_symlink(groupPath, SdkRuntime::SDK_IMAGES_LINK, ec);
    fs::create_directories(SdkRuntime::SDK_RESULT_GRID, ec);

    std::cout << ">> 执行 " << SdkRuntime::INFER_NAME
              << " -d -t -s" << std::endl;

    const std::string startInferCmd =
        "./" + SdkRuntime::INFER_NAME + " -d -t -s";

    const int inferRet = std::system(startInferCmd.c_str());

    if (inferRet == -1) {
        std::cerr << "[WARN] infer 进程 system 调用失败，group="
                  << groupName << std::endl;
    }

    SdkRuntime::killProcessByName(SdkRuntime::INFER_NAME);

    int outputTxtCount = 0;

    if (fs::exists(SdkRuntime::SDK_RESULT_GRID)) {
        for (const auto& item :
             fs::directory_iterator(SdkRuntime::SDK_RESULT_GRID)) {
            if (item.path().extension() == ".txt") {
                ++outputTxtCount;
            }
        }
    }

    std::cout << ">> 输出 txt 文件数量: " << outputTxtCount << std::endl;

    if (outputTxtCount != imageCount) {
        std::cout << "!!! 警告：txt 数量与图片数量不一致！"
                  << std::endl;
    }

    fs::current_path(originalWorkDir);

    fs::remove_all(groupSaveResultDir, ec);

    fs::copy(
        SdkRuntime::SDK_RESULT_DIR,
        groupSaveResultDir,
        fs::copy_options::recursive,
        ec);

    if (ec) {
        std::cerr << "[WARN] 保存推理结果失败: "
                  << groupSaveResultDir
                  << ", error=" << ec.message() << std::endl;
        ec.clear();
    } else {
        const fs::path savedGridDir =
            fs::path(groupSaveResultDir) / "grid";

        const auto timeoutResult =
            timeout_collector::collect(savedGridDir);

        const fs::path timeoutOutputDir =
            fs::path(groupSaveResultDir) / "timeout_samples";

        timeout_collector::saveTimeoutSamples(
            fs::path(groupPath),
            timeoutOutputDir,
            groupName,
            timeoutResult);

        std::cout << ">> 时间性能统计：总记录 "
                  << timeoutResult.totalCount
                  << " 条，超时（>200ms） "
                  << timeoutResult.timeoutSamples.size()
                  << " 条" << std::endl;

        if (!timeoutResult.timeoutSamples.empty()) {
            std::cout << ">> 超时图片和对应 result txt 已保存至: "
                      << timeoutOutputDir.string() << std::endl;
        }
    }

    SdkRuntime::killAllSdkProcesses();
    fs::remove(SdkRuntime::SDK_IMAGES_LINK, ec);

    std::cout << ">> group " << groupName << " 推理完成\n" << std::endl;

    return true;
}

std::vector<QrSampleResult> QrBenchmark::parseGroupGridLog(
    const std::string& groupGridDir,
    QrTaskType task)
{
    std::vector<QrSampleResult> results;

    if (!fs::exists(groupGridDir)) {
        return results;
    }

    for (const auto& entry : fs::directory_iterator(groupGridDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string extension = entry.path().extension().string();

        for (auto& ch : extension) {
            ch = static_cast<char>(
                std::tolower(static_cast<unsigned char>(ch)));
        }

        if (extension != ".txt") {
            continue;
        }

        std::ifstream input(entry.path());

        if (!input.is_open()) {
            continue;
        }

        std::string line;
        std::string currentImageName;
        int qrFieldValue = -1;

        while (std::getline(input, line)) {
            std::string imageName;

            if (extractImageName(line, imageName)) {
                if (!currentImageName.empty() && qrFieldValue != -1) {
                    results.push_back({
                        currentImageName,
                        entry.path().filename().string(),
                        qrFieldValue == 1 || qrFieldValue == 2
                    });
                }

                currentImageName = imageName;
                qrFieldValue = -1;
            }

            int value = -1;

            if (task == QrTaskType::CHARGER_QR) {
                if (parseChargerQrLine(line, value)) {
                    qrFieldValue = value;
                }
            } else {
                if (parseAreaQrLine(line, value)) {
                    qrFieldValue = value;
                }
            }
        }

        if (!currentImageName.empty() && qrFieldValue != -1) {
            results.push_back({
                currentImageName,
                entry.path().filename().string(),
                qrFieldValue == 1 || qrFieldValue == 2
            });
        }
    }

    return results;
}

QrGroupAnalysisResult QrBenchmark::analyzeGroupLevel(
    const std::string& groupPath,
    QrTaskType task,
    bool isArchiveMode)
{
    const std::string groupName = fs::path(groupPath).filename().string();
    const std::string groupResultGridPath = groupPath + "/results/grid";
    const std::string groupStatOutPath = groupPath + "/group_stat.txt";

    std::cout << "----------------------------------------\n";
    std::cout << "组级别分析 group: " << groupName << '\n';
    std::cout << "----------------------------------------\n";

    if (!fs::exists(groupResultGridPath)) {
        std::cout << "[ERROR] " << groupName
                  << " 无推理结果目录，跳过" << std::endl;
        return QrGroupAnalysisResult::ERROR_SKIPPED;
    }

    const auto sampleList = parseGroupGridLog(groupResultGridPath, task);
    const size_t total = sampleList.size();

    size_t successCount = 0;

    for (const auto& sample : sampleList) {
        if (sample.isSuccess) {
            ++successCount;
        }
    }

    const size_t failCount = total - successCount;

    const double decodeRate =
        total > 0
            ? static_cast<double>(successCount) / total * 100.0
            : 0.0;

    if (isArchiveMode && !currentFailExtractDir_.empty()) {
        std::error_code ec;

        for (const auto& sample : sampleList) {
            if (sample.isSuccess) {
                continue;
            }

            const fs::path sourceImage =
                fs::path(groupPath) / sample.imageName;

            const fs::path targetImage =
                fs::path(currentFailExtractDir_) /
                (groupName + "_" + sample.imageName);

            if (!fs::exists(sourceImage, ec)) {
                continue;
            }

            fs::copy_file(
                sourceImage,
                targetImage,
                fs::copy_options::overwrite_existing,
                ec);

            const fs::path sourceTxt =
                fs::path(groupResultGridPath) / sample.logFileName;

            const fs::path targetTxt =
                fs::path(currentFailExtractDir_) /
                (groupName + "_" + sample.logFileName);

            if (fs::exists(sourceTxt, ec)) {
                fs::copy_file(
                    sourceTxt,
                    targetTxt,
                    fs::copy_options::overwrite_existing,
                    ec);
            } else {
                std::cout << "[WARN] 未找到失败图片对应的推理 txt: "
                          << sourceTxt.string() << std::endl;
            }
        }
    }

    std::ofstream statStream(groupStatOutPath);

    if (statStream.is_open()) {
        statStream << "==== QR Group-Level Stat ====\n";
        statStream << "GroupName: " << groupName << '\n';
        statStream << "TotalSamples: " << total << '\n';
        statStream << "SuccessCount: " << successCount << '\n';
        statStream << "FailCount: " << failCount << '\n';
        statStream << std::fixed << std::setprecision(2);
        statStream << "DecodeRatePercent: " << decodeRate << '\n';
        statStream << "=============================\n";
    }

    std::ifstream readBack(groupStatOutPath);
    std::cout << readBack.rdbuf();

    return QrGroupAnalysisResult::GROUP_OK;
}

void QrBenchmark::batchRunAllGroups(QrTaskType task)
{
    initializeDirectories();

    const std::string taskBenchRoot = getTaskBenchRoot(task);
    const std::string taskReportRoot = getTaskReportRoot(task);

    std::vector<fs::path> groupPaths;

    if (fs::exists(taskBenchRoot)) {
        for (const auto& entry : fs::directory_iterator(taskBenchRoot)) {
            if (entry.is_directory()) {
                groupPaths.push_back(entry.path());
            }
        }
    }

    std::sort(groupPaths.begin(), groupPaths.end());

    if (groupPaths.empty()) {
        std::cout << "错误：" << taskBenchRoot
                  << " 下没有 group 文件夹" << std::endl;
        std::cout << "回车返回...";
        std::cin.ignore();
        std::cin.get();
        return;
    }

    std::cout << "\n请输入本次测试版本号（例如 V1.2.0）: ";
    std::cin >> testVersion_;

    while (testVersion_.empty()) {
        std::cout << "版本号不能为空！\n请输入版本号: ";
        std::cin >> testVersion_;
    }

    testTimestamp_ = getCurrentTimestamp();
    currentReportDir_ =
        taskReportRoot + "/" + testVersion_ + "/" + testTimestamp_;

    currentFailExtractDir_ = currentReportDir_ + "/failed_images";

    std::error_code ec;
    fs::create_directories(currentFailExtractDir_, ec);

    std::cout << "\n测试版本: " << testVersion_ << '\n';
    std::cout << "系统时间戳: " << testTimestamp_ << '\n';
    std::cout << "报告目录: " << currentReportDir_ << '\n';
    std::cout << "失败图片归档目录: "
              << currentFailExtractDir_ << "\n\n";

    std::cout << "待处理 groups:\n";

    for (const auto& path : groupPaths) {
        std::cout << path.string() << '\n';
    }

    std::cout << "\n确认开始批量推理？回车继续，Ctrl-C 退出";
    std::cin.ignore();
    std::cin.get();

    std::vector<QrSampleResult> allSamples;

    for (const auto& groupPath : groupPaths) {
        const std::string groupName =
            groupPath.filename().string();

        std::cout << "\n==============================================\n";
        std::cout << "准备执行 group: [" << groupName << "]\n";
        std::cout << "图片目录: " << groupPath.string() << '\n';

        runSingleGroup(groupPath.string());
        analyzeGroupLevel(groupPath.string(), task, true);

        const auto groupSamples = parseGroupGridLog(
            groupPath.string() + "/results/grid",
            task);

        allSamples.insert(
            allSamples.end(),
            groupSamples.begin(),
            groupSamples.end());
    }

    const size_t totalCount = allSamples.size();

    size_t successCount = 0;

    for (const auto& sample : allSamples) {
        if (sample.isSuccess) {
            ++successCount;
        }
    }

    const size_t failCount = totalCount - successCount;

    const double decodeRate =
        totalCount > 0
            ? static_cast<double>(successCount) / totalCount * 100.0
            : 0.0;

    const std::string summaryPath = currentReportDir_ + "/summary.txt";
    std::ofstream summaryFile(summaryPath);

    if (summaryFile.is_open()) {
        summaryFile << "SDK_VERSION=" << testVersion_ << '\n';
        summaryFile << "TIMESTAMP=" << testTimestamp_ << '\n';
        summaryFile << "TASK="
                    << (task == QrTaskType::CHARGER_QR
                            ? "CHARGER_QR"
                            : "AREA_QR")
                    << '\n';
        summaryFile << "TOTAL_SAMPLES=" << totalCount << '\n';
        summaryFile << "SUCCESS_COUNT=" << successCount << '\n';
        summaryFile << "FAIL_COUNT=" << failCount << '\n';
        summaryFile << std::fixed << std::setprecision(2);
        summaryFile << "DECODE_RATE_PERCENT=" << decodeRate << '\n';
    }

    const std::string detailPath = currentReportDir_ + "/detail.txt";
    std::ofstream detailFile(detailPath);

    if (detailFile.is_open()) {
        detailFile << "# image_name status log_file\n";

        for (const auto& sample : allSamples) {
            detailFile << sample.imageName << ' '
                       << (sample.isSuccess ? "success" : "failed")
                       << ' '
                       << sample.logFileName << '\n';
        }
    }

    for (const auto& groupPath : groupPaths) {
        const std::string groupName =
            groupPath.filename().string();

        const std::string destinationDir =
            currentReportDir_ + "/" + groupName;

        fs::create_directories(destinationDir, ec);

        const std::string sourceStatFile =
            groupPath.string() + "/group_stat.txt";

        if (fs::exists(sourceStatFile)) {
            fs::copy(
                sourceStatFile,
                destinationDir + "/group_stat.txt",
                fs::copy_options::overwrite_existing,
                ec);
        }
    }

    std::cout << "\n==== QR 全部 group 推理与分析完成 ====\n";
    std::cout << "全局总样本: " << totalCount
              << "，成功: " << successCount
              << "，失败: " << failCount
              << "，解码率: "
              << std::fixed << std::setprecision(2)
              << decodeRate << "%\n";

    std::cout << "报告归档至：" << currentReportDir_ << std::endl;

    testVersion_.clear();
    testTimestamp_.clear();
    currentReportDir_.clear();
    currentFailExtractDir_.clear();

    std::cout << "\n回车返回菜单...";
    std::cin.ignore();
    std::cin.get();
}

void QrBenchmark::runSingleGroupDebug(QrTaskType task)
{
    const std::string taskBenchRoot = getTaskBenchRoot(task);
    const std::string taskReportRoot = getTaskReportRoot(task);

    std::vector<fs::path> groupPaths;

    if (fs::exists(taskBenchRoot)) {
        for (const auto& entry : fs::directory_iterator(taskBenchRoot)) {
            if (entry.is_directory()) {
                groupPaths.push_back(entry.path());
            }
        }
    }

    std::sort(groupPaths.begin(), groupPaths.end());

    if (groupPaths.empty()) {
        std::cout << "[WARN] " << taskBenchRoot
                  << " 没有可用 group 文件夹！" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
        return;
    }

    std::cout << "\n=== 可用 Group 列表 ===\n";

    for (size_t index = 0; index < groupPaths.size(); ++index) {
        std::cout << " [" << index + 1 << "] "
                  << groupPaths[index].filename().string()
                  << '\n';
    }

    std::cout << "\n请输入要调试的 group 序号: ";

    size_t selectedIndex = 0;

    if (!(std::cin >> selectedIndex) ||
        selectedIndex < 1 ||
        selectedIndex > groupPaths.size()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');

        std::cout << "[ERROR] 序号超出范围或者 group 不存在"
                  << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
        return;
    }

    const fs::path selectedPath = groupPaths[selectedIndex - 1];
    const std::string groupName = selectedPath.filename().string();

    const int imageCount =
        countImagesInDirectory(selectedPath.string());

    std::cout << "\n调试 group: " << groupName
              << "，图片数: " << imageCount << std::endl;

    std::cout << "请输入本次测试版本号（例如 V1.2.0）: ";
    std::cin >> testVersion_;

    while (testVersion_.empty()) {
        std::cout << "版本号不能为空！\n请输入版本号: ";
        std::cin >> testVersion_;
    }

    testTimestamp_ = getCurrentTimestamp();

    currentReportDir_ =
        taskReportRoot + "/" + testVersion_ + "/" +
        testTimestamp_ + "_" + groupName;

    currentFailExtractDir_ = currentReportDir_ + "/failed_images";

    std::error_code ec;
    fs::create_directories(currentFailExtractDir_, ec);

    std::cout << "\n测试版本: " << testVersion_ << '\n';
    std::cout << "系统时间戳: " << testTimestamp_ << '\n';
    std::cout << "报告目录: " << currentReportDir_ << '\n';
    std::cout << "失败图片目录: "
              << currentFailExtractDir_ << "\n\n";

    runSingleGroup(selectedPath.string());

    std::cout << "\n==== 单组推理完成，执行组级别分析并归档 ====\n";

    const QrGroupAnalysisResult analysisResult =
        analyzeGroupLevel(selectedPath.string(), task, true);

    (void)analysisResult;

    fs::create_directories(currentReportDir_, ec);

    const std::string sourceStatFile =
        selectedPath.string() + "/group_stat.txt";

    if (fs::exists(sourceStatFile)) {
        fs::copy(
            sourceStatFile,
            currentReportDir_ + "/group_stat.txt",
            fs::copy_options::overwrite_existing,
            ec);
    }

    const auto groupSamples = parseGroupGridLog(
        selectedPath.string() + "/results/grid",
        task);

    const size_t totalCount = groupSamples.size();

    size_t successCount = 0;

    for (const auto& sample : groupSamples) {
        if (sample.isSuccess) {
            ++successCount;
        }
    }

    const size_t failCount = totalCount - successCount;

    const double decodeRate =
        totalCount > 0
            ? static_cast<double>(successCount) / totalCount * 100.0
            : 0.0;

    const std::string summaryPath =
        currentReportDir_ + "/single_summary.txt";

    std::ofstream summaryFile(summaryPath);

    if (summaryFile.is_open()) {
        summaryFile << "==== Single-Group QR Benchmark Summary ====\n";
        summaryFile << "GroupName: " << groupName << '\n';
        summaryFile << "Version: " << testVersion_ << '\n';
        summaryFile << "Timestamp: " << testTimestamp_ << '\n';
        summaryFile << "Task: "
                    << (task == QrTaskType::CHARGER_QR
                            ? "CHARGER_QR"
                            : "AREA_QR")
                    << '\n';

        summaryFile << "Total: " << totalCount
                    << " Success: " << successCount
                    << " Fail: " << failCount << '\n';

        summaryFile << std::fixed << std::setprecision(2);
        summaryFile << "DecodeRate: " << decodeRate << "%\n";
        summaryFile << "========================================\n";
    }

    std::ifstream readSummary(summaryPath);
    std::cout << readSummary.rdbuf();

    testVersion_.clear();
    testTimestamp_.clear();
    currentReportDir_.clear();
    currentFailExtractDir_.clear();

    std::cout << "\n单组执行与归档完毕，回车返回";
    std::cin.ignore();
    std::cin.get();
}

bool QrBenchmark::extractImageName(
    const std::string& line,
    std::string& outImageName)
{
    const size_t begin = line.find("[image=");

    if (begin == std::string::npos) {
        return false;
    }

    const size_t end = line.find(']', begin);

    if (end == std::string::npos) {
        return false;
    }

    outImageName = line.substr(begin + 7, end - (begin + 7));

    return !outImageName.empty();
}

bool QrBenchmark::parseChargerQrLine(
    const std::string& line,
    int& outVal)
{
    const std::string key =
        "tShmChargeQrDetect.u8QrCodeDetect=";

    const size_t position = line.find(key);

    if (position == std::string::npos) {
        return false;
    }

    std::string value = line.substr(position + key.size());

    value.erase(
        std::remove_if(
            value.begin(),
            value.end(),
            [](char ch) {
                return std::isspace(
                    static_cast<unsigned char>(ch));
            }),
        value.end());

    outVal = std::stoi(value);

    return true;
}

bool QrBenchmark::parseAreaQrLine(
    const std::string& line,
    int& outVal)
{
    const std::string key =
        "tShmAreaQrDetect.s8AreaId=";

    const size_t position = line.find(key);

    if (position == std::string::npos) {
        return false;
    }

    std::string value = line.substr(position + key.size());

    value.erase(
        std::remove_if(
            value.begin(),
            value.end(),
            [](char ch) {
                return std::isspace(
                    static_cast<unsigned char>(ch));
            }),
        value.end());

    outVal = std::stoi(value);

    return true;
}