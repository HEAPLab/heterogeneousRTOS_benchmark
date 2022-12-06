import subprocess
import numpy
import os
import math
import queue
import argparse

def main():

    parser = argparse.ArgumentParser(
                    prog = 'ProgramName',
                    description = 'What the program does',
                    epilog = 'Text at the bottom of help')
    
    parser.add_argument('binary')           # positional argument
    parser.add_argument('filename')           # positional argument
    args=parser.parse_args()

    try:
        os.remove(args.filename)
    except Exception:
        pass


    fl=open(args.filename, 'wb')

    expRegion=7
    #expRegion=2
    expRegionMultiplier=1

    expTrain=10
    #expTrain=2
    expTrainMultiplier=100

    executions=50000

    workers_num=8

    #procArr = numpy.full(-1, arrSize)
    procArr=queue.Queue(workers_num)
    fl.write(b'[')

    first=True
    #for t in range(1, expTrain+1, 1):
    #    for r in range(1, expRegion+1, 1):
    reg=16
    train=32000

    procArr.put(subprocess.Popen([args.binary, "-r", str(int(reg)), "-t", str(int(train)), "-e", str(int(executions))], shell=True, stdout=subprocess.PIPE))
                
    if (procArr.full()):
        if (first):
            first=False
        else:
            fl.write(b',')
        fl.write(procArr.get().stdout.read())

    while (not procArr.empty()):
        if (first):
            first=False
        else:
            fl.write(b',')
        fl.write(procArr.get().stdout.read())
    
    fl.write(b']')
    fl.flush()
    fl.close()


""" def maina():
    os.remove('pippo.txt')
    fl=open('pippo.txt', 'a')
    fl.write('[')
    fl.flush()
    faultdet=subprocess.Popen('%userprofile%/eclipse-workspace/faultdetector_benchmark/Debug/faultdetector_benchmark.exe', shell=True, stdout=fl)
    subprocess.Popen.wait(faultdet)
    fl.write(',')
    fl.flush()
    faultdet=subprocess.Popen('%userprofile%/eclipse-workspace/faultdetector_benchmark/Debug/faultdetector_benchmark.exe', shell=True, stdout=fl)
    subprocess.Popen.wait(faultdet)
    fl.write(']')
    fl.flush()
    fl.close() """

if __name__ == "__main__":
    main()