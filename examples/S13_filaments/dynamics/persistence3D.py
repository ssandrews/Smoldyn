# Measures filament persistence lengths from persistence3D.txt output.
#
# Run "smoldyn persistence3D.txt" first, which creates persistence3Dout.txt, and then
# "python3 persistence3D.py". For each filament type, this computes the average
# correlation between segment directions that are n segments apart, <t0.tn>, and
# reports the persistence length that it implies, Lp = -n l / ln <t0.tn>, where l = 1
# is the standard segment length. Uncertainties are standard errors from dividing the
# samples into 10 blocks of time. This requires numpy.

import math
import numpy as np

predicted = {"sym": 10, "asym": 16, "asymroll": 16}  # from 2 l kyaw kpitch / (kT (kyaw + kpitch))
nmax = 8
nblock = 10

tangents = {}
for line in open("persistence3Dout.txt"):
    words = line.split()
    if not words or words[0] != "FIL":
        continue
    ftype = words[2].split(":")[0]
    nseg = int(words[3])
    nodes = np.array(words[8:], dtype=float).reshape(nseg + 1, 3)
    t = np.diff(nodes, axis=0)
    t /= np.linalg.norm(t, axis=1)[:, None]
    tangents.setdefault(ftype, []).append(t)

sep = np.arange(1, nmax + 1)


def correlations(tlist):
    return np.array([np.mean([(t[:-n] * t[n:]).sum(1).mean() for t in tlist]) for n in sep])


print("Lp measured from <t0.tn> = exp(-n/Lp), for separations of n segments")
print("type       predicted  " + " ".join(f"{'n=' + str(n):>11s}" for n in sep))
for ftype, tlist in tangents.items():
    lp = -sep / np.log(correlations(tlist))
    blocks = np.array_split(np.arange(len(tlist)), nblock)
    blocklp = np.array([-sep / np.log(correlations([tlist[i] for i in b])) for b in blocks])
    err = blocklp.std(0, ddof=1) / math.sqrt(nblock)
    print(f"{ftype:10s} {predicted.get(ftype, float('nan')):9g}  "
          + " ".join(f"{v:6.2f}+-{e:.2f}" for v, e in zip(lp, err)))
