# Power‑Optimized Technology Mapping  

---

## 1  Problem Statement
Technology‑mapping is fundamentally a **delay–power trade‑off** problem.  
Given a combinational netlist expressed in BLIF/AIG, the task is to re‑implement every logic node with cells from a small technology library **while respecting timing**:

* We first obtain the **baseline critical‑path delay** by mapping every node to the **loweset delay but highest power cell** (INV1 / NAND1).  
  This value, noted **D<sub>ref</sub>**, is the upper limit that subsequent optimisations must never exceed.
* During optimisation we are allowed to **stretch individual gate delays**—replacing them with **lower‑power but slower variants**—as long as each path still meets **`arrival ≤ tolerance`**.  
  In other words, only the **positive slack** of a node can be traded for power reduction.
* The objective is therefore to **minimize total power** subject to the constraint  
  `max_path_delay ≤ tolerance`.

The mapped circuit must remain functionally equivalent to the original and is emitted in the modified **`.mbench`** format.

---

## 2  Inputs
| File | Description |
|------|-------------|
| `*.blif` | Original netlist (Berkeley BLIF). |
| `pa2.lib` | Technology library for mapping. |

### 2.1  Library Timing / Power Models
#### • Inverter cells
| Pattern | Delay *(ns)*<br>`0.28 + 0.72 × n` | Power *(mW)* |
|---------|-------------------------------------|---------------|
| **INV1** | 0.28 + 0.72 × n | 20.92 |
| **INV2** | 1.03 + 2.64 × n | 1.00 |
| **INV3** | 0.47 + 1.20 × n | 2.75 |
| **INV4** | 0.82 + 2.10 × n | 19.25 |

#### • NAND cells
| Pattern | Delay *(ns)*<br>`0.56 + 1.44 × n` | Power *(mW)* |
|---------|--------------------------------------|---------------|
| **NAND1** | 0.56 + 1.44 × n | 25.76 |
| **NAND2** | 2.31 + 5.95 × n | 1.40 |
| **NAND3** | 1.16 + 3.00 × n | 10.82 |
| **NAND4** | 1.79 + 4.61 × n | 25.38 |

*(n = fan‑out count)*

---

## 3  Output
The mapped netlist is written to `<circuit>.mbench` in the working directory.

```text
INPUT(N1)
INPUT(N4)
...
G15 = NAND2(N1, N4)      #slack = 0.83
G16 = INV3(G15)          #slack = 1.75
OUTPUT(G16)
```

---

## 4  Algorithm Overview  

### 4.1  Mapping Rules
1. **AND node** → `NANDx` + `INVx` (phase restore).
2. Complemented fan‑in → `INVx`.
3. Consecutive inverters on the same wire are removed.

### 4.2  Timing & Slack
* **ASAP** pass → arrival times.
* **ALAP** pass → required times.
* **Slack = Rq − Ar**; nodes with *|slack| ≤ ε* are treated as critical.

### 4.3  Greedy Power Pass
For each level (high → low):
1. Skip zero‑slack nodes.
2. Try slower, lower‑power variants (Δdelay ≤ slack + `tolerance`).
3. Update timing; iterate until `TotalPower` converges.

---

## 5  Build & Run
```bash
# compile
make clean && make        # or simply `make`

# run mapper
./ace  <netlist.blif>  pa2.lib
```

Example session:
```bash
$ ./ace blif/cm42a.blif pa2.lib
Process: ./pa2.lib
AND nodes =    504  Estimate = 1454
Initial delay: 8.00
Final delay:   9.00
Original power: 46.68
Optimized power: 13.57
...
done!!
```
Output written to `cm42a.mbench`.

---

## 6  Deliverables
| Item | Note |
|------|------|
| **Source code** | `main.cpp`, `graph.h`, `makefile` |
| **Library** | `pa2.lib` |
| **Results** | ISCAS‑85 mapped benches `c432 – c7552` |
| **README** | this file |


---

© 2025 B10807022 – Dept. EECS, NTUST

