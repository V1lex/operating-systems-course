# Лабораторная работа №3

## Вариант 3

Программа сортирует массив целых чисел параллельным слиянием. Числа читаются
из файла; `--threads N` задаёт максимальное число потоков, включая главный.
Результат выводится в `stdout`, время сортировки — в `stderr`.

## Сборка из корня

```bash
cmake -S lab_03 -B build/lab_03
cmake --build build/lab_03
```

## Запуск

```bash
./build/lab_03/lab3_merge_sort --threads 4 --input lab_03/data/sample.txt
```

Файлы `data/numbers_100.txt` — `data/numbers_1000000.txt` используются
для измерений. Ключ `--observe` показывает PID и ждёт Enter: в другой
консоли число потоков можно проверить через `ps -L -p PID` в Linux или
`ps -M -p PID` в macOS.

## Отчёт и трассировка

Исходник отчёта — `report/report.tex`, таблица замеров —
`report/measurements.csv`, графики — в `report/assets/`. PDF собирается
через `pdflatex` или `tectonic`:

```bash
cd lab_03/report
make
```

Полная трассировка Ubuntu находится в `report/trace/strace.txt`, краткая —
в `report/trace/strace_short.txt`.
