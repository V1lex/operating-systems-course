MAIN = report
PDFLATEX := $(shell command -v pdflatex 2>/dev/null)

$(MAIN).pdf: FORCE $(MAIN).tex preamble.tex titlepage.tex measurements.csv assets/speedup.png assets/efficiency.png trace/strace.txt trace/strace_short.txt ../src/merge_sort.c ../CMakeLists.txt
ifeq ($(PDFLATEX),)
	tectonic $(MAIN).tex
else
	$(PDFLATEX) -interaction=nonstopmode -halt-on-error $(MAIN).tex
	$(PDFLATEX) -interaction=nonstopmode -halt-on-error $(MAIN).tex
endif

clean:
	rm -f *.aux *.log *.out *.lol *.toc *.fls *.fdb_latexmk *.synctex.gz

FORCE:
.PHONY: FORCE xelatex clean
