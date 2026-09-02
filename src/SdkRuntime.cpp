#include "SdkRuntime.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

const std::string SdkRuntime::SDK_BIN_BASE =
    "/userdata/slot_b/mower_RGB_sdk_iceoryx/bin/";

const std::string SdkRuntime::SDK_LIB_PATH =
    "/userdata/slot_b/mower_RGB_sdk_iceoryx/lib_dl";

const std::string SdkRuntime::DEMO_NAME = "BoundaryBarrierDemo";
const std::string SdkRuntime::INFER_NAME = "InferenceEngine";
const std::string SdkRuntime::SDK_RESULT_DIR = SDK_BIN_BASE + "results";
const std::string SdkRuntime::SDK_RESULT_GRID = SDK_RESULT_DIR + "/grid";
const std::string SdkRuntime::SDK_IMAGES_LINK = SDK_BIN_BASE + "images";
const std::string SdkRuntime::SDK_REAL_IMAGES_DIR = SDK_BIN_BASE + "images";

void SdkRuntime::setupEnvironment()
{
    const char* existingPath = std::getenv("LD_LIBRARY_PATH");
    std::string newPath = SDK_LIB_PATH;

    if (existingPath != nullptr && std::string(existingPath).length() > 0) {
        newPath += ":" + std::string(existingPath);
    }

    setenv("LD_LIBRARY_PATH", newPath.c_str(), 1);
}

void SdkRuntime::killProcessByName(const std::string& processName)
{
    const std::string killCommand =
        "pgrep -f \"" + processName +
        "\" >/dev/null 2>&1 && pkill -f \"" + processName +
        "\" >/dev/null 2>&1";

    const int ret = std::system(killCommand.c_str());
    (void)ret;
}

void SdkRuntime::killAllSdkProcesses()
{
    killProcessByName(DEMO_NAME);
    killProcessByName(INFER_NAME);
}

void SdkRuntime::globalCleanup()
{
    std::cout << "\n>>> 捕获中断，执行清理" << std::endl;

    killAllSdkProcesses();

    std::error_code ec;
    fs::remove(SDK_IMAGES_LINK, ec);
    fs::remove_all(SDK_REAL_IMAGES_DIR, ec);
    fs::remove_all(SDK_RESULT_DIR, ec);

    std::cout << ">>> 清理完成" << std::endl;
}