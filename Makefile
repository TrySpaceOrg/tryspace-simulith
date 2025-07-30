# Makefile for TrySpace Simulith development
.PHONY: all build clean debug director server start stop test

export BUILDDIR ?= $(CURDIR)/build
export TOPDIR ?= $(CURDIR)/..

export BUILD_IMAGE_NAME ?= tryspace-lab
export RUNTIME_DIRECTOR_NAME ?= tryspace-director
export RUNTIME_SERVER_NAME ?= tryspace-server

# Commands
all: build

build:
	docker run --rm -it -v $(TOPDIR):$(TOPDIR) --user $(shell id -u):$(shell id -g) --name $(RUNTIME_SERVER_NAME) -w $(CURDIR) $(BUILD_IMAGE_NAME) make -j build-sim

build-sim:
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake -DBUILD_SIMULITH_TESTS=ON ..
	$(MAKE) --no-print-directory -C $(BUILDDIR)
	# Create components directory and copy shared libraries
	mkdir -p $(BUILDDIR)/components
	cp $(BUILDDIR)/*.so $(BUILDDIR)/components/ 2>/dev/null || true
	cp $(TOPDIR)/comp/demo/sim/build/*.so $(BUILDDIR)/components/ 2>/dev/null || true

clean:
	rm -rf $(BUILDDIR)

debug:
	docker run --rm -it -v $(TOPDIR):$(TOPDIR) --user $(shell id -u):$(shell id -g) --name $(RUNTIME_DIRECTOR_NAME) -w $(CURDIR) $(RUNTIME_DIRECTOR_NAME) /bin/bash

director:
	$(MAKE) clean build
	docker build -t $(RUNTIME_DIRECTOR_NAME) -f test/Dockerfile.director .

server:
	$(MAKE) clean build
	docker build -t $(RUNTIME_SERVER_NAME) -f test/Dockerfile.server .

start:
	docker run --rm -it --name $(RUNTIME_SERVER_NAME) $(RUNTIME_SERVER_NAME) ./simulith_server_standalone

stop:
	docker ps --filter name=tryspace-* | xargs docker stop

test:
	docker run --rm -it -v $(CURDIR):$(CURDIR) --name $(RUNTIME_SERVER_NAME) --user $(shell id -u):$(shell id -g) -w $(BUILDDIR) $(BUILD_IMAGE_NAME) make test
