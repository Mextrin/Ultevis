BUILD_DIR := build

configure:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

build:
	cmake --build $(BUILD_DIR) --config Release

run:
	./$(BUILD_DIR)/Airchestra_artefacts/Release/Airchestra.o

all:
	cmake --build $(BUILD_DIR) --config Release