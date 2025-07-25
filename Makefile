# Makefile for TrySpace Simulith development
.PHONY: all build clean debug runtime start stop test

export BUILDDIR ?= $(CURDIR)/build
export BUILD_IMAGE_NAME ?= tryspace-lab
export RUNTIME_NAME ?= tryspace-sim

# Commands
all: build

build:
	docker run --rm -it -v $(CURDIR):$(CURDIR) --user $(shell id -u):$(shell id -g) --name $(RUNTIME_NAME) -w $(CURDIR) $(BUILD_IMAGE_NAME) make -j build-sim

build-sim:
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake -DBUILD_SIMULITH_TESTS=ON ..
	$(MAKE) --no-print-directory -C $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

debug:
	docker run --rm -it -v $(CURDIR):$(CURDIR) --user $(shell id -u):$(shell id -g) --name $(RUNTIME_NAME) -w $(CURDIR) $(BUILD_IMAGE_NAME) /bin/bash

runtime:
	$(MAKE) clean build
	docker build -t $(RUNTIME_NAME) -f test/Dockerfile.sim .

start:
	docker run --rm -it --name $(RUNTIME_NAME) $(RUNTIME_NAME) ./simulith_server_standalone

stop:
	docker ps --filter name=tryspace-* | xargs docker stop

test:
	docker run --rm -it -v $(CURDIR):$(CURDIR) --name $(RUNTIME_NAME) --user $(shell id -u):$(shell id -g) -w $(BUILDDIR) $(BUILD_IMAGE_NAME) make test
