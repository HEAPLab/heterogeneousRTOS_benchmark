import ijson
import argparse

regions_list=[]
trainIterations_list=[]
testIterations_list=[]
totalPos_list=[]
fp_list=[]
tp_list=[]
totalNeg_list=[]
tn_list=[]
fn_list=[]


def main():
    parser = argparse.ArgumentParser(
                    prog = 'ProgramName',
                    description = 'What the program does',
                    epilog = 'Text at the bottom of help')
    parser.add_argument('filename1')           # positional argument
    parser.add_argument('filename2')           # positional argument
    args=parser.parse_args()
    with open(args.filename1, "rb") as f1:
            for record in ijson.items(f1, "item"):

                regions=int(record["regions"])

                trainIterations=int(record["trainIterations"])
                testIterations=int(record["testIterations"])

                total_pos=int(record["total_pos"])
                fp=int(record["false_pos"])
                tp=int(record["true_pos"])

                total_neg=int(record["true_neg"])+int(record["false_neg"])
                tn=int(record["true_neg"])
                fn=int(record["false_neg"])

                """
                precision=tp/total_pos
                scaling_factor=total_pos/total_neg
                recall=tp/(tp+(fn*scaling_factor))
                accuracy=(tp+tn*scaling_factor)/(total_pos+total_neg*scaling_factor)
                """

                regions_list.append(regions)
                trainIterations_list.append(trainIterations)
                testIterations_list.append(testIterations)
                totalPos_list.append(total_pos)
                fp_list.append(fp)
                tp_list.append(tp)
                totalNeg_list.append(total_neg)
                tn_list.append(tn)
                fn_list.append(fn)


                #print(f"regions {regions}, trainIterations {trainIterations}, testIterations {testIterations}\ntot pos {total_pos}, tp {tp}, fp {fp} | tot neg {total_neg}, tn {tn}, fn {fn}\n")

    with open(args.filename2, "rb") as f2:
            i=0
            for record in ijson.items(f2, "item"):

                regions=int(record["regions"])

                trainIterations=int(record["trainIterations"])
                testIterations=int(record["testIterations"])

                total_pos=int(record["total_pos"])
                fp=int(record["false_pos"])
                tp=int(record["true_pos"])

                total_neg=int(record["true_neg"])+int(record["false_neg"])
                tn=int(record["true_neg"])
                fn=int(record["false_neg"])

                """
                precision=tp/total_pos
                scaling_factor=total_pos/total_neg
                recall=tp/(tp+(fn*scaling_factor))
                accuracy=(tp+tn*scaling_factor)/(total_pos+total_neg*scaling_factor)
                """

                if (regions_list[i]!=regions or trainIterations_list[i]!=trainIterations or testIterations_list[i]!=testIterations):
                    print("ERROR")
                else:
                    fn_rate_new=fn/total_neg
                    fp_rate_new=fp/total_pos

                    fn_rate_old=fn_list[i]/totalNeg_list[i]
                    fp_rate_old=fp_list[i]/totalPos_list[i]

                    if (total_neg !=  totalNeg_list[i] or total_pos!=totalPos_list[i]):
                        print("WARNING: DIFFERENT TOTALS\n")

                    if (fp_rate_new < fp_rate_old):
                        print("FILE 2 BETTER FP RATE")
                    else:
                        if (fp_rate_new > fp_rate_old):
                            print("FILE 1 BETTER FP RATE")
                        else:
                            print("EQUAL FP")

                    print(f"F1 rate {fp_rate_old} F2 rate {fp_rate_new}")

                    if (fn_rate_new < fn_rate_old):
                        print("FILE 2 BETTER FN RATE")
                    else:
                        if (fn_rate_new > fn_rate_old):
                            print("FILE 1 BETTER FN RATE")
                        else:
                            print("EQUAL FP")
                    
                    print(f"F1 rate {fn_rate_old} F2 rate {fn_rate_new}")

                    print(f"regions: {regions} trainIterations: {trainIterations}")

                    totalPos_list.append(total_pos)
                    fp_list.append(fp)
                    tp_list.append(tp)
                    totalNeg_list.append(total_neg)
                    tn_list.append(tn)
                    fn_list.append(fn)


                #print(f"regions {regions}, trainIterations {trainIterations}, testIterations {testIterations}\ntot pos {total_pos}, tp {tp}, fp {fp} | tot neg {total_neg}, tn {tn}, fn {fn}\n")

                i=i+1


if __name__ == "__main__":
    main()