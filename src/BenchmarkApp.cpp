#include "BenchmarkApp.h"
#include "SdkRuntime.h"
#include "generated_update_log.h"
#include <iostream>
#include <thread>
#include <chrono>

BenchmarkApp::BenchmarkApp() {}

void BenchmarkApp::run() {
    SdkRuntime::setupEnvironment();
    // 各模块自己负责初始化自己的目录，不在App层全局初始化
    showMainMenu();
}

void BenchmarkApp::showMainMenu() {
    bool isRunning = true;
    while (isRunning) {
        clearConsole();
        std::cout << "\n========================================================" << std::endl;
        std::cout << "            Main Benchmark Tool                         " << std::endl;
        std::cout << "========================================================\n" << std::endl;
        std::cout << " [1] QR‑Benchmark" << std::endl;
        std::cout << " [2] Dirty‑Benchmark 脏污批量测试" << std::endl;
        std::cout << " [3] 查看更新日志" << std::endl;
        std::cout << " [4] 退出程序" << std::endl;
        std::cout << "\n========================================================" << std::endl;
        std::cout << "请选择操作 [1‑4]: ";
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
                std::cout << "无效选项，请输入1‑4" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                break;
        }
    }
}

void BenchmarkApp::clearConsole()
{
    int ret = std::system("clear");
    (void)ret;
}

void BenchmarkApp::showEmbeddedUpdateLog()
{
    std::cout << "\n" << g_embedded_update_log << "\n";
    std::cout << "\n回车返回菜单...";
    std::cin.ignore();
    std::cin.get();
}
