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
    samples = np.arange(len(data[ch][0:2519]))
    plt.plot(samples, data[ch][0:2519], label='Ch {0}'.format(ch))
    plt.ylabel('Amplitude')
    plt.xlabel('Time') 
plt.savefig("Channels.pdf", format='pdf')

integrated = []
i = 0

print(len(data[0]))
num_pulses = int(len(data[0]) / 2520.0)
print "num_pulses", num_pulses

#ped = data[0][0]
for sample in range(2520*num_pulses):
  if sample % 2520 == 0:
    ped = data[0][sample]
  data[0][sample] = data[0][sample]-ped
  

for i in range(num_pulses):
  integrated.append(simps(data[0][i*2520:(i+1)*2520-1])) 
#integrated = cumtrapz(data[0], None, 0.5)


savetxt("integration.txt", integrated)
plt.figure()
n, bins = np.histogram(integrated, bins=6)
print 'n', n, 'bins', bins
bincenters = 0.5*(bins[1:]+bins[:-1])
menStd = np.sqrt(n)
width = 1.00 
plt.bar(bincenters, n, width=width, color='g', yerr=menStd)
plt.xlabel('Amplitude')
plt.ylabel('Counts')
plt.savefig("Integrated.pdf", format='pdf')