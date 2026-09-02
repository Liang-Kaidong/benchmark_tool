#include "DirtyBenchmark.h"
#include "SdkRuntime.h"
<<<<<<< HEAD
#include "TimeoutCollector.h"
=======
>>>>>>> 6c47b77 ([20260831_1603] Fix some issues)
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cctype>
#include <iomanip>
<<<<<<< HEAD

=======
>>>>>>> 6c47b77 ([20260831_1603] Fix some issues)
namespace fs = std::filesystem;

const std::string DirtyBenchmark::BENCHMARK_ROOT      = "/userdata/benchmark";
const std::string DirtyBenchmark::DIRTY_BENCHMARK_DIR = BENCHMARK_ROOT + "/dirty_benchmark";
const std::string DirtyBenchmark::REPORT_ROOT          = DIRTY_BENCHMARK_DIR + "/report_output";

DirtyBenchmark::DirtyBenchmark() {}

void DirtyBenchmark::initializeDirectories()
{
    std::vector<std::string> dirsToCreate = {
        DIRTY_BENCHMARK_DIR,
        REPORT_ROOT
    };
    std::error_code ec;
    for (const auto& dirPath : dirsToCreate)
    {
        fs::create_directories(dirPath, ec);
    }
}

int DirtyBenchmark::countImagesInDirectory(const std::string& directoryPath)
{
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        return 0;
    }
    int imageCount = 0;
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            for (auto& ch : ext) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp") {
                imageCount++;
            }
        }
    }
    return imageCount;
}

std::string DirtyBenchmark::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M");
    return ss.str();
}

void DirtyBenchmark::showSubMenu()
{
    bool inSubMenu = true;
    while (inSubMenu) {
        int retSys = std::system("clear");
        (void)retSys;
        std::cout << "\n========================================================" << std::endl;
        std::cout << "              Dirty‑Benchmark 子菜单                    " << std::endl;
        std::cout << "========================================================\n" << std::endl;
        std::cout << " [1] 批量推理+交互式真值+归档报告" << std::endl;
        std::cout << " [2] 仅重新组级别分析（不归档，不拷贝图片）" << std::endl;
        std::cout << " [3] 单组调试推理（带归档报告）" << std::endl;
        std::cout << " [4] 返回上一级" << std::endl;
        std::cout << "\n========================================================" << std::endl;
        std::cout << "请选择 [1‑4]: ";
        int subOption = 0;
        if (!(std::cin >> subOption)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }
        switch (subOption) {
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

bool DirtyBenchmark::runSingleGroup(const std::string& groupPath, int groundTruthValue) {
    std::string groupName = fs::path(groupPath).filename().string();
    std::string groupGtFile = groupPath + "/group_gt";
    std::string groupSaveResultDir = groupPath + "/results";
    std::ofstream gtFileStream(groupGtFile);
    if (gtFileStream.is_open()) {
        gtFileStream << groundTruthValue << "\n";
        gtFileStream.close();
    }
    std::cout << "==============================================" << std::endl;
    std::cout << "开始执行group: [" << groupName << "]" << std::endl;
    std::cout << "本组图片源目录: " << groupPath << std::endl;
    std::cout << "本组序列真值: " << groundTruthValue << std::endl;
    std::cout << "==============================================" << std::endl;
    SdkRuntime::killAllSdkProcesses();
    std::error_code ecClean;
    fs::remove(SdkRuntime::SDK_IMAGES_LINK, ecClean);
    fs::remove_all(SdkRuntime::SDK_REAL_IMAGES_DIR, ecClean);
    fs::remove_all(SdkRuntime::SDK_RESULT_DIR, ecClean);
    if(ecClean)
    {
        std::cerr << "[CLEAN WARN] " << ecClean.message() << std::endl;
    }
    std::cout << ">> 已清理SDK：images软链接、真实images目录、result目录残留\n";
    std::string demoPath = SdkRuntime::SDK_BIN_BASE + SdkRuntime::DEMO_NAME;
    std::string inferPath = SdkRuntime::SDK_BIN_BASE + SdkRuntime::INFER_NAME;
    if (!fs::exists(SdkRuntime::SDK_BIN_BASE)) {
        std::cout << "[ERROR] SDK bin目录不存在 " << SdkRuntime::SDK_BIN_BASE << "，跳过本组" << std::endl;
        return false;
    }
    if (!fs::exists(demoPath) || !fs::exists(inferPath)) {
        std::cout << "[ERROR] SDK二进制缺失，跳过本组" << std::endl;
        return false;
    }
    int imageCount = countImagesInDirectory(groupPath);
    if (imageCount == 0) {
        std::cout << "[WARN] group " << groupName << " 无图片，跳过本组" << std::endl;
        return false;
    }
    std::cout << ">> 本组图片总数量：" << imageCount << " 张\n" << std::endl;
    fs::path currentWorkDir = fs::current_path();
    fs::current_path(SdkRuntime::SDK_BIN_BASE);
    std::cout << ">> 启动 " << SdkRuntime::DEMO_NAME << " --enable-all" << std::endl;
    std::string startDemoCmd = "./" + SdkRuntime::DEMO_NAME + " --enable-all &";
    int retSys = std::system(startDemoCmd.c_str());
    (void)retSys;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::error_code ec;
    fs::create_directory_symlink(groupPath, SdkRuntime::SDK_IMAGES_LINK, ec);
    fs::create_directories(SdkRuntime::SDK_RESULT_GRID, ec);
    std::cout << ">> 执行 " << SdkRuntime::INFER_NAME << " -d -t -s" << std::endl;
    std::string startInferCmd = "./" + SdkRuntime::INFER_NAME + " -d -t -s";
    int inferRet = std::system(startInferCmd.c_str());
    if(inferRet == -1)
    {
        std::cerr << "[WARN] infer进程system调用失败！group=" << groupName << std::endl;
    }
    SdkRuntime::killProcessByName(SdkRuntime::INFER_NAME);
    int outputTxtCount = 0;
    if (fs::exists(SdkRuntime::SDK_RESULT_GRID)) {
        for (const auto& item : fs::directory_iterator(SdkRuntime::SDK_RESULT_GRID)) {
            if (item.path().extension() == ".txt") outputTxtCount++;
        }
    }
    std::cout << ">> 输出txt文件数量: " << outputTxtCount << std::endl;
    if (outputTxtCount != imageCount) {
        std::cout << "!!!警告!!! txt数量与图片数量不一致！" << std::endl;
    }
<<<<<<< HEAD
    
    fs::current_path(currentWorkDir);
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

        const timeout_collector::TimeoutResult timeoutResult =
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
                << " 条，超时(>200ms) "
                << timeoutResult.timeoutSamples.size()
                << " 条" << std::endl;

        if (!timeoutResult.timeoutSamples.empty()) {
            std::cout << ">> 超时图片和对应 result txt 已保存至: "
                    << timeoutOutputDir.string()
                    << std::endl;
        }
    }

=======
    fs::current_path(currentWorkDir);
    fs::remove_all(groupSaveResultDir, ec);
    fs::copy(SdkRuntime::SDK_RESULT_DIR, groupSaveResultDir, fs::copy_options::recursive, ec);
>>>>>>> 6c47b77 ([20260831_1603] Fix some issues)
    SdkRuntime::killAllSdkProcesses();
    fs::remove(SdkRuntime::SDK_IMAGES_LINK, ec);
    std::cout << ">> group " << groupName << " 推理完成\n" << std::endl;
    return true;
}

GroupAnalysisResult DirtyBenchmark::analyzeGroupLevel(const std::string& groupPath, bool isArchiveMode) {
    std::string groupName = fs::path(groupPath).filename().string();
    std::string groupGtFilePath = groupPath + "/group_gt";
    std::string groupResultGridPath = groupPath + "/results/grid";
    std::string groupResultDirtPath = groupPath + "/results/dirt";
    std::string groupStatOutPath = groupPath + "/group_stat.txt";
    std::string firstDirtyImageName = "";
    fs::path firstDirtyTxtPath;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "组级别分析 group: " << groupName << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    if (!fs::exists(groupGtFilePath)) {
        std::cout << "[ERROR] " << groupName << " 缺少group_gt，跳过" << std::endl;
        return GroupAnalysisResult::ERROR_SKIPPED;
    }
    std::ifstream gtFile(groupGtFilePath);
    std::string gtStr;
    gtFile >> gtStr;
    if (gtStr != "0" && gtStr != "1") {
        std::cout << "[ERROR] " << groupName << " group_gt必须0或1" << std::endl;
        return GroupAnalysisResult::ERROR_SKIPPED;
    }
    int groundTruth = std::stoi(gtStr);
    if (!fs::exists(groupResultGridPath)) {
        std::cout << "[ERROR] " << groupName << " 无推理结果目录，跳过" << std::endl;
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
        std::string imageName = "";
        std::string dirtValueStr = "";
        while (std::getline(textStream, line)) {
            size_t imgPos = line.find("[image=");
            if (imgPos != std::string::npos) {
                size_t endPos = line.find("]", imgPos);
                if (endPos != std::string::npos) {
                    imageName = line.substr(imgPos + 7, endPos - (imgPos + 7));
                }
            }
            size_t dirtPos = line.find("tShmAIModelStatus.u8DirtDetect");
            if (dirtPos != std::string::npos) {
                size_t eqPos = line.find("=", dirtPos);
                if (eqPos != std::string::npos) {
                    dirtValueStr = line.substr(eqPos + 1);
                    dirtValueStr.erase(std::remove_if(dirtValueStr.begin(), dirtValueStr.end(),
                        [](char c){ return std::isspace(static_cast<unsigned char>(c)); }),
                        dirtValueStr.end());
                }
            }
        }
        if (dirtValueStr == "1" || dirtValueStr == "2") {
            firstDirtyImageName = imageName;
            firstDirtyTxtPath = txtPath;
            break;
        }
    }
    GroupAnalysisResult analysisResult;
    std::string resultText = "";
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
    if (isArchiveMode && !firstDirtyImageName.empty() && fs::exists(groupResultDirtPath) && !currentExtractDir_.empty()) {
        std::string baseStem = fs::path(firstDirtyImageName).stem().string();
        std::string targetPattern = baseStem + "_dirt.jpg";
        bool foundRenderedImg = false;
        for (const auto& entry : fs::directory_iterator(groupResultDirtPath)) {
            if (entry.path().filename().string().find(targetPattern) != std::string::npos) {
                std::string destination = currentExtractDir_ + "/" + groupName + "_FIRST_DIRTY_" + entry.path().filename().string();
                std::error_code ec;
                fs::copy(entry.path(), destination, fs::copy_options::overwrite_existing, ec);
                std::cout << ">> 提取首次脏污渲染图: " << entry.path().filename().string() << " -> " << currentExtractDir_ << std::endl;
                if (!firstDirtyTxtPath.empty()) {
                    fs::path txtDestination = fs::path(currentExtractDir_) /
                        (groupName + "_FIRST_DIRTY_" + firstDirtyTxtPath.filename().string());
                    fs::copy_file(firstDirtyTxtPath, txtDestination, fs::copy_options::overwrite_existing, ec);
                    std::cout << ">> 提取对应推理txt: " << firstDirtyTxtPath.filename().string()
                              << " -> " << currentExtractDir_ << std::endl;
                }
                foundRenderedImg = true;
                break;
            }
        }
        if (!foundRenderedImg) {
            std::cout << "[WARN] " << groupName << ": results/dirt未找到 " << baseStem << "_dirt.jpg" << std::endl;
        }
    }
    std::ofstream statStream(groupStatOutPath);
    if (statStream.is_open()) {
        statStream << "==== Group‑Level Stat ====\n";
        statStream << "GroupName: " << groupName << "\n";
        statStream << "GroupGT(0=无脏污,1=后期出现脏污): " << groundTruth << "\n";
        statStream << "GroupResult: " << resultText << "\n";
        if (!firstDirtyImageName.empty()) {
            statStream << "FirstDirtyFrame(原始图片): " << firstDirtyImageName << "\n";
        } else {
            statStream << "FirstDirtyFrame: NONE(本组全程未报脏污)\n";
        }
        statStream << "==========================\n";
        statStream.close();
    }
    std::ifstream readBack(groupStatOutPath);
    std::cout << readBack.rdbuf();
    return analysisResult;
}

void DirtyBenchmark::batchAnalyzeGroups(bool isArchiveMode) {
    initializeDirectories();
    std::vector<fs::path> groupPaths;
    if (fs::exists(DIRTY_BENCHMARK_DIR)) {
        for (const auto& entry : fs::directory_iterator(DIRTY_BENCHMARK_DIR)) {
            if (entry.is_directory()) {
                std::string dirName = entry.path().filename().string();
                if (dirName != "extracted_first_dirty" && dirName != "report_output") {
                    groupPaths.push_back(entry.path());
                }
            }
        }
    }
    std::sort(groupPaths.begin(), groupPaths.end());
    if (groupPaths.empty()) {
        std::cout << "错误：没有group文件夹" << std::endl;
        std::cout << "回车返回...";
        std::cin.ignore(); std::cin.get();
        return;
    }
    int totalPositives = 0, totalNegatives = 0;
    int truePositives = 0, falseNegatives = 0, falsePositives = 0, trueNegatives = 0;
    for (const auto& gPath : groupPaths) {
        std::string gtFile = gPath.string() + "/group_gt";
        if (fs::exists(gtFile)) {
            std::ifstream ifs(gtFile);
            int val;
            if (ifs >> val) {
                if (val == 1) totalPositives++;
                else if (val == 0) totalNegatives++;
            }
        }
        GroupAnalysisResult result = analyzeGroupLevel(gPath.string(), isArchiveMode);
        switch (result) {
            case GroupAnalysisResult::CORRECT_DETECT: truePositives++; break;
            case GroupAnalysisResult::MISS_DETECT:    falseNegatives++; break;
            case GroupAnalysisResult::FALSE_ALARM:    falsePositives++; break;
            case GroupAnalysisResult::CORRECT_CLEAN:  trueNegatives++; break;
            default: break;
        }
        std::cout << std::endl;
    }
    double recallRate = (totalPositives > 0) ? (double)truePositives / totalPositives * 100.0 : 0.0;
    double falseAlarmRate = (totalNegatives > 0) ? (double)falsePositives / totalNegatives * 100.0 : 0.0;
    std::stringstream summaryStream;
    summaryStream << "===== Dirty Benchmark GROUP‑LEVEL SUMMARY =====\n";
    summaryStream << "GT=1(序列后期出现脏污): " << totalPositives << "\n";
    summaryStream << "GT=0(全程干净): " << totalNegatives << "\n\n";
    summaryStream << "TP(检出): " << truePositives << "\n";
    summaryStream << "FN(漏检): " << falseNegatives << "\n";
    summaryStream << "FP(误报): " << falsePositives << "\n";
    summaryStream << "TN(正确不报): " << trueNegatives << "\n\n";
    summaryStream << "组级别召回率(Recall): " << recallRate << "%\n";
    summaryStream << "组级别误报率(FalseAlarm): " << falseAlarmRate << "%\n\n";
    summaryStream << "人工复核：首次脏污渲染图在报告目录extracted_first_dirty\n";
    summaryStream << "==============================================\n";
    std::cout << summaryStream.str();
    if (isArchiveMode && !currentReportDir_.empty()) {
        std::error_code ec;
        fs::create_directories(currentReportDir_, ec);
        std::ofstream summaryFile(currentReportDir_ + "/summary_group_level.txt");
        summaryFile << summaryStream.str();
        summaryFile.close();
        for (const auto& gPath : groupPaths) {
            std::string gName = gPath.filename().string();
            std::string destSubDir = currentReportDir_ + "/" + gName;
            fs::create_directories(destSubDir, ec);
            std::string srcStatFile = gPath.string() + "/group_stat.txt";
            if (fs::exists(srcStatFile)) {
                fs::copy(srcStatFile, destSubDir + "/group_stat.txt", fs::copy_options::overwrite_existing, ec);
            }
        }
        std::cout << "\n>> 报告归档至 " << currentReportDir_ << std::endl;
    }
    std::cout << "\n组级别分析完毕，回车返回菜单...";
    std::cin.ignore(); std::cin.get();
}

void DirtyBenchmark::batchRunAllGroups() {
    initializeDirectories();
    std::vector<fs::path> groupPaths;
    if (fs::exists(DIRTY_BENCHMARK_DIR)) {
        for (const auto& entry : fs::directory_iterator(DIRTY_BENCHMARK_DIR)) {
            if (entry.is_directory()) {
                std::string dirName = entry.path().filename().string();
                if (dirName != "extracted_first_dirty" && dirName != "report_output") {
                    groupPaths.push_back(entry.path());
                }
            }
        }
    }
    std::sort(groupPaths.begin(), groupPaths.end());
    if (groupPaths.empty()) {
        std::cout << "错误：没有group子文件夹" << std::endl;
        std::cout << "回车返回...";
        std::cin.ignore(); std::cin.get();
        return;
    }
    std::cout << "\n请输入本次测试版本号(例如 V1.2.0): ";
    std::cin >> testVersion_;
    while (testVersion_.empty()) {
        std::cout << "版本号不能为空！" << std::endl;
        std::cout << "请输入版本号: ";
        std::cin >> testVersion_;
    }
    testTimestamp_ = getCurrentTimestamp();
    currentReportDir_ = REPORT_ROOT + "/" + testVersion_ + "/" + testTimestamp_;
    currentExtractDir_ = currentReportDir_ + "/extracted_first_dirty";
    std::error_code ec;
    fs::create_directories(currentExtractDir_, ec);
    std::cout << "\n测试版本: " << testVersion_ << std::endl;
    std::cout << "系统时间戳: " << testTimestamp_ << std::endl;
    std::cout << "报告目录: " << currentReportDir_ << std::endl;
    std::cout << "脏污渲染图目录: " << currentExtractDir_ << "\n" << std::endl;
    std::vector<int> groupGroundTruths;
    groupGroundTruths.reserve(groupPaths.size());

    std::cout << "待处理groups（请先为全部组录入真值）:" << std::endl;
    for (size_t index = 0; index < groupPaths.size(); ++index) {
        const auto& gPath = groupPaths[index];
        std::string gName = gPath.filename().string();
        int imageCount = countImagesInDirectory(gPath.string());
        std::cout << "\\n==============================================" << std::endl;
        std::cout << "Group [" << (index + 1) << "/" << groupPaths.size() << "]: " << gName << std::endl;
        std::cout << "图片目录: " << gPath.string() << std::endl;
        std::cout << "图片数量：" << imageCount << std::endl;
        std::cout << "真值：0=全程干净；1=序列后期出现脏污" << std::endl;
        int inputGt = -1;
        std::cout << "输入本组真值(0/1): ";
        while (!(std::cin >> inputGt) || (inputGt != 0 && inputGt != 1)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "只能输入0或者1！\\n重新输入真值(0/1): ";
        }
        groupGroundTruths.push_back(inputGt);
    }

    std::cout << "\\n==== 真值录入完成，批量推理计划 ====" << std::endl;
    for (size_t index = 0; index < groupPaths.size(); ++index) {
        std::cout << " [" << (index + 1) << "] "
                  << groupPaths[index].filename().string()
                  << " -> GT=" << groupGroundTruths[index] << std::endl;
    }
    std::cout << "确认开始连续批量推理？回车继续，Ctrl‑C退出" << std::endl;
    std::cin.ignore(); std::cin.get();

    for (size_t index = 0; index < groupPaths.size(); ++index) {
        runSingleGroup(groupPaths[index].string(), groupGroundTruths[index]);
    }
    std::cout << "==== 全部group推理完成，执行归档分析 ====\n" << std::endl;
    batchAnalyzeGroups(true);
    testVersion_.clear();
    testTimestamp_.clear();
    currentReportDir_.clear();
    currentExtractDir_.clear();
}

void DirtyBenchmark::runSingleGroupDebug() {
    std::vector<fs::path> groupPaths;
    if (fs::exists(DIRTY_BENCHMARK_DIR)) {
        for (const auto& entry : fs::directory_iterator(DIRTY_BENCHMARK_DIR)) {
            if (entry.is_directory()) {
                std::string dName = entry.path().filename().string();
                if (dName != "extracted_first_dirty" && dName != "report_output") {
                    groupPaths.push_back(entry.path());
                }
            }
        }
    }
    std::sort(groupPaths.begin(), groupPaths.end());
    if (groupPaths.empty()) {
        std::cout << "[WARN] 没有可用group文件夹！" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return;
    }
    std::cout << "\n=== 可用Group列表 ===" << std::endl;
    for (size_t i = 0; i < groupPaths.size(); ++i) {
        std::cout << " [" << (i + 1) << "] " << groupPaths[i].filename().string() << std::endl;
    }
    std::cout << "\n请输入要调试的group序号: ";
    size_t selIdx = 0;
    if (!(std::cin >> selIdx) || selIdx < 1 || selIdx > groupPaths.size()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "[ERROR] 序号超出范围或者group不存在" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return;
    }
    fs::path selPath = groupPaths[selIdx - 1];
    std::string gName = selPath.filename().string();
    int imgCount = countImagesInDirectory(selPath.string());
    std::cout << "\n调试group: " << gName << "，图片数:" << imgCount << std::endl;
    std::cout << "请输入本次测试版本号(例如 V1.2.0): ";
    std::cin >> testVersion_;
    while (testVersion_.empty()) {
        std::cout << "版本号不能为空！" << std::endl;
        std::cout << "请输入版本号: ";
        std::cin >> testVersion_;
    }
    testTimestamp_ = getCurrentTimestamp();
    currentReportDir_ = REPORT_ROOT + "/" + testVersion_ + "/" + testTimestamp_ + "_" + gName;
    currentExtractDir_ = currentReportDir_ + "/extracted_first_dirty";
    std::error_code ec;
    fs::create_directories(currentExtractDir_, ec);
    std::cout << "\n测试版本: " << testVersion_ << std::endl;
    std::cout << "系统时间戳: " << testTimestamp_ << std::endl;
    std::cout << "报告目录: " << currentReportDir_ << std::endl;
    std::cout << "脏污渲染图目录: " << currentExtractDir_ << "\n" << std::endl;
    int inputGt = -1;
    std::cout << "输入本组真值(0/1): ";
    while (!(std::cin >> inputGt) || (inputGt != 0 && inputGt != 1)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        std::cout << "只能0或1\n重新输入(0/1): ";
    }
    runSingleGroup(selPath.string(), inputGt);
    std::cout << "\n==== 单组推理完成，执行组级别分析并归档 ====" << std::endl;
    GroupAnalysisResult retAna = analyzeGroupLevel(selPath.string(), true);
    fs::create_directories(currentReportDir_, ec);
    std::string srcStatFile = selPath.string() + "/group_stat.txt";
    if (fs::exists(srcStatFile)) {
        fs::copy(srcStatFile, currentReportDir_ + "/group_stat.txt", fs::copy_options::overwrite_existing, ec);
    }
    std::string summaryFilePath = currentReportDir_ + "/single_summary.txt";
    std::ofstream summaryFile(summaryFilePath);
    if (summaryFile.is_open()) {
        summaryFile << "==== Single‑Group Benchmark Summary ====\n";
        summaryFile << "GroupName: " << gName << "\n";
        summaryFile << "Version: " << testVersion_ << "\n";
        summaryFile << "Timestamp: " << testTimestamp_ << "\n";
        summaryFile << "AnalyseRetCode: " << static_cast<int>(retAna) << "\n";
        summaryFile << "========================================\n";
        summaryFile.close();
    }
    std::ifstream readSummary(summaryFilePath);
    std::cout << readSummary.rdbuf();
    testVersion_.clear();
    testTimestamp_.clear();
    currentReportDir_.clear();
    currentExtractDir_.clear();
    std::cout << "\n单组执行&归档完毕，回车返回";
    std::cin.ignore(); std::cin.get();
}
