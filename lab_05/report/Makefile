MAIN = report
PDFLATEX := $(shell command -v pdflatex 2>/dev/null)

$(MAIN).pdf: FORCE $(MAIN).tex preamble.tex titlepage.tex trace/linked.txt trace/dynamic.txt ../src/lib1.c ../src/lib2.c ../src/program1.c ../src/program2.c ../CMakeLists.txt
ifeq ($(PDFLATEX),)
	tectonic $(MAIN).tex
else
	$(PDFLATEX) -interaction=nonstopmode -halt-on-error $(MAIN).tex
	$(PDFLATEX) -interaction=nonstopmode -halt-on-error $(MAIN).tex
endif

clean:
	rm -f *.aux *.log *.out *.lol *.toc *.fls *.fdb_latexmk *.synctex.gz

FORCE:
.PHONY: FORCE clean
