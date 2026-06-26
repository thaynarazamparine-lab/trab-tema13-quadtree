import matplotlib.pyplot as plt
import csv
import os

def plot_stress():
    n_vals, brute_vals, quad_vals = [], [], []
    try:
        with open('results/stress_metrics.csv', 'r') as f:
            reader = csv.reader(f)
            next(reader)
            for row in reader:
                n_vals.append(int(row[0]))
                brute_vals.append(float(row[1]))
                quad_vals.append(float(row[2]))
                
        plt.figure(figsize=(8, 5))
        plt.plot(n_vals, brute_vals, marker='o', label='O(n²) - Forca Bruta', color='red')
        plt.plot(n_vals, quad_vals, marker='o', label='Quadtree', color='blue')
        plt.title('Desempenho: Forca Bruta vs Quadtree')
        plt.xlabel('Numero de Particulas (N)')
        plt.ylabel('Tempo (s)')
        plt.legend()
        plt.grid(True)
        plt.savefig('results/grafico_stress.png')
        plt.close()
    except FileNotFoundError:
        pass

def plot_speedup():
    threads, tempos = [], []
    try:
        with open('results/speedup_metrics.csv', 'r') as f:
            reader = csv.reader(f)
            next(reader)
            for row in reader:
                threads.append(int(row[0]))
                tempos.append(float(row[1]))
                
        plt.figure(figsize=(8, 5))
        plt.plot(threads, tempos, marker='s', color='green', linestyle='--')
        plt.title('Escalabilidade (Threads)')
        plt.xlabel('Numero de Threads')
        plt.ylabel('Tempo (s)')
        plt.xticks(threads)
        plt.grid(True)
        plt.savefig('results/grafico_speedup.png')
        plt.close()
    except FileNotFoundError:
        pass

if __name__ == '__main__':
    if not os.path.exists('results'):
        os.makedirs('results')
    plot_stress()
    plot_speedup()
