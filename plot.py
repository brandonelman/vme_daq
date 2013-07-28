import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
from scipy import loadtxt, savetxt
from scipy.integrate import simps, cumtrapz
import glob

data = []
for fn in sorted(glob.glob("*.dat")):
  data.append(loadtxt(fn))

print sorted(glob.glob("*.dat"))

plt.figure()

for ch in range(len(data)):
    samples = np.arange(len(data[ch]))
    plt.plot(samples, data[ch], label='Ch {0}'.format(ch))
    plt.ylabel('Amplitude')
    plt.xlabel('Time') 
plt.savefig("Channels.pdf", format='pdf')

ped = data[0][0]
for sample in range(len(data[0])):
  data[0][sample] = data[0][sample] - ped 

integrated = []
i = 0

print(len(data[0]))
num_pulses = int(len(data[0]) / 2520.0)


print "num_pulses", num_pulses

for i in range(num_pulses):
  integrated.append(simps(data[0][i*2520:(i+1)*2520-1])) 
#integrated = cumtrapz(data[0], None, 0.5)


savetxt("integration.txt", integrated)
plt.figure()
n, bins = np.histogram(integrated, bins=20)
print 'n', n, 'bins', bins
bincenters = 0.5*(bins[1:]+bins[:-1])
menStd = np.std(bins)/np.sqrt(n)
width = 0.05 
plt.bar(bincenters, n, width=width, color='r', yerr=menStd)
plt.xlabel('Amplitude')
plt.ylabel('Counts')
plt.savefig("Integrated.pdf", format='pdf')
