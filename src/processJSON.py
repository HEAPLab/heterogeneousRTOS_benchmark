import ijson
import argparse

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
#from scipy.stats import gaussian_kde
import seaborn

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
        for record in ijson.items(f, "item"):
            total_pos=0
            tp=0
            fp=0

            total_neg=0
            tn=0
            fn=0

            regions=record["regions"]
            regions_x.append(regions)
            trainIterations=record["trainIterations"]
            testIterations=record["testIterations"]

            total_pos=int(record["total_pos"])
            fp=fp+int(record["false_pos"])
            tp=tp+int(record["true_pos"])

            total_neg=int(record["true_neg"])+int(record["false_neg"])
            tn=tn+int(record["true_neg"])
            fn=fn+int(record["false_neg"])
            precision=tp/total_pos
            scaling_factor=total_pos/total_neg
            recall=tp/(tp+(fn*scaling_factor))
            accuracy=(tp+tn*scaling_factor)/(total_pos+total_neg*scaling_factor)

            regions_fp_rate.append(fp/total_pos)
            regions_fn_rate.append(fn/total_neg)


            relErrNum = np.asarray(record["relerr"], dtype=float)
            print(len(relErrNum))
            minErr=np.floor(np.amin(relErrNum))
            maxErr=np.ceil(np.amax(relErrNum))

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
            
            print(f"regions {regions}, trainIterations {trainIterations}, testIterations {testIterations}\ntot pos {total_pos}, tp {tp}, fp {fp} | tot neg {total_neg}, tn {tn}, fn {fn} | precision {precision}, recall {recall}, accuracy {accuracy}\n")

    df=pd.DataFrame({
        "Regions": regions_x,
        "False positives rate": regions_fp_rate,
        "False negatives rate": regions_fn_rate
    })

    ax = df.plot(x="Regions", y="False positives rate", legend=False)
    ax2 = ax.twinx()
    df.plot(x="Regions", y="False negatives rate", ax=ax2, legend=False, color="r")
    ax.figure.legend()
    #ax2.set_yscale('log', value'log', basey=2)
    plt.xscale('log', base=2)
    plt.show()

if __name__ == "__main__":
    main()