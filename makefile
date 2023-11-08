ROOT := $(CURDIR)
TARGET_DIR := $(CURDIR)/build
SUB_DIR := $(ROOT)/base $(ROOT)/vkb
CUR_SRC = $(wildcard *.cpp)

INCLUDE_PATH := E:/kits_mingw/include
VULKAN_INCLUDE_PATH := D:/VulkanSDK/1.3.261.1/Include
LIB_PATH := E:/kits_mingw/lib
VULKAN_LIB_PATH := D:/VulkanSDK/1.3.261.1/Lib

export INCLUDE_PATH VULKAN_INCLUDE_PATH LIB_PATH VULKAN_LIB_PATH
export ROOT TARGET_DIR

all: $(TARGET_DIR)/main.o $(TARGET_DIR)/pcss.o $(TARGET_DIR)/prt.o $(TARGET_DIR)/pbr.o $(TARGET_DIR)/application.o sub_dir main.exe

sub_dir:
	mingw32-make -C ./vkb
	mingw32-make -C ./base

$(TARGET_DIR)/main.o: main.cpp
	g++ -g -c main.cpp -I $(INCLUDE_PATH) -I $(VULKAN_INCLUDE_PATH) -o $(TARGET_DIR)/main.o -I $(ROOT)

$(TARGET_DIR)/pcss.o: pcss.cpp
	g++ -g -c pcss.cpp -I $(INCLUDE_PATH) -I $(VULKAN_INCLUDE_PATH) -o $(TARGET_DIR)/pcss.o -I $(ROOT)

$(TARGET_DIR)/pbr.o: pbr.cpp
	g++ -g -c pbr.cpp -I $(INCLUDE_PATH) -I $(VULKAN_INCLUDE_PATH) -o $(TARGET_DIR)/pbr.o -I $(ROOT)

$(TARGET_DIR)/prt.o: prt.cpp
	g++ -g -c prt.cpp -I $(INCLUDE_PATH) -I $(VULKAN_INCLUDE_PATH) -o $(TARGET_DIR)/prt.o -I $(ROOT)

$(TARGET_DIR)/application.o: application.cpp application.h
	g++ -g -c application.cpp -I $(INCLUDE_PATH) -I $(VULKAN_INCLUDE_PATH) -o $(TARGET_DIR)/application.o -I $(ROOT)

$(TARGET_DIR)/ssr.o: ssr.cpp ssr.h
	g++ -g -c ssr.cpp -I $(INCLUDE_PATH) -I $(VULKAN_INCLUDE_PATH) -o $(TARGET_DIR)/ssr.o -I $(ROOT)
	
main.exe: $(TARGET_DIR)/main.o $(TARGET_DIR)/pcss.o $(TARGET_DIR)/pbr.o $(TARGET_DIR)/prt.o $(TARGET_DIR)/ssr.o
	g++ -g $(TARGET_DIR)/*.o -L $(VULKAN_LIB_PATH) -L $(LIB_PATH) -l vulkan-1 -l glfw3dll -l assimp.dll -o main

clean:
	rm $(TARGET_DIR)/*.o
	rm main