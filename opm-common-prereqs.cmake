# -*- mode: cmake; tab-width: 2; indent-tabs-mode: t; truncate-lines: t; compile-command: "cmake -Wdev" -*-
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 noexpandtab softtabstop=2 nowrap:

# defines that must be present in config.h for our headers
set (opm-common_CONFIG_VAR
	HAVE_OPENMP
	)

# dependencies
set (opm-common_DEPS
	# compile with C99 support if available
	"C99"
)

list(APPEND opm-common_DEPS
      # various runtime library enhancements
      "Boost 1.44.0 COMPONENTS system unit_test_framework REQUIRED"
      "OpenMP QUIET"
      "HDF5 COMPONENTS C HL REQUIRED"
)

find_package_deps(opm-common)
