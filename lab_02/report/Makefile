MAIN   = report
LATEX ?= pdflatex
FLAGS  = -interaction=nonstopmode -halt-on-error
$(MAIN).pdf: FORCE $(MAIN).tex preamble.tex titlepage.tex ../src/parent.c ../src/child.c ../CMakeLists.txt trace/strace.txt trace/strace_short.txt
	$(LATEX) $(FLAGS) $(MAIN).tex
	$(LATEX) $(FLAGS) $(MAIN).tex
xelatex:
	$(MAKE) LATEX=xelatex
FORCE:
clean:
	rm -f *.aux *.log *.out *.lol *.toc *.fls *.fdb_latexmk *.synctex.gz
.PHONY: FORCE xelatex clean
