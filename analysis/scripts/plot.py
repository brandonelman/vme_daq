import sys, os
import matplotlib
import math
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
from scipy import loadtxt, savetxt 
from scipy.optimize import leastsq, curve_fit

usage = "USAGE: python plot.py [channels to write over] [file_name] [desired_title_of_plot].\n Can Repeat for multiple file names over 4 channels as follows: \npython plot.py 4 [fn1] [title1] [fn2] [title2]" 
if (len(sys.argv) < 3):
  print usage 
  exit(1)
if (len(sys.argv) % 2 != 0):
  print "You have entered an incorret number of arguements!"
  exit(1)

bins = 100
threshold = 1000  #For points "a" and "b", if |a-b| > threshold, cut these points.


def checkDerivative(pulses):
  """Removes those annoying spike points from the readout"""
  for readout in range(len(pulses)):
    for pulseNum in range(len(pulses[readout])/pulse_length):
      for channel in range(pulse_length/2+pulseNum*pulse_length-200, pulse_length*(pulseNum+1)-1): 
        result = (pulses[readout][channel+1]-pulses[readout][channel])
        if (abs(result) > threshold):
          pulses[readout][channel+1] = pulses[readout][channel]
      for channel in range(pulse_length*pulseNum, pulse_length*pulseNum+1000): 
        result = (pulses[readout][channel+1]-pulses[readout][channel])
        if (abs(result) > threshold):
          pulses[readout][channel+1] = pulses[readout][channel]
        
colors = ['g', 'r', 'c', 'm', 'y', 'k']
channels = int(sys.argv[1])
pulses = [loadtxt(sys.argv[i]) for i in range(2, len(sys.argv), 2)]
labels = [str(sys.argv[i]) for i in range(3,len(sys.argv),2)]

#Only change the four following values in the if statements below!
left_int_padding = 0 #Distance from beginning of pulse to begin integration 
right_int_padding = 0 #Distance from end of pulse to stop integration
upperlim = 0 #Values higher than this are considered overflow and removed
lowerlim = 0 #Values lower than this are considered error runs and are removed.
pulse_length = 2520*channels
if (channels == 1):
  upperlim = 2400 #These values are prob wrong 
  lowerlim = 2000
  left_int_padding = 0 
  right_int_padding = 0 #cuts pulse short already so dont need to
elif (channels == 2):
  upperlim = 10000  
  lowerlim = 8300 
  left_int_padding = 0   
  right_int_padding = 0 
  checkDerivative(pulses)
elif (channels == 4):
  upperlim = 3000  
  lowerlim = 500 
  left_int_padding = 0 
  right_int_padding = 4000 
  checkDerivative(pulses)
else: 
  print "Improper number of channels! Please try again."
  exit(1)
integrated = 0
pedestal = 8127.821289

#Plot Sample Pulse
samples = np.arange(pulse_length-left_int_padding-right_int_padding)
for readout in range(len(pulses)):
  pulse_in_plot = 0
  plt.figure()
  for pulse in range(len(pulses[readout])/pulse_length):
    if pulse % 1 == 0:
      pulse_in_plot += 1
      if (pulses[readout][pulse*pulse_length] > 7000 and pulses[readout][pulse*pulse_length] < 8200):
        plt.plot(samples, pulses[readout][pulse*pulse_length+left_int_padding:(pulse+1)*pulse_length-right_int_padding], label=labels[readout])
  plt.ylabel('Channel Number')
  plt.xlabel('Sample Number (2 Samples/ns)') 
  plt.title("{}: {} pulses in plot".format(labels[readout], pulse_in_plot))
  plt.savefig("{}_pulse.png".format(labels[readout]), format='png')

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

  for pulse in range(total_pulses):
    integrated_pulse = abs(np.sum(pulses[readout][pulse_length*pulse+left_int_padding:(pulse+1)*pulse_length-right_int_padding]))*.5*10**-3
    print integrated_pulse
    if (integrated_pulse > upperlim):
      overflow += 1
      continue #Skips overflow pulses
    if (integrated_pulse < lowerlim):
      underflow += 1
      continue #Skips pulses near 0 
    readout_integrated.append(integrated_pulse)

  pulses_integrated.append(readout_integrated)
  max_val = max(pulses_integrated[readout])
  min_val = min(pulses_integrated[readout])
  print 'min_val', min_val
  print 'max_val', max_val
  print 'overflow', overflow
  print 'underflow', underflow

  #Histogram data
  hist, bin_edges = np.histogram(pulses_integrated[readout], bins=bins, range=(min_val, max_val)) 

  bin_centres = (bin_edges[:-1] + bin_edges[1:])/2
  #print 'max(hist)', max(hist)
  #print 'bin_centres', bin_centres[np.argmax(hist)]

  #p0 is the initial guess
  p0 = [max(hist), bin_centres[np.argmax(hist)], 1.5]
  print 'Histogram Data:', hist
  print 'Total elements in hist:', np.sum(hist)
  print 'bin centres', bin_centres
 
  coeff, var_matrix = curve_fit(gauss, bin_centres, hist, p0=p0)
  hist_fit = gauss(bin_centres, *coeff)
  A = coeff[0] 
  mean = coeff[1]  
  sigma = coeff[2] 
  print 'For readout number ', readout, ':'
  print 'A = ', A
  print 'mean = ', mean
  print 'sigma = ', sigma

  #plt.title('{}: underflow = {}, overflow = {}'.format(label[readout], underflow, overflow))

  width = bin_edges[1]-bin_edges[0]
  plt.bar(bin_edges[:-1], hist, width=width, color=colors[readout],label=r'%s: $\mu = %f\ \sigma = %f\ $'%(labels[readout], mean,sigma))
  plt.plot(bin_edges[:-1], hist_fit, color=colors[readout])

plt.legend()
plt.xlabel("Integrated Channels (arbitrary units)")
plt.ylabel("Number of Pulses")
plt.xlim(lowerlim, upperlim)
plt.savefig("all_hists.png", format='png')
