from sys import exit, argv
import parse_utils
import datetime

class TimeDiff():
    def __init__(self, startTime, endTime):
        self.startTime = startTime
        self.endTime = endTime

    def getTimeDiff(self):
        start = datetime.datetime.strptime(self.startTime, "%H:%M:%S.%f")
        end = datetime.datetime.strptime(self.endTime, "%H:%M:%S.%f")
        diff = end - start
        return diff.total_seconds()

class task():
    def __init__(self, arrivalTime):
        self.arrivalTime = arrivalTime
        self.startTime = 0
        self.finishTime = 0
        self.runTimes = []
        self.waitTimes = []
        self.turnAroundTime = 0 # from arrive time to complete the task
        self.totalRunTime = 0
        self.totalWaitTime = 0

    def addRunTime(self, startTime, endTime):
        self.runTimes.append(TimeDiff(startTime, endTime))
        if len(self.runTimes) > 1:
            #waiting time is drived from the new run time with relative to the previous run time
            # wait time = start ( previous run end time) , end ( new run start time)
            waitingTime = TimeDiff(self.runTimes[-2].endTime , self.runTimes[-1].startTime)
            self.waitTimes.append(waitingTime)

    def calculate(self):
        diff = TimeDiff(self.arrivalTime, self.finishTime)
        self.turnAroundTime = diff.getTimeDiff()

        #c Total Running Time
        for runTime in self.runTimes:
            self.totalRunTime += runTime.getTimeDiff()

        # Total Waiting Time
        for waitTime in self.waitTimes:
            self.totalWaitTime += waitTime.getTimeDiff()
    

class parser():
    def __init__(self, filePath):
        self.fileContent = []
        self.arrivalTime = 0
        self.tasks = {}
        self.readFile(filePath)
        self.getArrivalTime()
        self.initTasks()
        self.getStartTime()
        self.getFinishTime()
        self.getRunTime()

    def readFile(self, filePath):
        print("Start reading file: {0}".format(filePath))
        with open(filePath, "r") as f:
            for line in f:
                self.fileContent.append(line)
    
    def getArrivalTime(self):

        for idx, line in enumerate(self.fileContent[20:]):
            if (line.find("allocated") != -1):
                self.arrivalTime = parse_utils.extract_time(line)
                print("[line-{0}]: arrivalTime={1}".format(idx+21, self.arrivalTime))
                return

    def initTasks(self):
        self.tasks = {"test2": task(self.arrivalTime), "test3" : task(self.arrivalTime),
                    "test4": task(self.arrivalTime), "test5" : task(self.arrivalTime),
                    "test6" : task(self.arrivalTime)}

    def getStartTime(self):
        for idx, line in enumerate(self.fileContent):
            taskName = parse_utils.is_task_startTime(line)
            if taskName:
                self.tasks[taskName].startTime = parse_utils.extract_time(line)
                print("[task:{0}] start time = {1}".format(taskName,self.tasks[taskName].startTime))

    def getFinishTime(self):
        for idx, line in enumerate(self.fileContent):
            taskName = parse_utils.is_task_finishTime(line)
            if taskName:
                self.tasks[taskName].finishTime = parse_utils.extract_time(line)
                print("[task:{0}] finish time = {1}".format(taskName,self.tasks[taskName].finishTime))

    def getRunTime(self):
        print("\n")
        idx = 20
        while idx < len(self.fileContent):
            result, taskName = parse_utils.is_runTime(self.fileContent[idx])
            if result:
                runStartTime = parse_utils.extract_time(self.fileContent[idx])
                # get finish time; sometimes the finish is not in the next line
                finishidx = idx
                ftaskName = taskName
                result = True
                while result & (ftaskName == taskName):
                    finishidx += 1
                    fresult, ftaskName = parse_utils.is_runTime(self.fileContent[finishidx])

                runFinishTime = parse_utils.extract_time(self.fileContent[finishidx])
                idx = finishidx - 1
                #self.tasks[taskName].runTimes.append(TimeDiff(runStartTime, runFinishTime))
                self.tasks[taskName].addRunTime(runStartTime, runFinishTime)
                print("[task:{0}] run time, from:[{1}] to [{2}]".format(taskName,runStartTime,runFinishTime))
            
            idx += 1


    def calculate(self):
        print("\n\n\n")
        print("--- Setup Results ---")
        
        AvgWaitingTime = 0
        AvgTurnAroundTime = 0

        for key in self.tasks:
            taskVal = self.tasks[key]
            taskVal.calculate()

            AvgWaitingTime += taskVal.totalWaitTime
            AvgTurnAroundTime += taskVal.turnAroundTime

            print("[{}]\tRuning(s)= {:.4f}\tWaiting(s)= {:.4f}\tTAT(s)= {:.4f}".format(key,taskVal.totalRunTime,
                                                            taskVal.totalWaitTime, taskVal.turnAroundTime))
        print("--------------------")

        AvgWaitingTime = AvgWaitingTime/len(self.tasks)
        AvgTurnAroundTime = AvgTurnAroundTime/len(self.tasks)

        return AvgWaitingTime, AvgTurnAroundTime

def userTest():
    print("start parser program..")
    if len(argv) < 3:
        print ("usage: python3 parser.py logsfile.txt")
        exit(255)

    p = parser(argv[1])
    p.calculate()
    
    print("Program finished")

def internalTest():
    adaptiveRR = parser("./AdaptiveRR.txt")
    adapAvgWait, adapAvgTAT = adaptiveRR.calculate()

    normalRR = parser("./NormalRR.txt")
    avgWait, avgTAT = normalRR.calculate()

    waitOpt = (1- (adapAvgWait / avgWait))*100
    tatOpt = (1-(adapAvgTAT / avgTAT))*100

    print("\n\n")
    print("====> Summary: ")
    print("\n----- Normal RR -----")
    print("Avg Waiting Time= {:.4f} Sec \t Avg Turn Around Time= {:.4f} Sec".format(avgWait,avgTAT))
    print("\n----- Adaptive RR -----")
    print("Avg Waiting Time= {:.4f} Sec \t Avg Turn Around Time= {:.4f} Sec".format(adapAvgWait,adapAvgTAT))
    print("\nAdaptive RR Enhance: Avg Waiting [{:.0f}%], Avg TAT [{:.0f}%]".format(waitOpt,tatOpt))
    print("\n\n\n\n")

def main():
    #userTest()
    internalTest()


if __name__ == "__main__":
    main()