all: $(TARGET_DIR)/context.o $(TARGET_DIR)/buffer.o $(TARGET_DIR)/common.o $(TARGET_DIR)/image.o $(TARGET_DIR)/texture.o $(TARGET_DIR)/pipeline.o $(TARGET_DIR)/renderpass.o $(TARGET_DIR)/scene.o $(TARGET_DIR)/swapchain.o $(TARGET_DIR)/command_buffer.o

$(TARGET_DIR)/context.o: context.cpp context.h
	g++ -g -c context.cpp -o $(TARGET_DIR)/context.o -I $(VULKAN_INCLUDE_PATH) -I $(INCLUDE_PATH) -I $(ROOT)

$(TARGET_DIR)/buffer.o: buffer.cpp buffer.h
	g++ -g -c buffer.cpp -o $(TARGET_DIR)/buffer.o -I $(VULKAN_INCLUDE_PATH) -I $(INCLUDE_PATH) -I $(ROOT)

$(TARGET_DIR)/common.o: common.cpp common.h
	g++ -g -c common.cpp -o $(TARGET_DIR)/common.o -I $(VULKAN_INCLUDE_PATH) -I $(INCLUDE_PATH) -I $(ROOT)

$(TARGET_DIR)/image.o: image.cpp image.h
	g++ -g -c image.cpp -o $(TARGET_DIR)/image.o -I $(VULKAN_INCLUDE_PATH) -I $(INCLUDE_PATH) -I $(ROOT)

$(TARGET_DIR)/texture.o: texture.cpp texture.h
	g++ -g -c texture.cpp -o $(TARGET_DIR)/texture.o -I $(VULKAN_INCLUDE_PATH) -I $(INCLUDE_PATH) -I $(ROOT)

$(TARGET_DIR)/pipeline.o: pipeline.cpp pipeline.h
	g++ -g -c pipeline.cpp -o $(TARGET_DIR)/pipeline.o -I $(VULKAN_INCLUDE_PATH) -I $(INCLUDE_PATH) -I $(ROOT)

$(TARGET_DIR)/renderpass.o: renderpass.cpp renderpass.h
	g++ -g -c renderpass.cpp -o $(TARGET_DIR)/renderpass.o -I $(VULKAN_INCLUDE_PATH) -I $(INCLUDE_PATH) -I $(ROOT)

$(TARGET_DIR)/scene.o: scene.cpp scene.h
	g++ -g -c scene.cpp -o $(TARGET_DIR)/scene.o -I $(VULKAN_INCLUDE_PATH) -I $(INCLUDE_PATH) -I $(ROOT)

$(TARGET_DIR)/swapchain.o: swapchain.cpp swapchain.h
	g++ -g -c swapchain.cpp -o $(TARGET_DIR)/swapchain.o -I $(VULKAN_INCLUDE_PATH) -I $(INCLUDE_PATH) -I $(ROOT)

$(TARGET_DIR)/command_buffer.o: command_buffer.cpp command_buffer.h
	g++ -g -c command_buffer.cpp -o $(TARGET_DIR)/command_buffer.o -I $(VULKAN_INCLUDE_PATH) -I $(INCLUDE_PATH) -I $(ROOT)