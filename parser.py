from sys import exit, argv
import parse_utils

class TimeDiff():
    def __init__(self, startTime, endTime):
        self.startTime = startTime
        self.endTime = endTime

class task():
    def __init__(self, arrivalTime):
        self.arrivalTime = arrivalTime
        self.startTime = 0
        self.finishTime = 0
        self.runTimes = []
        self.waitTimes = []
        self.totalTime = 0

    def addRunTime(self, startTime, endTime):
        self.runTimes.append(TimeDiff(startTime, endTime))
        if len(self.runTimes) > 1:
            #waiting time is drived from the new run time with relative to the previous run time
            # wait time = start ( previous run end time) , end ( new run start time)
            waitingTime = TimeDiff(self.runTimes[-2].endTime , self.runTimes[-1].startTime)
            self.waitTimes.append(waitingTime)
    

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
                runFinishTime = parse_utils.extract_time(self.fileContent[idx+1])
                idx += 1
                #self.tasks[taskName].runTimes.append(TimeDiff(runStartTime, runFinishTime))
                self.tasks[taskName].addRunTime(runStartTime, runFinishTime)
                print("[task:{0}] run time, from:[{1}] to [{2}]".format(taskName,runStartTime,runFinishTime))
            
            idx += 1


def main():
    print("start parser program..")
    if len(argv) < 2:
        print ("usage: python3 parser.py logsfile.txt")
        exit(255)

    p1 = parser(argv[1])
    
    print("Program finished")

if __name__ == "__main__":
    main()