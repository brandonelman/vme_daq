import sys, os
import matplotlib
import math
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
from scipy import loadtxt, savetxt 
from scipy.optimize import leastsq, curve_fit

bins = 100
threshold = 50 
pulse_length = 2520*4
upperlim = 1750
lowerlim = 1400 
def checkDerivative(pulses):
  """Removes those annoying spike points from the readout"""
  print 'len(pulses)',len(pulses)

  for readout in range(len(pulses)):
    print 'len(pulses[readout])/pulse_length', len(pulses[readout])/pulse_length
    for pulseNum in range(len(pulses[readout])/pulse_length):
      for channel in range(pulse_length/2+pulseNum*pulse_length-200, pulse_length*(pulseNum+1)-1): 
        result = (pulses[readout][channel+1]-pulses[readout][channel])
        if (abs(result) > threshold):
          pulses[readout][channel+1] = pulses[readout][channel]
        
colors = ['g', 'r', 'c', 'm', 'y', 'k']
labels=['coated pmt', 'coated pmt']
pulses = [loadtxt(sys.argv[i]) for i in range(1,len(sys.argv))] 

checkDerivative(pulses)

integrated = 0
pedestal = 8127.821289

#Plot Sample Pulse
plt.figure()
samples = np.arange(pulse_length)
if (len(sys.argv) == 3):
  plt.plot(samples, pulses[0][0:pulse_length], label=labels[0], color='g')
  plt.plot(samples, pulses[1][0:pulse_length], label=labels[1], color='r')
  plt.plot(samples, pulses[0][pulse_length*10:pulse_length*11], label=labels[0], color='g')
  plt.plot(samples, pulses[1][pulse_length*20:pulse_length*21], label=labels[1], color='r')
  plt.legend(loc=4)
if (len(sys.argv) == 2):
  plt.plot(samples, pulses[0][0:pulse_length], color='g')
  plt.plot(samples, pulses[0][pulse_length*100:pulse_length*101], color='r')
  plt.plot(samples, pulses[0][pulse_length*2:pulse_length*3], color='y')
  plt.plot(samples, pulses[0][pulse_length*292:pulse_length*293], color='c')
  plt.plot(samples, pulses[0][pulse_length*293:pulse_length*294], color='m')
  plt.plot(samples, pulses[0][pulse_length*291:pulse_length*292], color='b')
  #plt.plot(samples, pulses[0][pulse_length*4998:pulse_length*4999], color='k')
  plt.title(labels[0])
plt.ylabel('Channel Number')
plt.xlabel('Sample Number (2 Samples/ns)') 
plt.savefig("sample_pulses.pdf", format='pdf')

#Fit function
def gauss(x, *p):
  A, mu, sigma = p
  return A*np.exp(-(x-mu)**2/(2.*sigma**2))

#Integrate Pulses
pulses_integrated = []

plt.figure()
for readout in range(len(pulses)):
  pulses[readout][:] = [channel - pedestal for channel in pulses[readout]]
  readout_integrated = []

  overflow = 0
  underflow = 0
  total_pulses = len(pulses[readout])/pulse_length
  print 'total_pulses', total_pulses
  for pulse in range(total_pulses):
    integrated_pulse = abs(np.sum(pulses[readout][pulse_length*pulse:(pulse+1)*pulse_length-2501]))*.5*10**-3
    if (integrated_pulse < 1275):
      print 'integrated_pulse', integrated_pulse, 'pulse #', pulse
    if (integrated_pulse > upperlim):
      overflow += 1
      continue #Skips overflow pulses
    if (integrated_pulse < lowerlim):
      underflow += 1
      continue #Skips pulses near 0 
    readout_integrated.append(integrated_pulse)

  pulses_integrated.append(readout_integrated)
  print 'pulses_integrated[readout]', pulses_integrated[readout]
  print 'pulses_integrated', pulses_integrated
  
  max_val = max(pulses_integrated[readout])
  min_val = min(pulses_integrated[readout])

  print 'min_val', min_val
  print 'max_val', max_val
  #Histogram data
  hist, bin_edges = np.histogram(pulses_integrated[readout], bins=bins, range=(min_val, max_val)) 

  bin_centres = (bin_edges[:-1] + bin_edges[1:])/2


  #p0 is the initial guess
  p0 = [150, 1500, 2.0]
  print 'hist', hist
  print 'np.sum(hist)', np.sum(hist)
  print 'bin_centres', bin_centres
 
  coeff, var_matrix = curve_fit(gauss, bin_centres, hist, p0=p0)
  hist_fit = gauss(bin_centres, *coeff)
  A = coeff[0] 
  mean = coeff[1]  
  sigma = coeff[2] 
  print 'For readout number ', readout
  print 'A', A
  print 'mean', mean
  print 'sigma', sigma
  print 'underflow', underflow
  print 'overflow', overflow
  if (len(sys.argv) == 2):
    #plt.title(r'$A = %f\  \mu = %f\  \sigma = %f\ $' %(A,mean,sigma))
    plt.title('underflow = {}, overflow = {}'.format(underflow, overflow))


  width = bin_edges[1]-bin_edges[0]
  if (len(sys.argv) == 2):
    plt.bar(bin_edges[:-1], hist, width=width, color=colors[readout])
  else: 
    plt.bar(bin_edges[:-1], hist, width=width, color=colors[readout], label=labels[readout])
    plt.legend()
  plt.plot(bin_edges[:-1], hist_fit, color=colors[readout])

plt.xlabel("Integrated Channels")
plt.ylabel("Number of Pulses")
plt.xlim(lowerlim, upperlim)
plt.savefig("all_hist.pdf", format='pdf')
