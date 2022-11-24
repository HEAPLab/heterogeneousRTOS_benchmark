import ijson
import argparse


import matplotlib.ticker as ticker
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import cm
#from scipy.stats import gaussian_kde
#import seaborn

fntresholds=[0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0]
fn_withthresh=np.zeros(len(fntresholds), dtype=int)

def main():
    parser = argparse.ArgumentParser(
                    prog = 'ProgramName',
                    description = 'What the program does',
                    epilog = 'Text at the bottom of help')
    parser.add_argument('filename')           # positional argument
    args=parser.parse_args()
    with open(args.filename, "rb") as f:
        regions_x=[]
        regions_fp_rate=[]
        regions_fn_rate=[]
        regions_trainIterations=[]
        for record in ijson.items(f, "item"):

            regions=record["regions"]

            trainIterations=record["trainIterations"]
            testIterations=record["testIterations"]

            total_pos=int(record["total_pos"])
            fp=int(record["false_pos"])
            tp=int(record["true_pos"])

            total_neg=int(record["true_neg"])+int(record["false_neg"])
            tn=int(record["true_neg"])
            fn=int(record["false_neg"])
            fn_withthresh[0]=fn_withthresh[0]+int(record["false_neg"])

            precision=tp/total_pos
            scaling_factor=total_pos/total_neg
            recall=tp/(tp+(fn*scaling_factor))
            accuracy=(tp+tn*scaling_factor)/(total_pos+total_neg*scaling_factor)

            print(f"regions {regions}, trainIterations {trainIterations}, testIterations {testIterations}\ntot pos {total_pos}, tp {tp}, fp {fp} | tot neg {total_neg}, tn {tn}, fn {fn} | precision {precision}, recall {recall}, accuracy {accuracy}\n")
            
            #for charts generation
            regions_x.append(regions)
            regions_trainIterations.append(trainIterations)
            regions_fp_rate.append(fp/total_pos)
            regions_fn_rate.append(fn/total_neg)

            if (trainIterations==51200 and regions==8):
                relErrNum = np.asarray(record["relerr"], dtype=float)
                fn_withthresh[0]=fn
                total_neg_tresh=tn+len(relErrNum)
                if (total_neg!=total_neg_tresh):
                    print("ERROR! total_neg!=total_neg_tresh")

                for er in relErrNum:
                    for thri in range(1, len(fntresholds)):
                        if (er>fntresholds[thri]):
                            fn_withthresh[thri]=fn_withthresh[thri]+1

                """
                print(f"TOTAL {total_neg_tresh} | ")
                for thri in range(len(fntresholds)):
                    print(f"THRESH: {fntresholds[thri]} FN: {fn_withthresh[thri]}")
                """
            #print(len(relErrNum))
            #minErr=np.floor(np.amin(relErrNum))
            #maxErr=np.ceil(np.amax(relErrNum))

            #try:
                #density = gaussian_kde(relErrNum)
                #x_vals = np.linspace(minErr,maxErr,200)
                #density.covariance_factor = lambda : .5 #Smoothing parameter
                        #sns.kdeplot(relErrNum, bw_adjust=.25)


            #if np.cov(relErrNum)==0:
            #    print(f"covariance 0\n, relErr is {relErrNum[0]}")    
            #else:

                #hi=seaborn.histplot(relErrNum, stat='probability', bins=20)
                #hi.set(xlabel="False negatives relative error")
                #plt.show()


                #plt.plot(x_vals, density(x_vals))
            #except np.linalg.LinAlgError:
            #    print("singular matrix\n")
            

    #plt.figure(1)
    #plt.plot(regions_x, regions_fp_rate, '.-b', label="False positive rate")
    #plt.plot(regions_x, regions_fn_rate, '.-r', label="False negative rate")
    #plt.xscale('log', base=2)
    #plt.xlabel("number of regions")
    #plt.legend()
    #plt.show()

    """fig = plt.figure()
    ax = fig.gca(projection='3d')

    ax.plot_trisurf(regions_x, regions_trainIterations, linewidth=0.2, antialiased=True)

    plt.show()"""

    fig, ax = plt.subplots(subplot_kw={"projection": "3d"})
    #X,Y=np.meshgrid(regions_x, regions_trainIterations)
    #Z=np.array(regions_fn_rate)
    surf=ax.plot_trisurf(np.log2(regions_x).astype(int), np.log2((np.array(regions_trainIterations)/100)).astype(int), regions_fn_rate, cmap=cm.coolwarm, linewidth=0, antialiased=False)
    fig.colorbar(surf, shrink=0.5, aspect=5)
    #ax.set_zlabel('Log(2, Regions)')
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x,pos: ('{:d}'.format(int(pow(2, x))))))
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda y,pos: ('{:d}'.format(int(pow(2, y))*100))))
    ax.zaxis.set_major_formatter(ticker.PercentFormatter())

    ax.set_xlabel('Regions')
    ax.set_ylabel('Training iterations')
    ax.set_zlabel('False negatives rate')

    fig, ax = plt.subplots(subplot_kw={"projection": "3d"})
    #X,Y=np.meshgrid(regions_x, regions_trainIterations)
    #Z=np.array(regions_fn_rate)
    surf=ax.plot_trisurf(np.log2(regions_x).astype(int), np.log2((np.array(regions_trainIterations)/100)).astype(int), regions_fp_rate, cmap=cm.coolwarm, linewidth=0, antialiased=False)
    fig.colorbar(surf, shrink=0.5, aspect=5)
    #ax.set_zlabel('Log(2, Regions)'
    ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x,pos: ('{:d}'.format(int(pow(2, x))))))
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda y,pos: ('{:d}'.format(int(pow(2, y))*100))))
    ax.zaxis.set_major_formatter(ticker.PercentFormatter())

    ax.set_xlabel('Regions')
    ax.set_ylabel('Training iterations')
    ax.set_zlabel('False positives rate')

    fig, ax = plt.subplots()
    ax.plot(fntresholds, np.array(fn_withthresh)/np.array(total_neg_tresh), '.-b')
    ax.yaxis.set_major_formatter(ticker.PercentFormatter())
    plt.xlabel("Relative error threshold")
    plt.ylabel("False negatives")
    
    plt.show()

"""

    ax = df.plot(x="Regions", y="False positives rate", legend=False)
    ax2 = ax.twinx()
    df.plot(x="Regions", y="False negatives rate", ax=ax2, legend=False, color="r")
    ax.figure.legend()
    #ax2.set_yscale('log', value'log', basey=2)
    plt.xscale('log', base=2)
    plt.show()
"""

if __name__ == "__main__":
    main()