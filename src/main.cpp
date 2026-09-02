#include "BenchmarkApp.h"
#include "SdkRuntime.h"
#include <csignal>
#include <cstdlib>

void signalHandler(int signum) {
    (void)signum;
    SdkRuntime::globalCleanup();
    std::exit(1);
}

int main() {
    std::signal(SIGINT, signalHandler);
    BenchmarkApp app;
    app.run();
    return 0;
}
