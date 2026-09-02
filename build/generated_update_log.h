#pragma once
constexpr const char* g_embedded_update_log = R"UPDATE_LOG(
============================================================
Update Log  benchmark_tool
============================================================
[2026-09-02]
1. 新增单帧推理超时统计与归档：
   - 解析 results/grid/*.txt 中的时间日志。
   - 每条包含 [image=...] 且 total=...ms 的记录均参与统计。
   - 判定规则：total > 200ms 记为超时；total == 200ms 不记为超时。
   - 每个 group 运行后输出总时间记录数和超时数量。

2. 新增 TimeoutCollector.h 公共超时收集模块：
   - QR Benchmark 与 Dirty Benchmark 共用同一套超时解析和归档逻辑。
   - 避免两套测试逻辑重复维护。

3. 超时样本自动保存：
   - 保存原始超时图片。
   - 保存该图片对应的 result txt。
   - 生成超时明细汇总 txt。
   - 保存路径：
     <group>/results/timeout_samples/

4. 已接入测试项：
   - Charger-QR 基准测试。
   - Area-QR 基准测试。
   - Dirty-Benchmark 脏污批量测试。
   - 单组调试与批量测试均生效。

5. 代码格式统一：
   - 统一 include 排列、缩进、花括号、空行和长函数换行风格。

[2026‑08‑31]
1. QR基准测试目录结构重构：
   - 废弃旧 /userdata/benchmark/qr_benchmark
   - 拆分为独立两套：charger_qr / area_qr
   - 各自拥有独立 report_output，报告路径：
     /userdata/benchmark/charger_qr/report_output/<ver>/<timestamp>
     /userdata/benchmark/area_qr/report_output/<ver>/<timestamp>
   - 失败图片归档移入每次报告目录下failed_images，不再使用全局failed_images目录。
   - ProcessUtils::initializeDirectories 更新初始化目录列表。

[2026‑08‑28]
1. 修复 std::system warn_unused_result 编译警告；
   问题：GCC __attribute__((warn_unused_result)) 下 (void)std::system() 压制警告无效；
   方案：接收返回值到局部int变量，再(void)var消除警告。
   修改文件：ProcessUtils.cpp、DirtyBenchmark.cpp，全部system调用统一改造。

2. 修复 isspace 未强转 unsigned char 引发未定义行为UB；
   脏污解析文本时，std::isspace传入char直接调用存在负字符UB；
   使用 static_cast<unsigned char>(c) 做安全转换。

3. Bugfix：每次runSingleGroup推理前强制删除SDK bin下images符号链接、result目录；
   解决多次测试残留旧图片、旧推理结果干扰本组benchmark统计。

4. 主界面新增【查看更新日志】选项，读取源码目录log/update.log；
   使用Makefile注入绝对路径宏，无需将log文件夹拷贝至build输出目录。

5. 保持原有全部业务逻辑、变量名、流程不改动。
============================================================
)UPDATE_LOG";
