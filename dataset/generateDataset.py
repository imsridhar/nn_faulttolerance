import json
from os.path import join, dirname, realpath
CUR_FOLDER_PATH = dirname(realpath(__file__))

import parseTraces

import sys
sys.path.append('../simulationcontrol/')
import run

def __set_cfg(path, key, val, filename = "./../config/base.cfg"):
    new_text = ""
    with open(join(CUR_FOLDER_PATH, filename), 'r') as file:
        found = False
        for line in file:
            stripped_line = line.strip()
            lst = stripped_line.split('[')
            if len(lst) >= 2:
                lst = lst[1].split(']')
                if len(lst) >= 2:
                    if lst[0] == path: found = True
                    else: found = False
            
            lst = stripped_line.split('=')
            if found and len(lst) >= 2 and lst[0].strip() == key:
                lst2 = lst[1].split('#')
                lst2[0] = val + ' '
                lst[1] = '#'.join(lst2)
                text = lst[0].strip() + ' = ' + lst[1]
                new_text += text + '\n'
            else: new_text += line
            
    with open(join(CUR_FOLDER_PATH, filename), 'w') as file:
        file.write(new_text)

def __update_cores_and_powerBudget(cores, powerBudget):
    formatCores = ""
    for core in cores: formatCores += str(core) + ","
    __set_cfg("scheduler/open", "fixed_core_ids", formatCores + "-1")
    __set_cfg("scheduler/open/dvfs/fixed_power", "per_core_power_budget", "{:.1f}".format(powerBudget))

def __calc_amd_to_cores_mapping(row = 8, col = 8):
    amd_to_cores = {}
    for i in range(row * col):
        amd = parseTraces.__calc_core_amd(i, row, col)
        if amd in amd_to_cores: amd_to_cores[amd].append(i)
        else: amd_to_cores[amd] = [i]
    return amd_to_cores

def __get_amd_to_cores_mapping(amds):
    amd_to_cores = __calc_amd_to_cores_mapping()
    return [amd_to_cores[amd] for amd in amds]

def __get_amd_to_cores_specificMapping(amds):
    amd_to_cores = {4.0: [27, 28, 35, 36], 4.5: [18, 21, 42, 45], 5.5: [9, 14, 49, 54], 6.25: [1, 15, 48, 62], 7.0: [0, 7, 56, 63]}
    return [amd_to_cores[amd] for amd in amds]

def __get_operating_points(power_budgets, amds, cores_num=1):
    if cores_num == 4: cores_list  =__get_amd_to_cores_specificMapping(amds)
    else: cores_list = __get_amd_to_cores_mapping(amds)
    operating_points = []
    for cores in cores_list:
        for power_budget in power_budgets:
            operating_points.append((cores[:cores_num], power_budget))
    return operating_points

def __write_json(data, path, filename):
    with open(join(path, filename), 'w+') as file:
        json.dump(data, file)

def __model_raw_dataset(raw_dataset, benchmark, path = CUR_FOLDER_PATH, filename = 'final_data.json'):
    dataset = [['Benchmark', 'AMD', 'PB', 'IPS', 'cLLC', 'Target_AMD', 'Target_PB', 'Target_IPS']]
    op_count = len(raw_dataset)
    thread_count = len(raw_dataset[0][0])
    cuts = len(raw_dataset[0][-1][0])
    for cut_i in range(cuts - 1):
        for op_i in range(op_count):
            for op_j in range(op_count):
                if op_i == op_j: continue
                for thread_i in range(thread_count):
                    if raw_dataset[op_i][3][thread_i][cut_i] and raw_dataset[op_i][4][thread_i][cut_i] and raw_dataset[op_j][3][thread_i][cut_i + 1]:
                        dataset.append([benchmark, raw_dataset[op_i][1][thread_i], raw_dataset[op_i][2], raw_dataset[op_i][3][thread_i][cut_i], raw_dataset[op_i][4][thread_i][cut_i], raw_dataset[op_j][1][thread_i], raw_dataset[op_j][2], raw_dataset[op_j][3][thread_i][cut_i + 1]])
    __write_json(dataset, path, "{}_{}".format(benchmark, filename))

def __run_benchmark(benchmark = 'parsec-x264', cores_num = 4):
    try:
        power_budgets = [1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0]
        #power_budgets = [3.5, 4.0]
        # power_budgets = [4.0]
        amds = [4.0, 4.5, 5.5, 6.25, 7.0]
        #amds = [4.0]
        operating_points = __get_operating_points(power_budgets, amds, cores_num)

        result_dirs_info = []
        for p in operating_points:
            print("[Update base.cfg]: Updating cores to {} and power budget to {} in base.cfg.".format(p[0], p[1]))
            __update_cores_and_powerBudget(p[0], p[1])
            print("[Running Simulation]: Running fixed_power simulation for cores {} and power budget {}.".format(p[0], p[1]))
            run.BATCH_START = run.datetime.datetime.now().strftime('%Y-%m-%d_%H.%M.%S')
            result_dir = run.run(['4.0GHz', 'fixedPower', 'slowDVFS'], run.get_instance(benchmark, cores_num, input_set='simsmall'))
            print("[Save Simulation]: Saved simulation logs to {}.".format(result_dir))
            result_dirs_info.append((result_dir, p))

        raw_dataset = []
        for result_dir_info in result_dirs_info:
            result_dir = result_dir_info[0]
            print("[Parsing Traces]: Parsing traces of {} to get raw data.".format(result_dir))
            parsedData = parseTraces.__main(join(join(CUR_FOLDER_PATH, './../results'), result_dir), result_dir_info[1][0], result_dir_info[1][1])
            assert parsedData != [], '__main: Error, empty parsed data!'
            raw_dataset.append(parsedData)
        
        print("[Saving Raw Data]: Saving raw data")
        __write_json(raw_dataset, CUR_FOLDER_PATH, "{}_raw_data.json".format(benchmark))
        print("[Saving Final Data]: Saving final data")
        __model_raw_dataset(raw_dataset, benchmark)
    except AssertionError as msg:
        print(msg)
    except:
        print("__main: Error!")

def __main():
    for benchmark in (
                    #   'parsec-blackscholes',
                    #   'parsec-bodytrack',
                    #   'parsec-canneal',
                    #   'parsec-dedup',
                    #   'parsec-streamcluster',
                    #   'parsec-swaptions',
                      'parsec-x264',
                      ):
        __run_benchmark(benchmark)

if __name__ == "__main__":
    # for r in range(8):
    #     for c in range(8):
    #         print(parseTraces.__calc_core_amd(8 * r + c), end=" ")
    #     print()
    __main()
