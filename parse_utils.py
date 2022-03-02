import re
import datetime

def extract_time(text):
    regex = r"\d*:\d*:\d*.\d*"
    return re.findall(regex, str(text))[0]


def time_diff(time1str, time2str):
    time1 = datetime.datetime.strptime(time1str, "%H:%M:%S.%f")
    time2 = datetime.datetime.strptime(time2str, "%H:%M:%S.%f")
    timedif = time2 - time1
    return timedif.total_seconds()


def is_task_startTime(text):
    regex = r"start\stest\d"
    result = re.findall(regex, str(text))
    if len(result) > 0:
        return result[0][-5:]
    else:
        return ""


def is_task_finishTime(text):
    regex = r"done\stest\d"
    result = re.findall(regex, str(text))
    if len(result) > 0:
        return result[0][-5:]
    else:
        return ""

def is_runTime(text):
    regex = r"22222|33333|44444|55555|66666"
    result = re.findall(regex, str(text))
    if len(result) > 0:
        return True,"test"+result[0][-2]
    else:
        return False,""

def main():
    print("input test line ...")
    usrInput = input()
    time1str = extract_time(usrInput)
    usrInput = input()
    time2str = extract_time(usrInput)
    print("time difference = {0} Seconds".format(time_diff(time1str, time2str)))

if __name__ == "__main__":
    main()


