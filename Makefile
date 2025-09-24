# Makefile for TrySpace Simulith development
.PHONY: build clean debug director server stop test

export BUILDDIR ?= $(CURDIR)/build
export TOPDIR ?= $(CURDIR)/..

export BUILD_IMAGE ?= tryspaceorg/tryspace-lab:0.0.1
export CONTAINER_NAME ?= tryspace-lab
export RUNTIME_DIRECTOR_NAME ?= tryspace-director-$(MISSION)
export RUNTIME_SERVER_NAME ?= tryspace-server-$(MISSION)
export SPACECRAFT ?= latest
export MISSION ?= default

# Determine number of parallel jobs to avoid maxing out low-power systems (Raspberry Pi etc.).
# Use `nproc - 1` but ensure at least 1 job.
NPROC := $(shell nproc 2>/dev/null || echo 1)
JOBS := $(shell if [ $(NPROC) -le 1 ]; then echo 1; else expr $(NPROC) - 1; fi)

# Commands
build:
	docker run --rm -v $(TOPDIR):$(TOPDIR) --user $(shell id -u):$(shell id -g) --name $(CONTAINER_NAME) -w $(CURDIR) -e BUILDDIR=$(BUILDDIR) $(BUILD_IMAGE) make -j$(JOBS) build-sim

build-director: 
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake $(CURDIR)
	$(MAKE) --no-print-directory -C $(BUILDDIR) simulith_director_standalone

build-server: 
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake $(CURDIR)
	$(MAKE) --no-print-directory -C $(BUILDDIR) simulith_server_standalone

build-sim:
	$(MAKE) build-director
	$(MAKE) build-server

build-test:
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake $(CURDIR) -DBUILD_SIMULITH_TESTS=ON -DENABLE_UNIT_TESTS=true
	$(MAKE) --no-print-directory -C $(BUILDDIR)
	cd $(BUILDDIR) && ctest --output-on-failure -O ctest.log
	lcov -c --directory . --output-file $(BUILDDIR)/coverage.info
	lcov --remove $(BUILDDIR)/coverage.info '*/test/*' '*/tests/*' --output-file $(BUILDDIR)/coverage.filtered.info
	genhtml $(BUILDDIR)/coverage.filtered.info --output-directory $(BUILDDIR)/report
	@echo ""
	@echo "Review coverage report: "
	@echo "  firefox $(BUILDDIR)/report/index.html"
	@echo ""

copy-sims:
	@if [ ! -d "$(BUILDDIR)" ]; then \
		echo "Error: Build directory $(BUILDDIR) does not exist. Please run 'make build' first."; \
		exit 1; \
	fi
	mkdir -p $(BUILDDIR)/components
	@# Check if running from top-level with unified build structure
	@if [ -n "$(BUILDDIR_COMP)" ]; then \
		for comp_dir in $(TOPDIR)/comp/*/sim ; do \
			if [ -d "$$comp_dir" ]; then \
				comp_name=$$(basename $$(dirname "$$comp_dir")); \
				comp_build_dir="$(BUILDDIR_COMP)/$$comp_name/sim"; \
				if [ -d "$$comp_build_dir" ]; then \
					cp $$comp_build_dir/*.so $(BUILDDIR)/components/ 2>/dev/null || true; \
				fi; \
			fi; \
		done; \
	else \
		for dir in $(TOPDIR)/comp/*/sim/build ; do \
			if [ -d "$$dir" ]; then \
				cp $$dir/*.so $(BUILDDIR)/components/ 2>/dev/null || true; \
			fi; \
		done; \
	fi
	cp -r 42/InOut $(BUILDDIR)/ 2>/dev/null || true
	cp -r 42/Model $(BUILDDIR)/ 2>/dev/null || true

clean:
	rm -rf $(BUILDDIR)

debug:
	docker run --rm -it -v $(TOPDIR):$(TOPDIR) --user $(shell id -u):$(shell id -g) --name $(CONTAINER_NAME) -w $(CURDIR) $(BUILD_IMAGE) /bin/bash

director: copy-sims
	@if [ ! -d "$(BUILDDIR)" ]; then \
		echo "Error: Build directory $(BUILDDIR) does not exist. Please run 'make build' first."; \
		exit 1; \
	fi
	@# Prepare Docker build context
	@if [ "$(BUILDDIR)" != "$(CURDIR)/build" ]; then \
		rm -rf build; \
		mkdir -p build; \
		cp -L $(BUILDDIR)/* build/ 2>/dev/null || true; \
		cp -rL $(BUILDDIR)/components build/ 2>/dev/null || true; \
		cp -rL $(BUILDDIR)/InOut build/ 2>/dev/null || true; \
		cp -rL $(BUILDDIR)/Model build/ 2>/dev/null || true; \
	fi
	docker build -t $(RUNTIME_DIRECTOR_NAME):$(SPACECRAFT) -f docker/Dockerfile.director .

server:
	@if [ ! -d "$(BUILDDIR)" ]; then \
		echo "Error: Build directory $(BUILDDIR) does not exist. Please run 'make build' first."; \
		exit 1; \
	fi
	@# Prepare Docker build context
	@if [ "$(BUILDDIR)" != "$(CURDIR)/build" ]; then \
		rm -rf build; \
		mkdir -p build; \
		cp -L $(BUILDDIR)/* build/ 2>/dev/null || true; \
		cp -rL $(BUILDDIR)/components build/ 2>/dev/null || true; \
		cp -rL $(BUILDDIR)/InOut build/ 2>/dev/null || true; \
		cp -rL $(BUILDDIR)/Model build/ 2>/dev/null || true; \
	fi
	docker build -t $(RUNTIME_SERVER_NAME):$(SPACECRAFT) -f docker/Dockerfile.server .

stop:
	docker ps --filter name=tryspace-* | xargs docker stop

test:
	docker run --rm -v $(TOPDIR):$(TOPDIR) --user $(shell id -u):$(shell id -g) --name $(CONTAINER_NAME) -w $(CURDIR) $(BUILD_IMAGE) make -j$(JOBS) build-test
