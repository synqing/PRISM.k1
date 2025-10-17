PYTHON ?= python3
RUNNER := -m tools.parser_testbed.runner
VECTOR_DIR ?= tools/parser_testbed/vectors
REPORT ?= tools/parser_testbed/vectors/report.json

.PHONY: vectors mutations regress clean-report

vectors:
	$(PYTHON) $(RUNNER) generate --output-dir $(VECTOR_DIR) --include-mutations

mutations:
	$(PYTHON) $(RUNNER) mutate --output-dir $(VECTOR_DIR)

regress:
	$(PYTHON) $(RUNNER) regress --input-dir $(VECTOR_DIR) --report $(REPORT)

clean-report:
	rm -f $(REPORT)
