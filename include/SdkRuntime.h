#ifndef SDK_RUNTIME_H
#define SDK_RUNTIME_H

#include <string>

class SdkRuntime {
public:
    static void setupEnvironment();
    static void killProcessByName(const std::string& processName);
    static void killAllSdkProcesses();
    static void globalCleanup();

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