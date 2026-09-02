#include "DirtyBenchmark.h"

#include "SdkRuntime.h"
#include "TimeoutCollector.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

const std::string DirtyBenchmark::BENCHMARK_ROOT =
    "/userdata/benchmark";

const std::string DirtyBenchmark::DIRTY_BENCHMARK_DIR =
    BENCHMARK_ROOT + "/dirty_benchmark";

const std::string DirtyBenchmark::REPORT_ROOT =
    DIRTY_BENCHMARK_DIR + "/report_output";

DirtyBenchmark::DirtyBenchmark() = default;

void DirtyBenchmark::initializeDirectories()
{
    const std::vector<std::string> directories = {
        DIRTY_BENCHMARK_DIR,
        REPORT_ROOT
    };

    std::error_code ec;

    for (const auto& directory : directories) {
        fs::create_directories(directory, ec);
    }
}

int DirtyBenchmark::countImagesInDirectory(
    const std::string& directoryPath)
{
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        return 0;
    }

    int imageCount = 0;

    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string extension = entry.path().extension().string();

        for (auto& ch : extension) {
            ch = static_cast<char>(
                std::tolower(static_cast<unsigned char>(ch)));
        }

        if (extension == ".jpg" || extension == ".jpeg" ||
            extension == ".png" || extension == ".bmp") {
            ++imageCount;
        }
    }

    return imageCount;
}

std::string DirtyBenchmark::getCurrentTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto timeValue = std::chrono::system_clock::to_time_t(now);

    std::stringstream stream;
    stream << std::put_time(std::localtime(&timeValue), "%Y%m%d_%H%M");

    return stream.str();
}

void DirtyBenchmark::showSubMenu()
{
    bool inSubMenu = true;

    while (inSubMenu) {
        const int ret = std::system("clear");
        (void)ret;

        std::cout << "\n========================================================\n";
        std::cout << "              Dirty-Benchmark 子菜单                    \n";
        std::cout << "========================================================\n\n";
        std::cout << " [1] 批量推理 + 交互式真值 + 归档报告\n";
        std::cout << " [2] 仅重新组级别分析（不归档，不拷贝图片）\n";
        std::cout << " [3] 单组调试推理（带归档报告）\n";
        std::cout << " [4] 返回上一级\n";
        std::cout << "\n========================================================\n";
        std::cout << "请选择 [1-4]: ";

        int option = 0;

        if (!(std::cin >> option)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        switch (option) {
        case 1:
            batchRunAllGroups();
            break;

        case 2:
            batchAnalyzeGroups(false);
            break;

        case 3:
            runSingleGroupDebug();
            break;

        case 4:
            inSubMenu = false;
            break;

        default:
            std::cout << "无效选项" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            break;
        }
    }
}

bool DirtyBenchmark::runSingleGroup(
    const std::string& groupPath,
    int groundTruthValue)
{
    const std::string groupName = fs::path(groupPath).filename().string();
    const std::string groupGtFile = groupPath + "/group_gt";
    const std::string groupSaveResultDir = groupPath + "/results";

    std::ofstream gtFile(groupGtFile);

    if (gtFile.is_open()) {
        gtFile << groundTruthValue << '\n';
    }

    std::cout << "==============================================\n";
    std::cout << "开始执行 group: [" << groupName << "]\n";
    std::cout << "本组图片源目录: " << groupPath << '\n';
    std::cout << "本组序列真值: " << groundTruthValue << '\n';
    std::cout << "==============================================\n";

    SdkRuntime::killAllSdkProcesses();

    std::error_code cleanError;
    fs::remove(SdkRuntime::SDK_IMAGES_LINK, cleanError);
    fs::remove_all(SdkRuntime::SDK_REAL_IMAGES_DIR, cleanError);
    fs::remove_all(SdkRuntime::SDK_RESULT_DIR, cleanError);

    if (cleanError) {
        std::cerr << "[CLEAN WARN] "
                  << cleanError.message() << std::endl;
    }

    std::cout << ">> 已清理 SDK：images 软链接、真实 images 目录、"
              << "result 目录残留\n";

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
        std::cout << "[ERROR] SDK 二进制缺失，跳过本组"
                  << std::endl;
        return false;
    }

    const int imageCount = countImagesInDirectory(groupPath);

    if (imageCount == 0) {
        std::cout << "[WARN] group " << groupName
                  << " 无图片，跳过本组" << std::endl;
        return false;
    }

    std::cout << ">> 本组图片总数量：" << imageCount << " 张\n"
              << std::endl;

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

    std::cout << ">> 输出 txt 文件数量: "
              << outputTxtCount << std::endl;

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

    std::cout << ">> group " << groupName << " 推理完成\n"
              << std::endl;

    return true;
}

GroupAnalysisResult DirtyBenchmark::analyzeGroupLevel(
    const std::string& groupPath,
    bool isArchiveMode)
{
    const std::string groupName = fs::path(groupPath).filename().string();
    const std::string groupGtFilePath = groupPath + "/group_gt";
    const std::string groupResultGridPath = groupPath + "/results/grid";
    const std::string groupResultDirtPath = groupPath + "/results/dirt";
    const std::string groupStatOutPath = groupPath + "/group_stat.txt";

    std::string firstDirtyImageName;
    fs::path firstDirtyTxtPath;

    std::cout << "----------------------------------------\n";
    std::cout << "组级别分析 group: " << groupName << '\n';
    std::cout << "----------------------------------------\n";

    if (!fs::exists(groupGtFilePath)) {
        std::cout << "[ERROR] " << groupName
                  << " 缺少 group_gt，跳过" << std::endl;
        return GroupAnalysisResult::ERROR_SKIPPED;
    }

    std::ifstream gtFile(groupGtFilePath);
    std::string gtString;
    gtFile >> gtString;

    if (gtString != "0" && gtString != "1") {
        std::cout << "[ERROR] " << groupName
                  << " group_gt 必须为 0 或 1" << std::endl;
        return GroupAnalysisResult::ERROR_SKIPPED;
    }

    const int groundTruth = std::stoi(gtString);

    if (!fs::exists(groupResultGridPath)) {
        std::cout << "[ERROR] " << groupName
                  << " 无推理结果目录，跳过" << std::endl;
        return GroupAnalysisResult::ERROR_SKIPPED;
    }

    std::vector<fs::path> txtFiles;

    for (const auto& entry : fs::directory_iterator(groupResultGridPath)) {
        if (entry.path().extension() == ".txt") {
            txtFiles.push_back(entry.path());
        }
    }

    std::sort(txtFiles.begin(), txtFiles.end());

    for (const auto& txtPath : txtFiles) {
        std::ifstream textStream(txtPath);
        std::string line;
        std::string imageName;
        std::string dirtValue;

        while (std::getline(textStream, line)) {
            const size_t imageBegin = line.find("[image=");

            if (imageBegin != std::string::npos) {
                const size_t imageEnd = line.find(']', imageBegin);

                if (imageEnd != std::string::npos) {
                    imageName = line.substr(
                        imageBegin + 7,
                        imageEnd - (imageBegin + 7));
                }
            }

            const size_t dirtPosition =
                line.find("tShmAIModelStatus.u8DirtDetect");

            if (dirtPosition == std::string::npos) {
                continue;
            }

            const size_t equalPosition =
                line.find('=', dirtPosition);

            if (equalPosition == std::string::npos) {
                continue;
            }

            dirtValue = line.substr(equalPosition + 1);

            dirtValue.erase(
                std::remove_if(
                    dirtValue.begin(),
                    dirtValue.end(),
                    [](char ch) {
                        return std::isspace(
                            static_cast<unsigned char>(ch));
                    }),
                dirtValue.end());
        }

        if (dirtValue == "1" || dirtValue == "2") {
            firstDirtyImageName = imageName;
            firstDirtyTxtPath = txtPath;
            break;
        }
    }

    GroupAnalysisResult analysisResult;
    std::string resultText;

    if (groundTruth == 1) {
        if (!firstDirtyImageName.empty()) {
            analysisResult = GroupAnalysisResult::CORRECT_DETECT;
            resultText = "CORRECT_DETECT";
        } else {
            analysisResult = GroupAnalysisResult::MISS_DETECT;
            resultText = "MISS_DETECT";
        }
    } else {
        if (!firstDirtyImageName.empty()) {
            analysisResult = GroupAnalysisResult::FALSE_ALARM;
            resultText = "FALSE_ALARM";
        } else {
            analysisResult = GroupAnalysisResult::CORRECT_CLEAN;
            resultText = "CORRECT_CLEAN";
        }
    }

    if (isArchiveMode &&
        !firstDirtyImageName.empty() &&
        fs::exists(groupResultDirtPath) &&
        !currentExtractDir_.empty()) {
        const std::string baseStem =
            fs::path(firstDirtyImageName).stem().string();

        const std::string targetPattern = baseStem + "_dirt.jpg";
        bool foundRenderedImage = false;

        for (const auto& entry :
             fs::directory_iterator(groupResultDirtPath)) {
            if (entry.path().filename().string().find(targetPattern) ==
                std::string::npos) {
                continue;
            }

            const std::string imageDestination =
                currentExtractDir_ + "/" + groupName +
                "_FIRST_DIRTY_" + entry.path().filename().string();

            std::error_code ec;

            fs::copy(
                entry.path(),
                imageDestination,
                fs::copy_options::overwrite_existing,
                ec);

            std::cout << ">> 提取首次脏污渲染图: "
                      << entry.path().filename().string()
                      << " -> " << currentExtractDir_ << std::endl;

            if (!firstDirtyTxtPath.empty()) {
                const fs::path txtDestination =
                    fs::path(currentExtractDir_) /
                    (groupName + "_FIRST_DIRTY_" +
                     firstDirtyTxtPath.filename().string());

                fs::copy_file(
                    firstDirtyTxtPath,
                    txtDestination,
                    fs::copy_options::overwrite_existing,
                    ec);

                std::cout << ">> 提取对应推理 txt: "
                          << firstDirtyTxtPath.filename().string()
                          << " -> " << currentExtractDir_ << std::endl;
            }

            foundRenderedImage = true;
            break;
        }

        if (!foundRenderedImage) {
            std::cout << "[WARN] " << groupName
                      << ": results/dirt 未找到 "
                      << baseStem << "_dirt.jpg" << std::endl;
        }
    }

    std::ofstream statStream(groupStatOutPath);

    if (statStream.is_open()) {
        statStream << "==== Group-Level Stat ====\n";
        statStream << "GroupName: " << groupName << '\n';
        statStream << "GroupGT(0=无脏污,1=后期出现脏污): "
                   << groundTruth << '\n';
        statStream << "GroupResult: " << resultText << '\n';

        if (!firstDirtyImageName.empty()) {
            statStream << "FirstDirtyFrame(原始图片): "
                       << firstDirtyImageName << '\n';
        } else {
            statStream << "FirstDirtyFrame: "
                       << "NONE(本组全程未报脏污)\n";
        }

        statStream << "==========================\n";
    }

    std::ifstream readBack(groupStatOutPath);
    std::cout << readBack.rdbuf();

    return analysisResult;
}

void DirtyBenchmark::batchAnalyzeGroups(bool isArchiveMode)
{
    initializeDirectories();

    std::vector<fs::path> groupPaths;

    if (fs::exists(DIRTY_BENCHMARK_DIR)) {
        for (const auto& entry :
             fs::directory_iterator(DIRTY_BENCHMARK_DIR)) {
            if (!entry.is_directory()) {
                continue;
            }

            const std::string directoryName =
                entry.path().filename().string();

            if (directoryName != "extracted_first_dirty" &&
                directoryName != "report_output") {
                groupPaths.push_back(entry.path());
            }
        }
    }

    std::sort(groupPaths.begin(), groupPaths.end());

    if (groupPaths.empty()) {
        std::cout << "错误：没有 group 文件夹" << std::endl;
        std::cout << "回车返回...";
        std::cin.ignore();
        std::cin.get();
        return;
    }

    int totalPositives = 0;
    int totalNegatives = 0;
    int truePositives = 0;
    int falseNegatives = 0;
    int falsePositives = 0;
    int trueNegatives = 0;

    for (const auto& groupPath : groupPaths) {
        const std::string gtFile =
            groupPath.string() + "/group_gt";

        if (fs::exists(gtFile)) {
            std::ifstream input(gtFile);
            int value = -1;

            if (input >> value) {
                if (value == 1) {
                    ++totalPositives;
                } else if (value == 0) {
                    ++totalNegatives;
                }
            }
        }

        const GroupAnalysisResult result =
            analyzeGroupLevel(groupPath.string(), isArchiveMode);

        switch (result) {
        case GroupAnalysisResult::CORRECT_DETECT:
            ++truePositives;
            break;

        case GroupAnalysisResult::MISS_DETECT:
            ++falseNegatives;
            break;

        case GroupAnalysisResult::FALSE_ALARM:
            ++falsePositives;
            break;

        case GroupAnalysisResult::CORRECT_CLEAN:
            ++trueNegatives;
            break;

        default:
            break;
        }

        std::cout << std::endl;
    }

    const double recallRate =
        totalPositives > 0
            ? static_cast<double>(truePositives) /
                  totalPositives * 100.0
            : 0.0;

    const double falseAlarmRate =
        totalNegatives > 0
            ? static_cast<double>(falsePositives) /
                  totalNegatives * 100.0
            : 0.0;

    std::stringstream summary;

    summary << "===== Dirty Benchmark GROUP-Level SUMMARY =====\n";
    summary << "GT=1(序列后期出现脏污): " << totalPositives << '\n';
    summary << "GT=0(全程干净): " << totalNegatives << "\n\n";
    summary << "TP(检出): " << truePositives << '\n';
    summary << "FN(漏检): " << falseNegatives << '\n';
    summary << "FP(误报): " << falsePositives << '\n';
    summary << "TN(正确不报): " << trueNegatives << "\n\n";
    summary << "组级别召回率(Recall): " << recallRate << "%\n";
    summary << "组级别误报率(FalseAlarm): "
            << falseAlarmRate << "%\n\n";
    summary << "人工复核：首次脏污渲染图在报告目录 "
            << "extracted_first_dirty\n";
    summary << "==============================================\n";

    std::cout << summary.str();

    if (isArchiveMode && !currentReportDir_.empty()) {
        std::error_code ec;

        fs::create_directories(currentReportDir_, ec);

        std::ofstream summaryFile(
            currentReportDir_ + "/summary_group_level.txt");

        if (summaryFile.is_open()) {
            summaryFile << summary.str();
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

        std::cout << "\n>> 报告归档至 "
                  << currentReportDir_ << std::endl;
    }

    std::cout << "\n组级别分析完毕，回车返回菜单...";
    std::cin.ignore();
    std::cin.get();
}

void DirtyBenchmark::batchRunAllGroups()
{
    initializeDirectories();

    std::vector<fs::path> groupPaths;

    if (fs::exists(DIRTY_BENCHMARK_DIR)) {
        for (const auto& entry :
             fs::directory_iterator(DIRTY_BENCHMARK_DIR)) {
            if (!entry.is_directory()) {
                continue;
            }

            const std::string directoryName =
                entry.path().filename().string();

            if (directoryName != "extracted_first_dirty" &&
                directoryName != "report_output") {
                groupPaths.push_back(entry.path());
            }
        }
    }

    std::sort(groupPaths.begin(), groupPaths.end());

    if (groupPaths.empty()) {
        std::cout << "错误：没有 group 子文件夹" << std::endl;
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
        REPORT_ROOT + "/" + testVersion_ + "/" + testTimestamp_;

    currentExtractDir_ =
        currentReportDir_ + "/extracted_first_dirty";

    std::error_code ec;
    fs::create_directories(currentExtractDir_, ec);

    std::cout << "\n测试版本: " << testVersion_ << '\n';
    std::cout << "系统时间戳: " << testTimestamp_ << '\n';
    std::cout << "报告目录: " << currentReportDir_ << '\n';
    std::cout << "脏污渲染图目录: "
              << currentExtractDir_ << "\n\n";

    std::vector<int> groupGroundTruths;
    groupGroundTruths.reserve(groupPaths.size());

    std::cout << "待处理 groups（请先为全部组录入真值）:\n";

    for (size_t index = 0; index < groupPaths.size(); ++index) {
        const auto& groupPath = groupPaths[index];
        const std::string groupName =
            groupPath.filename().string();

        const int imageCount =
            countImagesInDirectory(groupPath.string());

        std::cout << "\n==============================================\n";
        std::cout << "Group [" << index + 1
                  << "/" << groupPaths.size()
                  << "]: " << groupName << '\n';

        std::cout << "图片目录: " << groupPath.string() << '\n';
        std::cout << "图片数量：" << imageCount << '\n';
        std::cout << "真值：0=全程干净；1=序列后期出现脏污\n";

        int groundTruth = -1;

        std::cout << "输入本组真值（0/1）: ";

        while (!(std::cin >> groundTruth) ||
               (groundTruth != 0 && groundTruth != 1)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');

            std::cout << "只能输入 0 或 1！\n";
            std::cout << "重新输入真值（0/1）: ";
        }

        groupGroundTruths.push_back(groundTruth);
    }

    std::cout << "\n==== 真值录入完成，批量推理计划 ====\n";

    for (size_t index = 0; index < groupPaths.size(); ++index) {
        std::cout << " [" << index + 1 << "] "
                  << groupPaths[index].filename().string()
                  << " -> GT=" << groupGroundTruths[index]
                  << '\n';
    }

    std::cout << "确认开始连续批量推理？回车继续，Ctrl-C 退出";
    std::cin.ignore();
    std::cin.get();

    for (size_t index = 0; index < groupPaths.size(); ++index) {
        runSingleGroup(
            groupPaths[index].string(),
            groupGroundTruths[index]);
    }

    std::cout << "==== 全部 group 推理完成，执行归档分析 ====\n"
              << std::endl;

    batchAnalyzeGroups(true);

    testVersion_.clear();
    testTimestamp_.clear();
    currentReportDir_.clear();
    currentExtractDir_.clear();
}

void DirtyBenchmark::runSingleGroupDebug()
{
    std::vector<fs::path> groupPaths;

    if (fs::exists(DIRTY_BENCHMARK_DIR)) {
        for (const auto& entry :
             fs::directory_iterator(DIRTY_BENCHMARK_DIR)) {
            if (!entry.is_directory()) {
                continue;
            }

            const std::string directoryName =
                entry.path().filename().string();

            if (directoryName != "extracted_first_dirty" &&
                directoryName != "report_output") {
                groupPaths.push_back(entry.path());
            }
        }
    }

    std::sort(groupPaths.begin(), groupPaths.end());

    if (groupPaths.empty()) {
        std::cout << "[WARN] 没有可用 group 文件夹！"
                  << std::endl;

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
        REPORT_ROOT + "/" + testVersion_ + "/" +
        testTimestamp_ + "_" + groupName;

    currentExtractDir_ =
        currentReportDir_ + "/extracted_first_dirty";

    std::error_code ec;
    fs::create_directories(currentExtractDir_, ec);

    std::cout << "\n测试版本: " << testVersion_ << '\n';
    std::cout << "系统时间戳: " << testTimestamp_ << '\n';
    std::cout << "报告目录: " << currentReportDir_ << '\n';
    std::cout << "脏污渲染图目录: "
              << currentExtractDir_ << "\n\n";

    int groundTruth = -1;

    std::cout << "输入本组真值（0/1）: ";

    while (!(std::cin >> groundTruth) ||
           (groundTruth != 0 && groundTruth != 1)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');

        std::cout << "只能输入 0 或 1。\n";
        std::cout << "重新输入（0/1）: ";
    }

    runSingleGroup(selectedPath.string(), groundTruth);

    std::cout << "\n==== 单组推理完成，执行组级别分析并归档 ====\n";

    const GroupAnalysisResult analysisResult =
        analyzeGroupLevel(selectedPath.string(), true);

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

    const std::string summaryPath =
        currentReportDir_ + "/single_summary.txt";

    std::ofstream summaryFile(summaryPath);

    if (summaryFile.is_open()) {
        summaryFile << "==== Single-Group Benchmark Summary ====\n";
        summaryFile << "GroupName: " << groupName << '\n';
        summaryFile << "Version: " << testVersion_ << '\n';
        summaryFile << "Timestamp: " << testTimestamp_ << '\n';
        summaryFile << "AnalyseRetCode: "
                    << static_cast<int>(analysisResult) << '\n';
        summaryFile << "========================================\n";
    }

    std::ifstream readSummary(summaryPath);
    std::cout << readSummary.rdbuf();

    testVersion_.clear();
    testTimestamp_.clear();
    currentReportDir_.clear();
    currentExtractDir_.clear();

    std::cout << "\n单组执行与归档完毕，回车返回";
    std::cin.ignore();
    std::cin.get();
}