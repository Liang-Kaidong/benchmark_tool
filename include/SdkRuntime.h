#ifndef SDK_RUNTIME_H
#define SDK_RUNTIME_H
#include <string>

/**
 * SDK生命周期：环境变量、进程启停、全局清理
 * 只处理SDK进程与运行环境，不包含UI、文件统计、时间戳等无关逻辑
 */
class SdkRuntime
{
public:
    static void setupEnvironment();
    static void killProcessByName(const std::string& processName);
    static void killAllSdkProcesses();
    static void globalCleanup();

    // SDK相关常量，仅SDK运行时使用
    static const std::string SDK_BIN_BASE;
    static const std::string SDK_LIB_PATH;
    static const std::string DEMO_NAME;
    static const std::string INFER_NAME;
    static const std::string SDK_RESULT_DIR;
    static const std::string SDK_RESULT_GRID;
    static const std::string SDK_IMAGES_LINK;
    static const std::string SDK_REAL_IMAGES_DIR;

private:
    SdkRuntime() = default;
};

#endif // SDK_RUNTIME_H
