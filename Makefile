# Makefile for TrySpace Simulith development
.PHONY: build clean debug director server stop test

export BUILDDIR ?= $(CURDIR)/build
export TOPDIR ?= $(CURDIR)/..

export BUILD_IMAGE ?= tryspaceorg/tryspace-lab:0.0.0
export CONTAINER_NAME ?= tryspace-lab
export RUNTIME_DIRECTOR_NAME ?= tryspace-director
export RUNTIME_SERVER_NAME ?= tryspace-server

# Determine number of parallel jobs to avoid maxing out low-power systems (Raspberry Pi etc.).
# Use `nproc - 1` but ensure at least 1 job.
NPROC := $(shell nproc 2>/dev/null || echo 1)
JOBS := $(shell if [ $(NPROC) -le 1 ]; then echo 1; else expr $(NPROC) - 1; fi)

# Commands
build:
	docker run --rm -v $(TOPDIR):$(TOPDIR) --user $(shell id -u):$(shell id -g) --name $(CONTAINER_NAME) -w $(CURDIR) $(BUILD_IMAGE) make -j$(JOBS) build-sim

build-director: 
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake ..
	$(MAKE) --no-print-directory -C $(BUILDDIR) simulith_director_standalone

build-server: 
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake ..
	$(MAKE) --no-print-directory -C $(BUILDDIR) simulith_server_standalone
	# Create components directory and copy shared libraries
	mkdir -p $(BUILDDIR)/components
	@for dir in $(TOPDIR)/comp/*/sim/build ; do \
		if [ -d "$$dir" ] && [ -f "$$dir/Makefile" ]; then \
			cp $$dir/*.so $(BUILDDIR)/components/ 2>/dev/null || true; \
		fi; \
	done
	# Copy 42 configuration to build directory for runtime
	cp -r 42/InOut $(BUILDDIR)/ 2>/dev/null || true
	cp -r 42/Model $(BUILDDIR)/ 2>/dev/null || true

build-sim:
	$(MAKE) build-director
	$(MAKE) build-server

build-test:
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake .. -DBUILD_SIMULITH_TESTS=ON
	$(MAKE) --no-print-directory -C $(BUILDDIR)
	cd $(BUILDDIR) && ctest --output-on-failure -O ctest.log

clean:
	rm -rf $(BUILDDIR)

debug:
	docker run --rm -it -v $(TOPDIR):$(TOPDIR) --user $(shell id -u):$(shell id -g) --name $(CONTAINER_NAME) -w $(CURDIR) $(BUILD_IMAGE) /bin/bash

director:
	$(MAKE) clean build
	docker build -t $(RUNTIME_DIRECTOR_NAME) -f test/Dockerfile.director .

server:
	$(MAKE) clean build
	docker build -t $(RUNTIME_SERVER_NAME) -f test/Dockerfile.server .

stop:
	docker ps --filter name=tryspace-* | xargs docker stop

test:
	docker run --rm -v $(TOPDIR):$(TOPDIR) --user $(shell id -u):$(shell id -g) --name $(CONTAINER_NAME) -w $(CURDIR) $(BUILD_IMAGE) make -j$(JOBS) build-test
