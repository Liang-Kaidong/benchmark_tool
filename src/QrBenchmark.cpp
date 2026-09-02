#include "QrBenchmark.h"
#include "SdkRuntime.h"
<<<<<<< HEAD
#include "TimeoutCollector.h"
=======
>>>>>>> 6c47b77 ([20260831_1603] Fix some issues)
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cctype>
#include <iomanip>
<<<<<<< HEAD

namespace fs = std::filesystem;

const std::string QrBenchmark::BENCHMARK_ROOT             = "/userdata/benchmark";
const std::string QrBenchmark::CHARGER_QR_BENCHMARK_DIR   = BENCHMARK_ROOT + "/charger_qr";
const std::string QrBenchmark::AREA_QR_BENCHMARK_DIR      = BENCHMARK_ROOT + "/area_qr";

=======
namespace fs = std::filesystem;

const std::string QrBenchmark::BENCHMARK_ROOT             = "/userdata/benchmark";
const std::string QrBenchmark::CHARGER_QR_BENCHMARK_DIR   = BENCHMARK_ROOT + "/charger_qr";
const std::string QrBenchmark::AREA_QR_BENCHMARK_DIR      = BENCHMARK_ROOT + "/area_qr";

>>>>>>> 6c47b77 ([20260831_1603] Fix some issues)
QrBenchmark::QrBenchmark()
{
}

void QrBenchmark::initializeDirectories()
{
    std::vector<std::string> dirsToCreate = {
        CHARGER_QR_BENCHMARK_DIR,
        AREA_QR_BENCHMARK_DIR,
        CHARGER_QR_BENCHMARK_DIR + "/report_output",
        AREA_QR_BENCHMARK_DIR + "/report_output"
    };
    std::error_code ec;
    for(const auto& dirPath : dirsToCreate)
    {
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

std::string QrBenchmark::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M");
    return ss.str();
}

std::string QrBenchmark::getTaskBenchRoot(QrTaskType task)
{
    if(task == QrTaskType::CHARGER_QR)
    {
        return CHARGER_QR_BENCHMARK_DIR;
    }
    else
    {
        return AREA_QR_BENCHMARK_DIR;
    }
}

std::string QrBenchmark::getTaskReportRoot(QrTaskType task)
{
    std::string benchRoot = getTaskBenchRoot(task);
    return benchRoot + "/report_output";
}

void QrBenchmark::showSubMenu()
{
    bool inSubMenu = true;
    while (inSubMenu)
    {
        int retSys = std::system("clear");
        (void)retSys;
        std::cout << "\n========================================================" << std::endl;
        std::cout << "               QR Benchmark子菜单                       " << std::endl;
        std::cout << "========================================================" << std::endl;
        std::cout << "\n  [1] Charger‑QR 基准测试" << std::endl;
        std::cout << "  [2] Area‑QR 基准测试" << std::endl;
        std::cout << "  [3] 返回上一级菜单" << std::endl;
        std::cout << "\n========================================================\n" << std::endl;
        std::cout << "请选择子选项 [1-3]: ";
        int option = 0;
        if (!(std::cin >> option))
        {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }
        switch (option)
        {
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
    std::cout << "\n----------------------------------------" << std::endl;
    std::cout << " [1] 批量跑全部group" << std::endl;
    std::cout << " [2] 单组调试(选择序号执行)" << std::endl;
    std::cout << "请选择模式 [1/2]: ";
    int mode = 0;
    if(!(std::cin >> mode))
    {
        std::cin.clear();
        std::cin.ignore(10000,'\n');
        return;
    }
    if(mode == 1)
    {
        batchRunAllGroups(task);
    }
    else if(mode == 2)
    {
        runSingleGroupDebug(task);
    }
}

bool QrBenchmark::runSingleGroup(const std::string& groupPath)
{
    std::string groupName = fs::path(groupPath).filename().string();
    std::string groupSaveResultDir = groupPath + "/results";
    std::cout << "==============================================" << std::endl;
    std::cout << "开始执行group: [" << groupName << "]" << std::endl;
    std::cout << "本组图片源目录: " << groupPath << std::endl;
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
    fs::current_path(currentWorkDir);
<<<<<<< HEAD

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
    fs::remove_all(groupSaveResultDir, ec);
    fs::copy(SdkRuntime::SDK_RESULT_DIR, groupSaveResultDir, fs::copy_options::recursive, ec);
>>>>>>> 6c47b77 ([20260831_1603] Fix some issues)
    SdkRuntime::killAllSdkProcesses();
    fs::remove(SdkRuntime::SDK_IMAGES_LINK, ec);
    std::cout << ">> group " << groupName << " 推理完成\n" << std::endl;
    return true;
}

std::vector<QrSampleResult> QrBenchmark::parseGroupGridLog(const std::string& groupGridDir, QrTaskType task)
{
    std::vector<QrSampleResult> outResults;
    if(!fs::exists(groupGridDir)) return outResults;
    for(const auto& entry : fs::directory_iterator(groupGridDir))
    {
        if(!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        for(auto& ch : ext) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if(ext != ".txt") continue;
        std::string logFilePath = entry.path().string();
        std::ifstream fin(logFilePath);
        if(!fin.is_open()) continue;
        std::string line;
        std::string currentImageName;
        int qrFieldVal = -1;
        while(std::getline(fin, line))
        {
            std::string imgName;
            if(extractImageName(line, imgName))
            {
                if(!currentImageName.empty() && qrFieldVal != -1)
                {
                    QrSampleResult res;
                    res.imageName = currentImageName;
                    res.logFileName = entry.path().filename().string();
                    res.isSuccess = (qrFieldVal == 1 || qrFieldVal == 2);
                    outResults.push_back(res);
                }
                currentImageName = imgName;
                qrFieldVal = -1;
            }
            int val;
            if(task == QrTaskType::CHARGER_QR)
            {
                if(parseChargerQrLine(line, val)) qrFieldVal = val;
            }
            else
            {
                if(parseAreaQrLine(line, val)) qrFieldVal = val;
            }
        }
        if(!currentImageName.empty() && qrFieldVal != -1)
        {
            QrSampleResult res;
            res.imageName = currentImageName;
            res.logFileName = entry.path().filename().string();
            res.isSuccess = (qrFieldVal == 1 || qrFieldVal == 2);
            outResults.push_back(res);
        }
        fin.close();
    }
    return outResults;
}

QrGroupAnalysisResult QrBenchmark::analyzeGroupLevel(const std::string& groupPath, QrTaskType task, bool isArchiveMode)
{
    std::string groupName = fs::path(groupPath).filename().string();
    std::string groupResultGridPath = groupPath + "/results/grid";
    std::string groupStatOutPath = groupPath + "/group_stat.txt";
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "组级别分析 group: " << groupName << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    if (!fs::exists(groupResultGridPath)) {
        std::cout << "[ERROR] " << groupName << " 无推理结果目录，跳过" << std::endl;
        return QrGroupAnalysisResult::ERROR_SKIPPED;
    }
    auto sampleList = parseGroupGridLog(groupResultGridPath, task);
    size_t total = sampleList.size();
    size_t successCnt = 0;
    for(auto &s : sampleList)
    {
        if(s.isSuccess) successCnt++;
    }
    size_t failCnt = total - successCnt;
    double decodeRate = total>0 ? static_cast<double>(successCnt)/total*100.0 : 0.0;
    if(isArchiveMode && !currentFailExtractDir_.empty())
    {
        std::error_code ec;
        for(const auto& item : sampleList)
        {
            if(!item.isSuccess)
            {
                fs::path srcImg = fs::path(groupPath) / item.imageName;
                fs::path dstImg = fs::path(currentFailExtractDir_) / (groupName + "_" + item.imageName);
                if(fs::exists(srcImg, ec))
                {
                    fs::copy_file(srcImg, dstImg, fs::copy_options::overwrite_existing, ec);
                    fs::path srcTxt = fs::path(groupResultGridPath) / item.logFileName;
                    fs::path dstTxt = fs::path(currentFailExtractDir_) / (groupName + "_" + item.logFileName);
                    if (fs::exists(srcTxt, ec))
                    {
                        fs::copy_file(srcTxt, dstTxt, fs::copy_options::overwrite_existing, ec);
                    }
                    else
                    {
                        std::cout << "[WARN] 未找到失败图片对应的推理txt: " << srcTxt.string() << std::endl;
                    }
                }
            }
        }
    }
    std::ofstream statStream(groupStatOutPath);
    if (statStream.is_open()) {
        statStream << "==== QR Group‑Level Stat ====\n";
        statStream << "GroupName: " << groupName << "\n";
        statStream << "TotalSamples: " << total << "\n";
        statStream << "SuccessCount: " << successCnt << "\n";
        statStream << "FailCount: " << failCnt << "\n";
        statStream << std::fixed << std::setprecision(2);
        statStream << "DecodeRatePercent: " << decodeRate << "\n";
        statStream << "=============================\n";
        statStream.close();
    }
    std::ifstream readBack(groupStatOutPath);
    std::cout << readBack.rdbuf();
    return QrGroupAnalysisResult::GROUP_OK;
}

void QrBenchmark::batchRunAllGroups(QrTaskType task)
{
    initializeDirectories();
    std::string taskBenchRoot = getTaskBenchRoot(task);
    std::string taskReportRoot = getTaskReportRoot(task);
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
        std::cout << "错误：" << taskBenchRoot << " 下没有group文件夹" << std::endl;
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
    currentReportDir_ = taskReportRoot + "/" + testVersion_ + "/" + testTimestamp_;
    currentFailExtractDir_ = currentReportDir_ + "/failed_images";
    std::error_code ec;
    fs::create_directories(currentFailExtractDir_, ec);
    std::cout << "\n测试版本: " << testVersion_ << std::endl;
    std::cout << "系统时间戳: " << testTimestamp_ << std::endl;
    std::cout << "报告目录: " << currentReportDir_ << std::endl;
    std::cout << "失败图片归档目录: " << currentFailExtractDir_ << "\n" << std::endl;
    std::cout << "待处理groups:" << std::endl;
    for (const auto& p : groupPaths) {
        std::cout << p.string() << std::endl;
    }
    std::cout << "\n确认开始批量推理？回车继续，Ctrl‑C退出" << std::endl;
    std::cin.ignore(); std::cin.get();
    std::vector<QrSampleResult> allGlobalSamples;
    for (const auto& gPath : groupPaths) {
        std::string gName = gPath.filename().string();
        std::cout << "\n==============================================" << std::endl;
        std::cout << "准备执行group: [" << gName << "]" << std::endl;
        std::cout << "图片目录: " << gPath.string() << std::endl;
        runSingleGroup(gPath.string());
        analyzeGroupLevel(gPath.string(), task, true);
        auto groupSamples = parseGroupGridLog(gPath.string()+"/results/grid", task);
        allGlobalSamples.insert(allGlobalSamples.end(), groupSamples.begin(), groupSamples.end());
    }
    size_t totalAll = allGlobalSamples.size();
    size_t successAll = 0;
    for(auto &s : allGlobalSamples) if(s.isSuccess) successAll++;
    size_t failAll = totalAll - successAll;
    double decodeRateAll = totalAll>0 ? static_cast<double>(successAll)/totalAll*100.0 : 0.0;
    std::string summaryPath = currentReportDir_ + "/summary.txt";
    std::ofstream fsum(summaryPath);
    if(fsum.is_open())
    {
        fsum << "SDK_VERSION=" << testVersion_ << "\n";
        fsum << "TIMESTAMP=" << testTimestamp_ << "\n";
        fsum << "TASK=" << ((task == QrTaskType::CHARGER_QR) ? "CHARGER_QR" : "AREA_QR") << "\n";
        fsum << "TOTAL_SAMPLES=" << totalAll << "\n";
        fsum << "SUCCESS_COUNT=" << successAll << "\n";
        fsum << "FAIL_COUNT=" << failAll << "\n";
        fsum << std::fixed << std::setprecision(2);
        fsum << "DECODE_RATE_PERCENT=" << decodeRateAll << "\n";
        fsum.close();
    }
    std::string detailPath = currentReportDir_ + "/detail.txt";
    std::ofstream fdet(detailPath);
    if(fdet.is_open())
    {
        fdet << "# image_name status log_file\n";
        for(const auto& item : allGlobalSamples)
        {
            std::string status = item.isSuccess ? "success" : "failed";
            fdet << item.imageName << " " << status << " " << item.logFileName << "\n";
        }
        fdet.close();
    }
    for (const auto& gPath : groupPaths) {
        std::string gName = gPath.filename().string();
        std::string destSubDir = currentReportDir_ + "/" + gName;
        fs::create_directories(destSubDir, ec);
        std::string srcStatFile = gPath.string() + "/group_stat.txt";
        if (fs::exists(srcStatFile)) {
            fs::copy(srcStatFile, destSubDir + "/group_stat.txt", fs::copy_options::overwrite_existing, ec);
        }
    }
    std::cout << "\n==== QR全部group推理&分析完成 ====\n";
    std::cout << "全局总样本:" << totalAll << " 成功:" << successAll << " 失败:" << failAll;
    std::cout << " 解码率:" << std::fixed << std::setprecision(2) << decodeRateAll << " %\n";
    std::cout << "报告归档至：" << currentReportDir_ << std::endl;
    testVersion_.clear();
    testTimestamp_.clear();
    currentReportDir_.clear();
    currentFailExtractDir_.clear();
    std::cout << "\n回车返回菜单...";
    std::cin.ignore(); std::cin.get();
}

void QrBenchmark::runSingleGroupDebug(QrTaskType task)
{
    std::string taskBenchRoot = getTaskBenchRoot(task);
    std::string taskReportRoot = getTaskReportRoot(task);
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
        std::cout << "[WARN] " << taskBenchRoot << " 没有可用group文件夹！" << std::endl;
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
    currentReportDir_ = taskReportRoot + "/" + testVersion_ + "/" + testTimestamp_ + "_" + gName;
    currentFailExtractDir_ = currentReportDir_ + "/failed_images";
    std::error_code ec;
    fs::create_directories(currentFailExtractDir_, ec);
    std::cout << "\n测试版本: " << testVersion_ << std::endl;
    std::cout << "系统时间戳: " << testTimestamp_ << std::endl;
    std::cout << "报告目录: " << currentReportDir_ << std::endl;
    std::cout << "失败图片目录: " << currentFailExtractDir_ << "\n" << std::endl;
    runSingleGroup(selPath.string());
    std::cout << "\n==== 单组推理完成，执行组级别分析并归档 ====" << std::endl;
    QrGroupAnalysisResult retAna = analyzeGroupLevel(selPath.string(), task, true);
    (void)retAna;
    fs::create_directories(currentReportDir_, ec);
    std::string srcStatFile = selPath.string() + "/group_stat.txt";
    if (fs::exists(srcStatFile)) {
        fs::copy(srcStatFile, currentReportDir_ + "/group_stat.txt", fs::copy_options::overwrite_existing, ec);
    }
    auto groupSamples = parseGroupGridLog(selPath.string()+"/results/grid", task);
    size_t total = groupSamples.size();
    size_t success = 0;
    for(auto &s:groupSamples) if(s.isSuccess) success++;
    size_t fail = total - success;
    double rate = total>0 ? static_cast<double>(success)/total*100.0 :0.0;
    std::string summaryFilePath = currentReportDir_ + "/single_summary.txt";
    std::ofstream summaryFile(summaryFilePath);
    if (summaryFile.is_open()) {
        summaryFile << "==== Single‑Group QR Benchmark Summary ====\n";
        summaryFile << "GroupName: " << gName << "\n";
        summaryFile << "Version: " << testVersion_ << "\n";
        summaryFile << "Timestamp: " << testTimestamp_ << "\n";
        summaryFile << "Task: " << ((task == QrTaskType::CHARGER_QR) ? "CHARGER_QR":"AREA_QR") << "\n";
        summaryFile << "Total:" << total << " Success:" << success << " Fail:" << fail << "\n";
        summaryFile << std::fixed << std::setprecision(2);
        summaryFile << "DecodeRate:" << rate << "%\n";
        summaryFile << "========================================\n";
        summaryFile.close();
    }
    std::ifstream readSummary(summaryFilePath);
    std::cout << readSummary.rdbuf();
    testVersion_.clear();
    testTimestamp_.clear();
    currentReportDir_.clear();
    currentFailExtractDir_.clear();
    std::cout << "\n单组执行&归档完毕，回车返回";
    std::cin.ignore(); std::cin.get();
}

bool QrBenchmark::extractImageName(const std::string& line, std::string& outImageName)
{
    size_t pos = line.find("[image=");
    if(pos == std::string::npos) return false;
    size_t endPos = line.find("]", pos);
    if(endPos == std::string::npos) return false;
    outImageName = line.substr(pos + 7, endPos - (pos + 7));
    return !outImageName.empty();
}

bool QrBenchmark::parseChargerQrLine(const std::string& line, int& outVal)
{
    const std::string key = "tShmChargeQrDetect.u8QrCodeDetect=";
    size_t pos = line.find(key);
    if(pos == std::string::npos) return false;
    std::string numStr = line.substr(pos + key.size());
    numStr.erase(std::remove_if(numStr.begin(), numStr.end(),
        [](char c){ return std::isspace(static_cast<unsigned char>(c)); }), numStr.end());
    int v = std::stoi(numStr);
    outVal = v;
    return true;
}

bool QrBenchmark::parseAreaQrLine(const std::string& line, int& outVal)
{
    const std::string key = "tShmAreaQrDetect.s8AreaId=";
    size_t pos = line.find(key);
    if(pos == std::string::npos) return false;
    std::string numStr = line.substr(pos + key.size());
    numStr.erase(std::remove_if(numStr.begin(), numStr.end(),
        [](char c){ return std::isspace(static_cast<unsigned char>(c)); }), numStr.end());
    int v = std::stoi(numStr);
    outVal = v;
    return true;
}
