all: $(TARGET_DIR)/glfw_context.o $(TARGET_DIR)/timer.o

$(TARGET_DIR)/glfw_context.o: glfw_context.cpp glfw_context.h
	g++ -g -c glfw_context.cpp -I $(INCLUDE_PATH) -o $(TARGET_DIR)/glfw_context.o

$(TARGET_DIR)/timer.o:
	g++ -g -c timer.cpp -o $(TARGET_DIR)/timer.o