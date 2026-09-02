#include "BenchmarkApp.h"

#include "SdkRuntime.h"
#include "generated_update_log.h"

#include <chrono>
#include <iostream>
#include <thread>

BenchmarkApp::BenchmarkApp() = default;

void BenchmarkApp::run()
{
    SdkRuntime::setupEnvironment();
    showMainMenu();
}

void BenchmarkApp::showMainMenu()
{
    bool isRunning = true;

    while (isRunning) {
        clearConsole();

        std::cout << "\n========================================================\n";
        std::cout << "            Main Benchmark Tool                         \n";
        std::cout << "========================================================\n\n";
        std::cout << " [1] QR-Benchmark\n";
        std::cout << " [2] Dirty-Benchmark 脏污批量测试\n";
        std::cout << " [3] 查看更新日志\n";
        std::cout << " [4] 退出程序\n";
        std::cout << "\n========================================================\n";
        std::cout << "请选择操作 [1-4]: ";

        int mainChoice = 0;
        if (!(std::cin >> mainChoice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        switch (mainChoice) {
        case 1:
            qrBenchmarkModule_.showSubMenu();
            break;

        case 2:
            dirtyBenchmarkModule_.showSubMenu();
            break;

        case 3:
            showEmbeddedUpdateLog();
            break;

        case 4:
            SdkRuntime::globalCleanup();
            std::cout << "程序退出" << std::endl;
            isRunning = false;
            break;

        default:
            std::cout << "无效选项，请输入 1-4" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            break;
        }
    }
}

void BenchmarkApp::clearConsole()
{
    const int ret = std::system("clear");
    (void)ret;
}

void BenchmarkApp::showEmbeddedUpdateLog()
{
    std::cout << '\n' << g_embedded_update_log << '\n';
    std::cout << "\n回车返回菜单...";

    std::cin.ignore();
    std::cin.get();
}