import sys
import os
import time 
import ConfigParser #Necessary for config file parsing
from subprocess import call #Used to call DAQ program 
from commands import getstatusoutput #Used to get output of terminal

"""
Purposes of this module: 
  Calls DAQ and Production Module for standard runs.
    1. Parse necessary configuration files
    2. Interactively ask user which PMT serials are being used and other
       hardware related questions for changing some configuration settings.
    3. Save new configuration file in /daq/tmp
    4. Call DAQ script with correct configuration file, run number, and tag.
       Run Number comes from number of files in expected output directory, 
       while tag must be determined by user options. Maybe set two
       possibilities: qa (which handles spe and gain together) and surf
"""

DAQ_BIN = "/home/brandon/coding/vme_daq/bin/daq"
VOLT_UP_BIN = "/home/brandon/coding/vme_daq/bin/volt_up"
VOLT_DOWN_BIN = "/home/brandon/coding/vme_daq/bin/volt_down"
WAIT_TIME = 0 #Wait time in minutes for PMTs/Lamp to heat up
TMP_DIR = "/daq/tmp"
MASTER_CONFIG_LOC = "/daq/conf"

def getConfig(conf_fn): 
  """Returns a ConfigParser object containing data parsed from conf_fn """
  parser = ConfigParser.SafeConfigParser()
  parser.readfp(open(str(conf_fn)))
  return parser

def getRunNumber(mode):
  """Checks folder based on mode to determine number of files within, which is  
     the desired run_number"""
  mode = mode.lower()
  status = 0
  output = ""
  if(mode == 'run'):
    status, output = getstatusoutput("ls /daq/prod | grep run | wc -l")
    output = int(output) 
  elif(mode == 'surf'):
    status, output = getstatusoutput("ls /daq/prod | grep surf | wc -l")
    output = int(output)
  elif(mode == 'misc'):
    status, output = getstatusoutput("ls /daq/test | grep misc | wc -l")
    output = int(output)
  else:
    print "Invalid mode {} for getting run number".format(mode)
    sys.exit(1)
  return output

if __name__ == '__main__':
  usage = "wrapper.py [RUN_MODE (run, surf, misc)] [TAG (pre,post,,etc)]"

  if (len(sys.argv) < 3):
    print usage
  #Mode corresponds to the name of the schema in the database: "run" is for QA
  #testing (i.e. gain or spe), "surf" is for surface consistency testing, and
  #misc is for everything else, while tag is the suffix on the created file name
  #for indexing purposes. Some possible tags are pre and post for all run modes, 
  #and any miscellaneous tags necessary for descriptiveness.

  mode = str(sys.argv[1]).lower()
  tag = str(sys.argv[2])
  run_number = getRunNumber(mode)
  pmt_serials = []
  cfg_parsers = []
  tmp_files = []
  pmt_voltages = [0,0,0,0,0,0]
  #Open Master config files based on mode
  if (mode == 'run'):
    cfg_parsers.append(getConfig("{}/gain.conf".format(MASTER_CONFIG_LOC)))
    cfg_parsers.append(getConfig("{}/spe.conf".format(MASTER_CONFIG_LOC)))
    tmp_files.append(open("{}/gain.conf".format(TMP_DIR), 'w'))
    tmp_files.append(open("{}/spe.conf".format(TMP_DIR), 'w'))
  elif (mode == 'surf'):
    cfg_parsers.append(getConfig("{}/surf.conf".format(MASTER_CONFIG_LOC)))
    tmp_files.append(open("{}/surf_ang000.conf".format(TMP_DIR), 'w'))
    tmp_files.append(open("{}/surf_ang090.conf".format(TMP_DIR), 'w'))
    tmp_files.append(open("{}/surf_ang180.conf".format(TMP_DIR), 'w'))
    tmp_files.append(open("{}/surf_ang270.conf".format(TMP_DIR), 'w'))
  elif (mode == 'misc'):
    cfg_parsers.append(getConfig("{}/misc.conf".format(MASTER_CONFIG_LOC)))
    tmp_files.append(open("{}/misc.conf".format(TMP_DIR), 'w'))
  else:
    print "Invalid mode: {}".format(mode)
    sys.exit(1)

  output_folder = cfg_parsers[0].get('VME', 'output-folder')

  try:
    if not os.path.isdir("{0}/{1}_{2:05d}".format(output_folder, mode, run_number)):
      os.makedirs("{0}/{1}_{2:05d}".format(output_folder, mode, run_number))
  except OSError as exception:
    if exception.errno != errno.EEXIST:
      raise

  for i in range(4):
    pmt_serials.append(cfg_parsers[0].get('Hardware', 'pmt-id-{}'.format(i)))
  for i in range(6):
    pmt_voltages[i] = cfg_parsers[0].get('Hardware', 'pmt-voltages-{}'.format(i))
  num_pulses = int(cfg_parsers[0].get('VME', 'num-pulses'))
  #Get Hardware Info for each Run


  print "Current expected number of pulses is {}.".format(num_pulses)
  print "If correct, press enter. Otherwise, enter correct number of pulses."
  input = raw_input('-->')
  if (input != ''):
    num_pulses = int(input)
  
  print "Current PMT Serial for Channel 0 is {}.".format(pmt_serials[0])
  print "If correct, press enter. Otherwise, enter correct serial."
  input = raw_input('-->')
  if (input != ''):
    pmt_serials[0] = input
  if ("{}".format(pmt_serials[0]) != 'none'):
    input = raw_input("Enter voltage for PMT in channel 0 (HV Ch 0)(default: {})".format(pmt_voltages[0]))
    if (input != ''):
      pmt_voltages[0] = int(input)
  else:
    pmt_voltages[0] = 0

  print "Current PMT Serial for Channel 1 is {}.".format(pmt_serials[1])
  print "If correct, press enter. Otherwise, enter correct serial."
  input = raw_input('-->')
  if (input != ''):
    pmt_serials[1] = input
  if ("{}".format(pmt_serials[1]) != 'none'):
    input = raw_input("Enter voltage for PMT in channel 1 (HV Ch 3)(default: {})".format(pmt_voltages[3]))
    if (input != ''):
      pmt_voltages[3] = int(input)
  else:
    pmt_voltages[3] = 0

  print "Current PMT Serial for Channel 2 is {}.".format(pmt_serials[2])
  print "If correct, press enter. Otherwise, enter correct serial."
  input = raw_input('-->')
  if (input != ''):
    pmt_serials[2] = input
  if ("{}".format(pmt_serials[2]) != 'none'):
    input = raw_input("Enter voltage for PMT in channel 2 (HV Ch 4)(default: {})".format(pmt_voltages[4]))
    if (input != ''):
      pmt_voltages[4] = int(input)
  else:
    pmt_voltages[4] = 0

  print "Current PMT Serial for Channel 3 is {}.".format(pmt_serials[3])
  print "If correct, press enter. Otherwise, enter correct serial."
  input = raw_input('-->')
  if (input != ''):
    pmt_serials[3] = input
  if ("{}".format(pmt_serials[3]) != 'none'):
    input = raw_input("Enter voltage for PMT in channel 3 (HV Ch 5)(default: {})".format(pmt_voltages[5]))
    if (input != ''):
      pmt_voltages[5] = int(input)
  else:
    pmt_voltages[5] = 0

  ####
  #Add more hardware settings here later
  ####

  
  #Set hardware values in config files
  for cfg_parser in cfg_parsers:
    for i in range(6):
      cfg_parser.set('Hardware', 'pmt-voltages-{}'.format(i), str(pmt_voltages[i]))
    for i in range(4):
      cfg_parser.set('Hardware', 'pmt-id-{}'.format(i), str(pmt_serials[i]))
    cfg_parser.set('VME', 'num-pulses', str(num_pulses))

  #Save config files to temp folder for temporary storage before running
  if (mode == 'run' or mode == 'misc'): 
    for i in range(len(tmp_files)):
      cfg_parsers[i].write(tmp_files[i]) 
      tmp_files[i].close()
  elif (mode == 'surf'):
    for i in range(len(tmp_files)):
      cfg_parsers[0].write(tmp_files[i]) 
      tmp_files[i].close()

  input = raw_input("Turn on voltage? [y/n].")
  if (input.lower() == 'y'):
    print "Turning on voltage source..."
    print "{} {} {} {} {} {} {}".format(VOLT_UP_BIN,     pmt_voltages[0], 
                                            pmt_voltages[1], pmt_voltages[2],
                                            pmt_voltages[3], pmt_voltages[4], 
                                            pmt_voltages[5])
    os.system("{} {} {} {} {} {} {}".format(VOLT_UP_BIN,     pmt_voltages[0], 
                                            pmt_voltages[1], pmt_voltages[2],
                                            pmt_voltages[3], pmt_voltages[4], 
                                            pmt_voltages[5]))

  print 'Now wait for {} minutes for heating of lamp and PMT'.format(WAIT_TIME)
  for i in xrange(WAIT_TIME, 0, -1):
    time.sleep(1000)
    sys.stdout.write(str(i)+' ')
    sys.stdout.flush()

  #Make folder to store final data
  i = 0
  for fn in os.listdir("{}".format(TMP_DIR)):
    print "Current filename: {}\n".format(fn)
    tmp_tag = tag
    if (mode == 'surf'):
      raw_input("Press enter when PMT is prepared at {} degrees".format(i*90))
      tmp_tag = "{0}_ang{1:03d}".format(tag, i*90) 
      i = i+1
    elif(fn == "spe.conf"):
      raw_input("Press enter when PMT is prepared for s.p.e. testing")
      tmp_tag = "{}_{}".format(tag, "SPE")
    elif(fn == "gain.conf"):
      raw_input("Press enter when PMT is prepared for gain testing")
      tmp_tag = "{}_{}".format(tag, "gain")
    print "{} -r {} -t {} -m {} {}/{}".format(DAQ_BIN, run_number, tmp_tag, mode, TMP_DIR, fn) 
    os.system("{} -r {} -t {} -m {} {}/{}".format(DAQ_BIN, run_number, tmp_tag, mode, TMP_DIR, fn)) 
    #Remove tmp file after run
    os.system("rm {}/{}".format(TMP_DIR, fn)) 
  input = raw_input("Runs completed. Would you like to turn off voltage? [y/n]")
  if (input.lower() == 'y'): 
    os.system("{}".format(VOLT_DOWN_BIN))
#   call(["./foo"]) #Insert production code here
