# OSALの切り替え
ifeq ($(OSAL),WIN)
	OBJS += osal_windows.o
else
	OBJS += osal_posix.o 
	LDFLAGS += -pthread
endif

# プログラム名とオブジェクトファイル名
TARGET = easyFLAC.dll
OBJS += common_callback.o easyFLAC.o
DEF = exports.def

# 定義済マクロの再定義
CXXFLAGS += -g -Wall -I. -Ofast -std=gnu++11
LDFLAGS += -Llib -lFLAC_dynamic -Wl,--enable-stdcall-fixup,--kill-at

# サフィックスルール適用対象の拡張子の定義
.SUFFIXES: .cpp .o

# プライマリターゲット
.PHONY: all
all: $(TARGET)

# プログラムの生成ルール
$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $^ -shared $(DEF)
	strip $(TARGET)
	ls -al $(TARGET)

# サフィックスルール
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

# ファイル削除用ターゲット
.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS)

# 実行
.PHONY: run
run:
	./$(TARGET)
