BUILD_DIR := build

configure:
	cmake -B $(BUILD_DIR)

build:
	cmake --build $(BUILD_DIR)

run:
	./$(BUILD_DIR)/Airchestra_artefacts/Debug/Airchestra.o

all:
	cmake --build $(BUILD_DIR)