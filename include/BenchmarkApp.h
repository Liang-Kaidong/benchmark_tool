#ifndef BENCHMARK_APP_H
#define BENCHMARK_APP_H
#include "DirtyBenchmark.h"
#include "QrBenchmark.h"

class BenchmarkApp {
public:
    BenchmarkApp();
    void run();
private:
    void showMainMenu();

    // UI层私有工具，不再全局暴露
    void clearConsole();
    void showEmbeddedUpdateLog();

    DirtyBenchmark dirtyBenchmarkModule_;
    QrBenchmark    qrBenchmarkModule_;
};

#endif // BENCHMARK_APP_H
