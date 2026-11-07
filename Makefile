.DEFAULT_GOAL := demo

.PHONY: demo build test setup web-build server dashboard

CPP_BUILD := build/cpp

$(CPP_BUILD)/CMakeCache.txt: cpp/CMakeLists.txt
	cmake -S cpp -B $(CPP_BUILD) -DCMAKE_BUILD_TYPE=Debug

build: $(CPP_BUILD)/CMakeCache.txt
	cmake --build $(CPP_BUILD)

demo: build
	./$(CPP_BUILD)/statetap_example

test: build
	ctest --test-dir $(CPP_BUILD) --output-on-failure

setup:
	npm install

web-build:
	npm run build

server:
	npm run dev:server

dashboard:
	npm run dev:dashboard

