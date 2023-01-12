import ijson
import argparse
from math import isnan


import matplotlib.ticker as ticker
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import cm
#from scipy.stats import gaussian_kde
#import seaborn

# My axis should display 10⁻¹ but you can switch to e-notation 1.00e+01
def log_tick_formatter(val, pos=None):
    return f"$10^{{{int(val)}}}$"  # remove int() if you don't use MaxNLocator
    # return f"{10**val:.2e}"      # e-Notation


#fntresholds=[0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0]
fntresholds=np.linspace(0.0, 3.0, num=30)
fn_withthresh=np.zeros(len(fntresholds), dtype=int)

def main():
    parser = argparse.ArgumentParser(
                    prog = 'ProgramName',
                    description = 'What the program does',
                    epilog = 'Text at the bottom of help')
    parser.add_argument('filename')           # positional argument
    parser.add_argument('-fnt', '--falsenegativethreshold')      # option that takes a value
    parser.add_argument('-r', '--regions')      # option that takes a value
    parser.add_argument('-t', '--train')      # option that takes a value
    parser.add_argument('-s', '--single')      # option that takes a value
    parser.add_argument('-dse3d', '--designspaceexploration3d')      # option that takes a value
    parser.add_argument('-dse2dr', '--designspaceexploration2dregions')
    parser.add_argument('-fftT', '--fftcheckperiod')      # option that takes a value
    parser.add_argument('-fftsize', '--fftsize')      # option that takes a value

    args=parser.parse_args()
    with open(args.filename, "rb") as f:
        regions_x=[]
        regions_fp_rate=[]
        regions_fn_rate=[]
        regions_trainIterations=[]

        region_fp_rate=[]
        region_fn_rate=[]
        region_trainIterations=[]
        region_relerrmean=[]
        region_relerrvariance=[]

        
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

            if (args.fftcheckperiod is not None and args.fftsize is not None):
                fftperiodicity=int(args.fftcheckperiod)
                fftsize=int(args.fftsize)
                incr=(192)*fftsize*(1+fftsize/fftperiodicity) #in order to take into account instruction duplication, which timing overhead has been taken into account in timing analysis
                total_neg=total_neg+incr
                tn=tn+incr

            fn_rate=100*fn/total_neg
            fp_rate=100*fp/total_pos

            relErrNum = np.asarray(record["relerr"], dtype=np.uint32)
            relErrNum = relErrNum.view(dtype=np.float32)

            relErrMean=np.mean(relErrNum)
            relErrVar=np.var(relErrNum)
            relErrMax = np.max(relErrNum)

            """
            precision=tp/total_pos
            scaling_factor=total_pos/total_neg
            recall=tp/(tp+(fn*scaling_factor))
            accuracy=(tp+tn*scaling_factor)/(total_pos+total_neg*scaling_factor)"""

            #total_neg_tresh_1=tn+len(relErrNum)
            #if (total_neg!=total_neg_tresh_1):
            #    print("ERROR! total_neg!=total_neg_tresh")

            #erthr=0
            #fn_withthresh_1=0
            #cont=True
            #if (fp_rate<=1):
            #    while(cont):
            #        cont=False
            #        for er in relErrNum:
            #            if (isnan(er)):
            #                print("ISNAN")
            #            else:
            #                if (er>erthr):
            #                    fn_withthresh_1=fn_withthresh_1+1
            #            if (fn_withthresh_1/total_neg_tresh_1>0.01):
            #                cont=True
            #                erthr=erthr+0.025
            #                break
            #if (fp_rate<=1):  
            #    print(f"regions {regions}, trainIterations {trainIterations}, testIterations {testIterations}\ntot pos {total_pos}, tp {tp}, fp {fp} fp rate {fp_rate} | tot neg {total_neg}, tn {tn}, fn {fn} fn rate {fn_rate} | rel err mean: {relErrMean} max: {relErrMax} var: {relErrVar} accepted rel err threshold to get under 1% fn: fn:{fn_withthresh_1/total_neg_tresh_1}, thresh: {erthr}\n") #| precision {precision}, recall {recall}, accuracy {accuracy}\n")
            #else:
            print(f"regions {regions}, trainIterations {trainIterations}, testIterations {testIterations}\ntot pos {total_pos}, tp {tp}, fp {fp} fp rate {fp_rate} | tot neg {total_neg}, tn {tn}, fn {fn} fn rate {fn_rate} | rel err mean: {relErrMean} max: {relErrMax} var: {relErrVar}\n") #| precision {precision}, recall {recall}, accuracy {accuracy}\n")
            #for charts generation
            regions_x.append(regions)
            regions_trainIterations.append(trainIterations)
            fp_r_not_clamped=fp*100/total_pos
            fp_r_clamped= 4 if fp_r_not_clamped > 4 else fp_r_not_clamped
            regions_fp_rate.append(fp_r_clamped)
            regions_fn_rate.append(fn*100/total_neg)

            if (args.designspaceexploration2dregions is not None and (int(args.designspaceexploration2dregions)==regions) or args.single is not None):
                region_fp_rate.append(fp_r_not_clamped)
                region_trainIterations.append(trainIterations)
                region_relerrmean.append(relErrMean)
                region_relerrvariance.append(relErrVar)
                region_fn_rate.append(fn_rate)

            if (args.falsenegativethreshold is not None and (args.single is not None or trainIterations==int(args.train) and regions==int(args.regions))):
                #relErrNum = np.asarray(record["relerr"], dtype=np.uint32)
                #relErrNum = relErrNum.view(dtype=np.float32)
                fn_withthresh[0]=fn
                total_neg_tresh=tn+len(relErrNum)
                if (total_neg!=total_neg_tresh):
                    print("ERROR! total_neg!=total_neg_tresh")

                for er in relErrNum:
                    for thri in range(1, len(fntresholds)):
                        if (isnan(er)):
                            print("ISNAN")
                        else:
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
    
    if (args.falsenegativethreshold is not None):
        fig, ax = plt.subplots()
        relerrarr=(np.array(fn_withthresh)/np.array(total_neg))*np.array(100)
        ax.plot(fntresholds, (relerrarr))
        plt.gcf().subplots_adjust(left=0.2)
        ax.yaxis.set_major_formatter(ticker.PercentFormatter())
        ax.set_yscale('log')
        
        plt.xlabel("Accepted relative error threshold")
        plt.ylabel("False negatives rate")
        
        plt.show()

    if (args.designspaceexploration2dregions is not None):
        fig, ax = plt.subplots()
        #region_2d_trainIterations=np.array(region_trainIterations)
        region_2d_fp_rate=np.array(region_fp_rate)
        ax.plot(np.log2((np.array(regions_trainIterations)/100)).astype(int), region_2d_fp_rate)
        plt.gcf().subplots_adjust(left=0.2)
        ax.yaxis.set_major_formatter(ticker.PercentFormatter())
        ax.set_yscale('log')
        ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x,pos: ('{:d}'.format(int(pow(2, x))*100))))

        plt.xlabel("Training iterations")
        plt.ylabel("False positives rate")
        
        plt.show()

        fig, ax = plt.subplots()
        #region_2d_trainIterations=np.array(region_trainIterations)
        region_2d_fn_rate=np.array(region_fn_rate)
        ax.plot(np.log2((np.array(regions_trainIterations)/100)).astype(int), region_2d_fn_rate)
        plt.gcf().subplots_adjust(left=0.2)
        ax.yaxis.set_major_formatter(ticker.PercentFormatter())
        #ax.set_yscale('log')
        ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x,pos: ('{:d}'.format(int(pow(2, x))*100))))

        plt.xlabel("Training iterations")
        plt.ylabel("False negatives rate")
        
        plt.show()

        fig, ax = plt.subplots()
        #region_2d_trainIterations=np.array(region_trainIterations)
        region_2d_relerr_mean=np.array(region_relerrmean)
        ax.plot(np.log2((np.array(regions_trainIterations)/100)).astype(int), region_2d_relerr_mean)
        plt.gcf().subplots_adjust(left=0.2)
        ax.yaxis.set_major_formatter(ticker.PercentFormatter())
        #ax.set_yscale('log')
        ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x,pos: ('{:d}'.format(int(pow(2, x))*100))))

        plt.xlabel("Training iterations")
        plt.ylabel("False negatives relative error mean")
        
        plt.show()

        fig, ax = plt.subplots()
        #region_2d_trainIterations=np.array(region_trainIterations)
        region_2d_relerr_variance=np.array(region_relerrvariance)
        ax.plot(np.log2((np.array(regions_trainIterations)/100)).astype(int), region_2d_relerr_variance)
        plt.gcf().subplots_adjust(left=0.2)
        ax.yaxis.set_major_formatter(ticker.PercentFormatter())
        #ax.set_yscale('log')
        ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x,pos: ('{:d}'.format(int(pow(2, x))*100))))

        plt.xlabel("Training iterations")
        plt.ylabel("False negatives relative error variance")
        
        plt.show()


    if (args.designspaceexploration3d is not None):
        fig, ax = plt.subplots(subplot_kw={"projection": "3d"})
        #X,Y=np.meshgrid(regions_x, regions_trainIterations)
        #Z=np.array(regions_fn_rate)
        surf=ax.plot_trisurf(np.log2(regions_x).astype(int), np.log2((np.array(regions_trainIterations)/100)).astype(int), regions_fn_rate, cmap=cm.coolwarm, linewidth=0, antialiased=False)
        fig.colorbar(surf, shrink=0.5, aspect=5, pad=0.2)
        #ax.set_zlabel('Log(2, Regions)')
        ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x,pos: ('{:d}'.format(int(pow(2, x))))))
        ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda y,pos: ('{:d}'.format(int(pow(2, y))*100))))
        ax.zaxis.set_major_formatter(ticker.PercentFormatter())

        ax.zaxis.set_tick_params(pad=0.2)



        ax.set_xlabel('Regions')
        ax.set_ylabel('Training iterations')
        ax.set_zlabel('False negatives rate')



        fig, ax = plt.subplots(subplot_kw={"projection": "3d"})
        #X,Y=np.meshgrid(regions_x, regions_trainIterations)
        #Z=np.array(regions_fn_rate)
        surf=ax.plot_trisurf(np.log2(regions_x).astype(int), np.log2((np.array(regions_trainIterations)/100)).astype(int), regions_fp_rate, cmap=cm.coolwarm, linewidth=0, antialiased=False)
        fig.colorbar(surf, shrink=0.5, aspect=5, pad=0.2)
        #ax.set_zlabel('Log(2, Regions)'
        ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x,pos: ('{:d}'.format(int(pow(2, x))))))
        ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda y,pos: ('{:d}'.format(int(pow(2, y))*100))))
        ax.zaxis.set_major_formatter(ticker.PercentFormatter())
    #  fig.colorbar(surf, orientation="vertical", pad=0.2)


        ax.zaxis.set_tick_params(pad=0.2)


        ax.set_xlabel('Regions')
        ax.set_ylabel('Training iterations')
        ax.set_zlabel('False positives rate')

        plt.show()


if __name__ == "__main__":
    main()