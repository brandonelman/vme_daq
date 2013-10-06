import sys, os
import matplotlib
import math
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
from scipy import loadtxt, savetxt 
from scipy.optimize import leastsq, curve_fit

threshold = 50
def checkDerivative(pulses):
  """Removes those annoying spike points from the readout"""
  
  for i in range(len(pulses)):
    for j in range(len(pulses[i])-1):
      if (j > 4000):
        if (abs(pulses[i][j+1]-pulses[i][j]) > threshold):
          pulses[i][j+1] = pulses[i][j]

padding = 150 
colors = ['g', 'r', 'c', 'm', 'y', 'k']
labels= ['10_04  COATED PMT #1_RUN_9','10_04 COATED PMT WITH QUARTZ #1_RUN_1']
pulses = [loadtxt(sys.argv[i]) for i in range(1,len(sys.argv))] 
checkDerivative(pulses)
pulse_length = 2520*4
integrated = 0
pedestal = 8127
#Plot Sample Pulse
plt.figure()
samples = np.arange(pulse_length)
plt.plot(samples, pulses[0][0:pulse_length], label='Bare PMT', color='g')
plt.plot(samples, pulses[1][0:pulse_length], label='Coated PMT', color='r')
plt.ylabel('Channel Number')
plt.xlabel('Sample Number (2 Samples/ns)') 
plt.legend(loc=4)
plt.savefig("sample_pulses.pdf", format='pdf')


#Fit function
def gauss(x, *p):
  A, mu, sigma = p
  return A*np.exp(-(x-mu)**2/(2.*sigma**2))

#Integrate Pulses
pulses_integrated = [np.zeros(len(pulses[i])) for i in range(len(pulses))]
plt.figure()
for readout in range(len(pulses)):
  pulses[readout][:] = [channel - pedestal for channel in pulses[readout]]

  total_pulses = len(pulses[readout])/pulse_length
  print 'total_pulses', total_pulses
  for pulse in range(total_pulses):
    pulses_integrated[readout][pulse] += math.fabs(np.sum(pulses[readout][pulse_length*pulse:(pulse+1)*pulse_length-1]))*.5*10**-3
    print 'pulses_integrated[readout][pulse]',pulses_integrated[readout][pulse]
  max_val = max(pulses_integrated[readout])
  min_val = min(pulses_integrated[readout])

  #Histogram data
  hist, bin_edges = np.histogram(pulses_integrated[readout].flatten(), bins=1700, range=(min_val, max_val)) 

  bin_centres = (bin_edges[:-1] + bin_edges[1:])/2


  #p0 is the initial guess
  p0 = [100.0, max_val, 2.5]

  print 'hist', hist
  print 'np.sum(hist)', np.sum(hist)
  print 'bin_centres', bin_centres
 
  #coeff, var_matrix = curve_fit(gauss, bin_centres, hist, p0=p0)
  #hist_fit = gauss(bin_centres, *coeff)

  #A = coeff[0] 
  #mean = coeff[1]  
#  sigma = coeff[2] 
#  print 'For readout number ', readout
#  print 'A', A
  #print 'mean', mean
#  print 'sigma', sigma
  print 'hist data', hist

  width = bin_edges[1]-bin_edges[0]
  #print 'width', width
  plt.bar(bin_edges[:-1], hist, width=width, color=colors[readout], label=labels[readout])
  #plt.plot(bin_edges[:-1], hist_fit, color=colors[readout])
#savetxt("integrated_BARE.dat", pulses_integrated[0])
#savetxt("integrated_BLANK.dat", pulses_integrated[1])
#savetxt("integrated_COATED.dat", pulses_integrated[1])
plt.xlabel("Integrated Channels")
plt.ylabel("Number of Pulses")
#plt.xlim(min(pulses_integrated[0])-10, max(pulses_integrated[0])+10)
plt.xlim(0,1750)
plt.legend()
#plt.title(r'$A = %f\  \mu = %f\  \sigma = %f\ $' %(A,mean,sigma))
plt.savefig("all_hist.pdf", format='pdf')
