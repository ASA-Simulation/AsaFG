.PHONY: clean configure build install help

.DEFAULT_GOAL := help


# Custom variables
PWD := $(shell pwd)


configure: ## Configure the project for building.
	mkdir -p ./build/
	conan install ./ --build=missing --settings=build_type=Debug
	meson setup --reconfigure \
		--backend ninja \
		--buildtype debug \
		--prefix=$(PWD)/../dist \
		--libdir=$(PWD)/../dist/lib \
		-Dpkg_config_path=$(PWD)/../dist/lib/pkgconfig:$(PWD)/build \
		./build/ .


build: ## Build all targets in the project.
	meson compile -C ./build


install: ## Install all targets in the project.
	meson install -C ./build


package: ## Package the project using conan.
	conan create ./ --build=missing --settings=build_type=Debug


clean: ## Clean all generated build files in the project.
	rm -rf ./build/
	rm -rf ./subprojects/packagecache/

fgms-start: ## Start FGMS
	docker run --rm -p5000:5000/udp -p5001:5001 flightgear/fgms:0.13.4

fgms-watch: ## Watch FGMS (telnet port 5001)
	watch -n 1 telnet 127.0.0.1 5001


help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
