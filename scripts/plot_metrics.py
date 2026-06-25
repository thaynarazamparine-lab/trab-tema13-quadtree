#!/usr/bin/env python3
"""
plot_metrics.py — Gera gráficos a partir dos CSVs da simulação.

Dependências: matplotlib (stdlib: csv, os, sys, math)

Uso:
    python3 scripts/plot_metrics.py [normal|imbalance|both]

Saída: results/graphs/*.png
"""

import sys
import os
import csv
import math
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

# ── Parâmetros ─────────────────────────────────────────────────────────────
CSV_DIRS = {
    'normal':    'results/csv/normal',
    'imbalance': 'results/csv/imbalance',
}
GRAPH_DIR = 'results/graphs'
DPI = 150


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def load_csv(csv_dir, filename):
    """Lê CSV e retorna dict de listas: {coluna: [valores]}."""
    path = os.path.join(csv_dir, filename)
    if not os.path.exists(path):
        return None
    data = {}
    try:
        with open(path, newline='') as f:
            reader = csv.DictReader(f)
            for row in reader:
                for k, v in row.items():
                    data.setdefault(k, []).append(float(v))
    except Exception as e:
        print(f'  Aviso: nao foi possivel ler {path}: {e}')
        return None
    return data if data else None


def col(data, key):
    """Acessa coluna com fallback para lista vazia."""
    return data.get(key, []) if data else []


def save(fig, name):
    ensure_dir(GRAPH_DIR)
    path = os.path.join(GRAPH_DIR, name)
    fig.savefig(path, dpi=DPI, bbox_inches='tight')
    plt.close(fig)
    print(f'  Salvo: {path}')


# ── Gráfico 1: Tempo por frame ──────────────────────────────────────────────
def plot_frame_time(label, csv_dir):
    d = load_csv(csv_dir, 'time.csv')
    if not d:
        return
    fig, ax = plt.subplots()
    ax.plot(col(d, 'frame'), col(d, 'time_ms'), linewidth=0.8)
    ax.set_title(f'Tempo por Frame — {label}')
    ax.set_xlabel('Frame')
    ax.set_ylabel('Tempo (ms)')
    ax.grid(True, alpha=0.3)
    save(fig, f'{label}_01_tempo_por_frame.png')


# ── Gráfico 2: Profundidade da Quadtree ────────────────────────────────────
def plot_tree_depth(label, csv_dir):
    d = load_csv(csv_dir, 'tree.csv')
    if not d:
        return
    fig, ax = plt.subplots()
    ax.plot(col(d, 'frame'), col(d, 'max_depth'), label='profundidade máxima', linewidth=0.8)
    ax.plot(col(d, 'frame'), col(d, 'avg_depth'), label='profundidade média',  linewidth=0.8, linestyle='--')
    ax.set_title(f'Profundidade da Quadtree — {label}')
    ax.set_xlabel('Frame')
    ax.set_ylabel('Profundidade')
    ax.legend()
    ax.grid(True, alpha=0.3)
    save(fig, f'{label}_02_profundidade_quadtree.png')


# ── Gráfico 3: Quantidade de nós ───────────────────────────────────────────
def plot_node_count(label, csv_dir):
    d = load_csv(csv_dir, 'tree.csv')
    if not d:
        return
    fig, ax = plt.subplots()
    ax.plot(col(d, 'frame'), col(d, 'total_nodes'), linewidth=0.8, color='steelblue')
    ax.set_title(f'Nós da Quadtree ao Longo do Tempo — {label}')
    ax.set_xlabel('Frame')
    ax.set_ylabel('Total de Nós')
    ax.grid(True, alpha=0.3)
    save(fig, f'{label}_03_nos_quadtree.png')


# ── Gráfico 4: Partículas por thread ───────────────────────────────────────
def plot_thread_particles(label, csv_dir):
    d = load_csv(csv_dir, 'threads.csv')
    if not d:
        return
    frames    = col(d, 'frame')
    tids      = col(d, 'thread_id')
    particles = col(d, 'particles_processed')

    # Agrupa por thread_id
    groups = {}
    for f, t, p in zip(frames, tids, particles):
        groups.setdefault(int(t), {'frames': [], 'particles': []})
        groups[int(t)]['frames'].append(f)
        groups[int(t)]['particles'].append(p)

    fig, ax = plt.subplots()
    for tid in sorted(groups):
        ax.plot(groups[tid]['frames'], groups[tid]['particles'],
                label=f'Thread {tid}', linewidth=0.8)
    ax.set_title(f'Partículas por Thread — {label}')
    ax.set_xlabel('Frame')
    ax.set_ylabel('Partículas Processadas')
    ax.legend()
    ax.grid(True, alpha=0.3)
    save(fig, f'{label}_04_particulas_por_thread.png')


# ── Gráfico 5: Tempo por thread ────────────────────────────────────────────
def plot_thread_time(label, csv_dir):
    d = load_csv(csv_dir, 'threads.csv')
    if not d:
        return
    frames = col(d, 'frame')
    tids   = col(d, 'thread_id')
    times  = col(d, 'time_spent_ms')

    groups = {}
    for f, t, tm in zip(frames, tids, times):
        groups.setdefault(int(t), {'frames': [], 'times': []})
        groups[int(t)]['frames'].append(f)
        groups[int(t)]['times'].append(tm)

    fig, ax = plt.subplots()
    for tid in sorted(groups):
        ax.plot(groups[tid]['frames'], groups[tid]['times'],
                label=f'Thread {tid}', linewidth=0.8)
    ax.set_title(f'Tempo por Thread — {label}')
    ax.set_xlabel('Frame')
    ax.set_ylabel('Tempo (ms)')
    ax.legend()
    ax.grid(True, alpha=0.3)
    save(fig, f'{label}_05_tempo_por_thread.png')


# ── Gráfico 6: Migrações entre regiões ────────────────────────────────────
def plot_migrations(label, csv_dir):
    d = load_csv(csv_dir, 'transfers.csv')
    if not d:
        return
    fig, ax = plt.subplots()
    ax.plot(col(d, 'frame'), col(d, 'migrations'), linewidth=0.8, color='darkorange')
    ax.set_title(f'Migrações Entre Regiões por Frame — {label}')
    ax.set_xlabel('Frame')
    ax.set_ylabel('Migrações')
    ax.grid(True, alpha=0.3)
    save(fig, f'{label}_06_migracoes.png')


# ── Gráfico 7: Fator de desbalanceamento ──────────────────────────────────
def plot_imbalance_factor(label, csv_dir):
    d = load_csv(csv_dir, 'balance.csv')
    if not d:
        return
    imb_pct = [v * 100.0 for v in col(d, 'imbalance')]
    fig, ax = plt.subplots()
    ax.plot(col(d, 'frame'), imb_pct, linewidth=0.8, color='crimson')
    ax.set_title(f'Fator de Desbalanceamento — {label}')
    ax.set_xlabel('Frame')
    ax.set_ylabel('Desbalanceamento (%)')
    ax.grid(True, alpha=0.3)
    save(fig, f'{label}_07_desbalanceamento.png')


# ── Gráfico 8: Carga média vs. máxima ─────────────────────────────────────
def plot_load_comparison(label, csv_dir):
    d = load_csv(csv_dir, 'balance.csv')
    if not d:
        return
    frames   = col(d, 'frame')
    avg_load = col(d, 'avg_load')
    max_load = col(d, 'max_load')

    fig, ax = plt.subplots()
    ax.plot(frames, avg_load, label='Carga Média', linewidth=0.8)
    ax.plot(frames, max_load, label='Carga Máxima', linewidth=0.8, linestyle='--')
    ax.fill_between(frames, avg_load, max_load, alpha=0.15, label='Desequilíbrio')
    ax.set_title(f'Carga Média vs. Máxima — {label}')
    ax.set_xlabel('Frame')
    ax.set_ylabel('Partículas')
    ax.legend()
    ax.grid(True, alpha=0.3)
    save(fig, f'{label}_08_carga_media_vs_maxima.png')


# ── Execução ───────────────────────────────────────────────────────────────
def generate_all(label, csv_dir):
    print(f'\nGerando gráficos para: {label} ({csv_dir})')
    plot_frame_time(label, csv_dir)
    plot_tree_depth(label, csv_dir)
    plot_node_count(label, csv_dir)
    plot_thread_particles(label, csv_dir)
    plot_thread_time(label, csv_dir)
    plot_migrations(label, csv_dir)
    plot_imbalance_factor(label, csv_dir)
    plot_load_comparison(label, csv_dir)


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else 'both'
    if mode in ('normal', 'both'):
        generate_all('normal', CSV_DIRS['normal'])
    if mode in ('imbalance', 'both'):
        generate_all('imbalance', CSV_DIRS['imbalance'])
    print('\nConcluido. Verifique results/graphs/')
