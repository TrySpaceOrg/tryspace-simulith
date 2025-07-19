# Makefile for TrySpace Simulith development
.PHONY: all build clean debug runtime start stop test

export BUILDDIR ?= $(CURDIR)/build
export BUILD_IMAGE_NAME ?= tryspace-lab
export RUNTIME_SIM_IMAGE_NAME ?= tryspace-sim

# Commands
all: build

build:
	docker run --rm -it -v $(CURDIR):$(CURDIR) --name "tryspace_sim_build" -w $(CURDIR) $(BUILD_IMAGE_NAME) make -j build-sim

build-sim:
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake ..
	$(MAKE) --no-print-directory -C $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

debug:
	docker run --rm -it -v $(CURDIR):$(CURDIR) --name "tryspace_sim_debug" -w $(CURDIR) $(BUILD_IMAGE_NAME) /bin/bash

runtime:
	$(MAKE) clean build
	docker build -t $(RUNTIME_SIM_IMAGE_NAME) -f test/Dockerfile.sim .

start:
	docker run --rm -it --name "tryspace_sim_runtime" $(RUNTIME_SIM_IMAGE_NAME) ./simulith_server_standalone

stop:
	docker ps --filter name=tryspace-* --filter status=running -aq | xargs docker stop

test:
	docker run --rm -it --name "tryspace_sim_test" $(RUNTIME_SIM_IMAGE_NAME) make test
