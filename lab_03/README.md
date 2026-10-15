# Лабораторная работа №3

## Вариант 3

Программа сортирует массив целых чисел параллельным слиянием. Числа читаются из файла;
`--threads N` задаёт предел числа потоков вместе с главным потоком.
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

С ключом `--observe` программа покажет PID и дождётся Enter. Число потоков можно проверить командой `ps -L -p PID` в Linux или `ps -M -p PID` в macOS.

## Отчёт и трассировка

Исходник отчёта находится в `report/report.tex`, замеры — в
`report/measurements.csv`. Для сборки PDF нужен `pdflatex`:

```bash
cd lab_03/report
make
```