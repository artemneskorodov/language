PROJECTS = common frontend backend frontstart middleend

.PHONY: all clean rebuild

all: $(PROJECTS)

$(PROJECTS):
	@echo "===============" Building $@ "==============="
	$(MAKE) -C $@

clean:
	@for dir in $(PROJECTS); do \
		echo "===============" Clearing $$dir "==============="; \
		$(MAKE) -C $$dir clean; \
	done
	rm -rf logs
	rm -rf bin

rebuild: clean all

del_logs:
	rm -rf logs
