import json
import os

def __calc_core_amd(core, row = 8, col = 8):
    amd = 0
    r = core // col
    c = core % col
    for i in range(row):
        for j in range(col):
            amd += abs(r - i) + abs(c - j)
    amd /= row * col
    return amd

def __parse_trace(core, cuts, path, filename, dividor = 1):
    lst = []
    with open(os.path.join(path, filename), 'r') as file:
        data = json.load(file)[0][core]
        n = len(data)
        cut_length = n / cuts
        data_left_ratio = 1
        cut_covered = 0
        cut_value = 0
        i = 0
        while i < n:
            if cut_length - cut_covered <= data_left_ratio:
                rem = cut_length - cut_covered
                cut_value += data[i] * rem
                lst.append(cut_value / dividor)
                data_left_ratio -= rem
                cut_covered = 0
                cut_value = 0
            else:
                cut_value += data[i] * data_left_ratio
                cut_covered += data_left_ratio
                data_left_ratio = 1
                i += 1
        if len(lst) < cuts: lst.append(cut_value)
    assert len(lst) == cuts, '__parse_trace: Error, length of list not equal to cuts!'
    return lst

def __divide(lst1, lst2):
    assert len(lst1) == len(lst2), '__divide: Error, lengths are not equal!'
    return [n / d if d != 0 else None for n, d in zip(lst1, lst2)]

def __main(result_dir, cores, power_budget, cuts = 100):
    try:
        amd_data = []
        ips_data = []
        cllc_data = []
        for core in cores: 
            amd_data.append(__calc_core_amd(core))
            ips_data.append(__divide(__parse_trace(core, cuts, result_dir, 'FreqTraces.json', 1e9), __parse_trace(core, cuts, result_dir, 'CpiTraces.json')))
            cllc_data.append(__divide(__parse_trace(core, cuts, result_dir, 'NucaCpiTraces.json'), __parse_trace(core, cuts, result_dir, 'CpiStackPartTrace.json')))
        return [cores, amd_data, power_budget, ips_data, cllc_data]
    except AssertionError as msg:
        print(msg)
    except:
        print("__main: Error!")
    return []


if __name__  == "__main__":
    pass