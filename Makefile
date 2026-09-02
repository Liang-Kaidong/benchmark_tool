# ============================================================
# benchmark_tool Makefile
# GCC 11 + C++17 + Fully Static Link (-static)
# ============================================================

CXX := g++

SRC_DIR := src
BUILD_DIR := build
TARGET := $(BUILD_DIR)/benchmark_tool

LOG_SRC_FILE := $(CURDIR)/log/update.log
GEN_LOG_H := $(BUILD_DIR)/generated_update_log.h

CXXFLAGS := -std=c++17 \
	-Wall \
	-Wextra \
	-O2 \
	-pthread \
	-Iinclude \
	-I$(BUILD_DIR) \
	-MMD \
	-MP

LDFLAGS := -static \
	-pthread

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean rebuild info

all: $(TARGET)

$(BUILD_DIR):
	@mkdir -p $@

# 将 log/update.log 嵌入编译期生成的头文件。
# 使用带名称的 Raw String delimiter，避免日志中的普通括号造成字符串结束。
$(GEN_LOG_H): $(LOG_SRC_FILE) | $(BUILD_DIR)
	@echo "[GEN] $@ <- $<"
	@printf '%s\n' '#pragma once' > $@
	@printf '%s\n' 'constexpr const char* g_embedded_update_log = R"UPDATE_LOG(' >> $@
	@cat $< >> $@
	@printf '%s\n' ')UPDATE_LOG";' >> $@

# 每个 cpp 依赖生成头，确保 BenchmarkApp.cpp 能 include 它。
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(GEN_LOG_H) | $(BUILD_DIR)
	@echo "[CXX] $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS) | $(BUILD_DIR)
	@echo ""
	@echo "=============================================="
	@echo " 正在进行全静态链接..."
	@echo "=============================================="
	$(CXX) $(OBJS) $(LDFLAGS) -o $@
	@echo ""
	@echo "=============================================="
	@echo " 编译完成！"
	@echo " 可执行文件 : $(TARGET)"
	@echo " 编译器     : $(CXX)"
	@echo " C++ 标准   : C++17"
	@echo " 优化等级   : O2"
	@echo " 链接方式   : 完全静态 (-static)"
	@echo "=============================================="
	@echo ""

-include $(DEPS)

clean:
	@echo "=============================================="
	@echo " 清理 build 目录（含编译期生成头）"
	@echo "=============================================="
	rm -rf $(BUILD_DIR)
	@echo "清理完成。"

rebuild: clean all

info:
	@echo "=============================================="
	@echo " Benchmark Tool Build Information"
	@echo "=============================================="
	@echo " Compiler     : $(CXX)"
	@echo " C++ Standard : C++17"
	@echo " Optimization : O2"
	@echo " Link Type    : Fully Static (-static)"
	@echo " Source Dir   : $(SRC_DIR)"
	@echo " Build Dir    : $(BUILD_DIR)"
	@echo " Log source   : $(LOG_SRC_FILE)"
	@echo " Generated h  : $(GEN_LOG_H)"
	@echo "=============================================="