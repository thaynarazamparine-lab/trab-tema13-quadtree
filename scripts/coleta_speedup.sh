#!/bin/bash

mkdir -p obj results

echo "=== Compilando e Executando Teste de Fogo ==="
gcc -Wall -Wextra -std=c11 -O3 -Iinclude tests/test_stress.c src/quadtree.c -lm -o obj/test_stress
./obj/test_stress > results/stress_metrics.csv
cat results/stress_metrics.csv

echo -e "\n=== Coletando Metricas de Speedup (1, 2, 4, 8 Threads) ==="
gcc -Wall -Wextra -std=c11 -O3 -Iinclude src/sim_parallel.c src/quadtree.c src/transfer.c src/metrics.c -lm -lpthread -o obj/sim_parallel

echo "Threads,Tempo_Execucao" > results/speedup_metrics.csv
for threads in 1 2 4 8; do
    echo -n "Testando com $threads thread(s)... "
    tempo=$(/usr/bin/time -f "%e" ./obj/sim_parallel $threads 2>&1 | tail -n 1)
    echo "$threads,$tempo" >> results/speedup_metrics.csv
    echo "$tempo s"
done
