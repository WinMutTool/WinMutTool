import sys
import traceback
import os
from enum import Enum, auto
import re
from math import exp, log, isclose

class MutationStatus(Enum):
    KILLED_BY_PROC_OUTPUT = auto()          # 程序输出
    KILLED_BY_PROC_END_STATUS = auto()      # 程序结束状态

    SURVIVED_NOT_AFFECT_STATUS = auto()     # 未影响状态
    SURVIVED_NOT_AFFECT_OUTPUT = auto()     # 未影响输出
    SURVIVED_NOT_COVERED = auto()           # 未参与运行

KILLED = [
            MutationStatus.KILLED_BY_PROC_END_STATUS,
            MutationStatus.KILLED_BY_PROC_OUTPUT
         ]

SURVIVED = [
            MutationStatus.SURVIVED_NOT_AFFECT_STATUS,
            MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT,
            MutationStatus.SURVIVED_NOT_COVERED
           ]



class Mutation:
    def __init__(self, m_type, sop, op_0, op_1, op_2):
        self.m_type = m_type
        self.sop = sop
        self.op_0 = op_0
        self.op_1 = op_1
        self.op_2 = op_2

        self.not_covered = True
        self.mutation_status = []

class ProcEndStatus(Enum):
    SAME_WITH_ORI = auto()
    EXITED = auto()
    SIGNALED = auto()

class ProcTreeNode:
    # TODO: mut_id_list 使用数组，或许可以换种高级方式
    def __init__(self, eq_class_tuple_list, proc_end_status, proc_exit_or_signal_val=0):
        self.eq_class_tuple_list = eq_class_tuple_list
        self.proc_end_status = proc_end_status
        if proc_end_status != ProcEndStatus.SAME_WITH_ORI:
            self.proc_exit_or_signal_val = proc_exit_or_signal_val

# MA : mutation analysis
class MACase:
    def __init__(self, case_dir) -> None:
        self.case_dir          = case_dir
        self.all_mutation_path = os.path.join(case_dir, "all_mutation")
        self.proc_tree_path    = os.path.join(case_dir, "proc_tree")

        self.all_mutation = self.getAllMutation(self.all_mutation_path)

        self.proc_tree = self.readProcTree(self.proc_tree_path)

        self.cal_mutation_score()

        self.initMACaseStat()

    class MACaseStat:
        # ms_k2g: mutation score: ratio of killed to generated mutants
        # ms_k2c: mutation score: ratio of killed to   covered mutants
        def __init__(self, case_dir, ms_k2g, ms_k2c) -> None:
            self.case_dir = case_dir
            self.ms_k2g = ms_k2g
            self.ms_k2c = ms_k2c

    def initMACaseStat(self):
        self.generated = len(self.all_mutation) - 1
        self.killed    = len(self.mutation_status_statistics_killed_set)
        self.uncovered = len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED])
        self.covered   = self.generated - self.uncovered

        self.killed_by_proc_output = len(self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT])
        self.killed_by_proc_end_status = len(self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_END_STATUS])
        self.killed_by_both = len(self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_END_STATUS] & self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT])

        self.survived_not_affect_status = len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_STATUS])
        self.survived_not_affect_output = len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT])
        self.survived_by_both = len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT] & self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_STATUS])
        self.survived_not_covered = len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED])


        assert (self.covered + self.uncovered == self.generated) and "check consistance in initMACaseStat\n"

        ms_k2g = self.killed / self.generated
        ms_k2c = self.killed / self.covered
        self.maCaseStat = self.MACaseStat(self.case_dir, ms_k2g, ms_k2c)

    def cal_mutation_score(self):
        self.checkProcEndStatus()
        self.checkProcOutput()

        self.all_mutation_num = len(self.all_mutation) - 1
        self.killed_mutation_num = 0

        # 基于分流执行的方法，对一个 mut_id，可能会出现多种状态，以 killed 为准，
        # 多个 killed 原因可以共存，但记录在 survived 中的 mut_id 不能有 killed
        self.mutation_status_statistics = {
            MutationStatus.KILLED_BY_PROC_OUTPUT        : set(),
            MutationStatus.KILLED_BY_PROC_END_STATUS    : set(),
            MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT   : set(),
            MutationStatus.SURVIVED_NOT_AFFECT_STATUS   : set(),
            MutationStatus.SURVIVED_NOT_COVERED         : set(),

        }
        self.mutation_status_statistics_killed_set = set()
        self.mutation_status_statistics_survived_set = set()
        # self.killed_by_proc_output_set = set()

        # self.killed_by_proc_end_status_set = set()
        # self.survivied_not_affect_status_set = set()
        # self.survivied_not_affect_output_set = set()
        # self.survivied_not_covered = set()
        for mut_id, mutation in enumerate(self.all_mutation):
            if mut_id == 0:
                continue
            else:
                assert isinstance(self.all_mutation[mut_id], Mutation) and "cal_mutation_score: not a Mutation class\n"
                # 前置，添加 MutationStatus.SURVIVED_NOT_COVERED 
                if self.all_mutation[mut_id].not_covered:
                    self.all_mutation[mut_id].mutation_status.append(MutationStatus.SURVIVED_NOT_COVERED)
                
                for mutation_status in self.all_mutation[mut_id].mutation_status:
                    if mutation_status in KILLED:
                        self.killed_mutation_num += 1
                        break
                self.classifyMutationStatus(mut_id, mutation)
        self.checkClassifyConsistency()

        
    
    def classifyMutationStatus(self,mut_id,  mutation):
        # assert isinstance(mutation, Mutation) and "classifyMutationStatus: not a Mutation class\n"
        killed_flag = False
        for mutation_status in self.all_mutation[mut_id].mutation_status:
            if mutation_status in KILLED:
                self.mutation_status_statistics[mutation_status].add(mut_id)
                self.mutation_status_statistics_killed_set.add(mut_id)
                killed_flag = True
        if not killed_flag:
            for mutation_status in self.all_mutation[mut_id].mutation_status:
                self.mutation_status_statistics[mutation_status].add(mut_id)
                self.mutation_status_statistics_survived_set.add(mut_id)
    
    def checkClassifyConsistency(self):
        # 1. 出现在 killed中的 mut_id 必定不能出现在 SURVIVED_NOT_COVERED 中
        for mut_id in self.mutation_status_statistics_killed_set:
            assert mut_id not in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED]
        # 2. 出现在 SURVIVED_NOT_COVERED 中的 mut_id，其对应的 mutation 应当只有这一条
        for mut_id in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED]:
            assert len(self.all_mutation[mut_id].mutation_status) == 1
        # 3. KILLED 的所有 flag 可以和除了 SURVIVED_NOT_COVERED 共存    （已被 1 检查）
        # 4. SURVIVED 中，NOT_AFFECT_STATUS 可以和 NOT_AFFECT_OUTPUT 共存，但不能和其他共存
        for mut_id in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_STATUS]:
            assert mut_id not in self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_END_STATUS] \
                   and mut_id not in self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT] \
                   and mut_id not in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED]
        for mut_id in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT]:
            assert mut_id not in self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_END_STATUS] \
                   and mut_id not in self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT] \
                   and mut_id not in self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED]
        # 5. killed + survived = all_mutation
        assert len(self.mutation_status_statistics_killed_set) + len(self.mutation_status_statistics_survived_set) \
                == len(self.all_mutation) - 1
        
    def __str__(self) -> str:
        print(self.case_dir)
        ret = self.case_name
        if len(self.all_mutation) - 1 == 0 :    # 确实就是这么神奇（grep case_162）
            ret += ": no mutation covered\n"
            return ret

        generated = len(self.all_mutation)
        killed    = len(self.mutation_status_statistics_killed_set)
        uncovered = len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED])
        covered   = generated - uncovered

        ret += "\n" + f"mutation score: {self.killed_mutation_num}/{self.all_mutation_num}({self.killed_mutation_num / self.all_mutation_num *100:>.2f}%)"
        ret += "\n" + f"mutation score: {killed}/{covered}({killed / covered *100:>.2f}%)"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"KILLED_BY_PROC_OUTPUT: {len(self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT])}"
        # ret += f" : {self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT]}"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"KILLED_BY_PROC_END_STATUS: {len(self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_END_STATUS])}"
        # ret += "\n"

        ret += "\t\t"
        ret += f"KILLED_BY_BOTH: {len(self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_END_STATUS] & self.mutation_status_statistics[MutationStatus.KILLED_BY_PROC_OUTPUT])}"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"SURVIVED_NOT_AFFECT_STATUS: {len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_STATUS])}"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"SURVIVED_NOT_AFFECT_OUTPUT: {len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT])}"
        # ret += "\n"

        ret += "\t\t"
        ret += f"SURVIVED_BY_BOTH: {len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT] & self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_AFFECT_STATUS])}"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"SURVIVED_NOT_COVERED: {len(self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED])}"
        # ret += f" : {self.mutation_status_statistics[MutationStatus.SURVIVED_NOT_COVERED]}"
        ret += "\n"

        ret += "\t\t\t"
        ret += f"total: {len(self.all_mutation) - 1} \
                 killed: {len(self.mutation_status_statistics_killed_set)} \
                 survivied: {len(self.mutation_status_statistics_survived_set)}"
        ret += "\n"
        return ret


    def readProcTree(self, proc_tree_path):
        proc_tree = []
        with open(proc_tree_path, encoding='utf-8', errors='replace') as f:
            for l in f:
                l = l.strip()
                if l.startswith('--'):
                    self.case_name = '--'.join(l.split('--')[2:]).strip()
                    continue
                elif l.startswith('++'):
                    continue
                elif l.startswith("accmut::ori_exit_val:"):
                    self.ori_end_status = ProcEndStatus.EXITED
                    self.ori_exit_or_signal_val = int(l.split("accmut::ori_exit_val:")[1].strip())
                elif l.startswith("reduced:"):          # reduced with ori (0)
                    eq_class_reduced_str = l.split("reduced:")[1].strip()
                    eq_class_reduced_tuple_list = self.getEqClass(eq_class_reduced_str)

                    # self.getProcFromEqClass(eq_class_reduced_tuple_list, ProcEndStatus.SAME_WITH_ORI)
                    proc_tree.append(ProcTreeNode(eq_class_reduced_tuple_list, ProcEndStatus.SAME_WITH_ORI))
                else:
                    assert l[0] in '0123456789' and f"{l} not a proc\n"
                    fork_from = int(l.split("=>")[0].strip())
                    eq_from   = int(l.split("=>")[1].split(":")[0].strip())
                    eq_to	  = int(l.split("=>")[1].split(":")[1].strip())
                    mut_id_in_all_mutation    = int(l.split(":")[2].split("(")[0].strip())
                    mut_str_in_module = l.split("(")[1].split(")")[0].strip()
                    proc_end_status_str = l.split("):")[1].split("accmut::eq_class:")[0].split("/")[1].strip()
                    eq_class_str = l.split("accmut::eq_class:")[1].strip()

                    eq_class_tuple_list = self.getEqClass(eq_class_str)
                    proc_end_status, proc_exit_or_signal_val = self.getProcEndStatus(proc_end_status_str)

                    proc_tree.append(ProcTreeNode(eq_class_tuple_list, proc_end_status, proc_exit_or_signal_val))
        return proc_tree
    
    def getOutputNeedCheck(self):
        contents = os.listdir(self.case_dir)
        file_need_check   = []
        mut_id_need_check = []
        for content in contents:
            if content.startswith("mut_output-"):
                if content.startswith("mut_output-0-"):
                    assert os.path.isfile(os.path.join(self.case_dir, content))             \
                            and f"getOutputNeedCheck: {self.case_dir}/{content}not a file\n"
                    file_need_check.append(content.split("mut_output-0-")[1])
                else:
                    pattern = re.compile(r'^mut_output-(\d+)-(.*)$')
                    match_result = pattern.match(content)
                    assert match_result and f"{content} : not a mut output\n"
                    mut_id = int(match_result.group(1))
                    mut_id_need_check.append(mut_id)

        return file_need_check, mut_id_need_check


    def isSameContent(self, file1_path, file2_path):
        # print(file1_path, file2_path)
        with open(file1_path, 'rb') as file1, open(file2_path, 'rb') as file2:  # 二进制打开对比，确实有不明原因的乱码，默认的utf-8无法识别
            content1 = file1.read()
            content2 = file2.read()
            return content1 == content2

    def checkProcOutput(self):
        file_need_check, mut_id_need_check = self.getOutputNeedCheck()

        for mut_id in mut_id_need_check:
            for output_file_name in file_need_check:
                ori_file = "mut_output-" + str(0) + "-" + output_file_name
                ori_file_path = os.path.join(self.case_dir, ori_file)
                cur_check_file = "mut_output-" + str(mut_id) + "-" + output_file_name
                cur_check_file_path = os.path.join(self.case_dir, cur_check_file)

                assert os.path.isfile(ori_file_path) and f"{ori_file_path}: should exist\n"

                assert isinstance(self.all_mutation[mut_id], Mutation) and "checkProcOutput: not a Mutation class\n"
                assert self.all_mutation[mut_id].not_covered == False and "if have output, it must be covered\n"

                if os.path.isfile(cur_check_file_path):
                    if self.isSameContent(ori_file_path, cur_check_file_path):
                        self.all_mutation[mut_id].mutation_status.append(MutationStatus.SURVIVED_NOT_AFFECT_OUTPUT)
                    else:
                        self.all_mutation[mut_id].mutation_status.append(MutationStatus.KILLED_BY_PROC_OUTPUT)
                else: 
                    self.all_mutation[mut_id].mutation_status.append(MutationStatus.KILLED_BY_PROC_OUTPUT)

    def isSameProcEndStatusWithOri(self, proc):
        assert isinstance(proc, ProcTreeNode) and "isSameProcEndStatusWithOri: not a ProcTreeNode class\n"
        return (proc.proc_end_status == self.ori_end_status) and (proc.proc_exit_or_signal_val == self.ori_exit_or_signal_val)

    def checkProcEndStatus(self):
        for proc in self.proc_tree:
            assert isinstance(proc, ProcTreeNode) and "checkProcEndStatus: not a ProcTreeNode class\n"

            eq_class_mut_id = self.getIdListFromTupleList(proc.eq_class_tuple_list)
            proc_end_status = proc.proc_end_status
            
            for mut_id in eq_class_mut_id:
                if mut_id != 0:
                    assert isinstance(self.all_mutation[mut_id], Mutation) and "checkProcEndStatus: not a Mutation class\n"
                    self.all_mutation[mut_id].not_covered = False

                    if self.isSameProcEndStatusWithOri(proc) or proc_end_status == ProcEndStatus.SAME_WITH_ORI:
                        self.all_mutation[mut_id].mutation_status.append(MutationStatus.SURVIVED_NOT_AFFECT_STATUS)
                    else:
                        self.all_mutation[mut_id].mutation_status.append(MutationStatus.KILLED_BY_PROC_END_STATUS)

            # if proc_end_status == ProcEndStatus.SAME_WITH_ORI:
            #     for mut_id in eq_class_mut_id:
            #         if mut_id != 0:
            #             assert isinstance(self.all_mutation[mut_id], Mutation) and "checkProcEndStatus: not a Mutation class\n"
            #             self.all_mutation[mut_id].not_covered = False
            #             self.all_mutation[mut_id].mutation_status.append[MutationStatus.SURVIVED_NOT_AFFECT_STATUS]
            # else:
            #     if self.isSameProcEndStatus(proc, self.ori_end_status, self.ori_exit_or_signal_val):
            #         for mut_id in eq_class_mut_id:
            #             assert mut_id != 0 and "checkProcEndStatus: if need compare end status, it must not ori\n"
            #             self.all_mutation[mut_id].not_covered = False
            #             self.all_mutation[mut_id].mutation_status.append[MutationStatus.SURVIVED_NOT_AFFECT_STATUS]
            #     else:
            #         for mut_id in eq_class_mut_id:
            #             assert mut_id != 0 and "checkProcEndStatus: if need compare end status, it must not ori\n"
            #             self.all_mutation[mut_id].not_covered = False
            #             self.all_mutation[mut_id].mutation_status.append[MutationStatus.KILLED_BY_PROC_END_STATUS]

            

            
    
    def getIdListFromTupleList(self, eq_class_tuple_list):
        eq_class_mut_id = []
        for eq_class_tuple in eq_class_tuple_list:
            eq_class_mut_id.append(eq_class_tuple[0])
        return eq_class_mut_id

    def getProcEndStatus(self, proc_end_status_str):
        proc_end_status = None
        proc_exit_or_signal_val = None

        if proc_end_status_str.startswith("r"):
            proc_end_status = ProcEndStatus.EXITED
            proc_exit_or_signal_val = int(proc_end_status_str.split("r")[1].strip())
        elif proc_end_status_str.startswith("s"):
            proc_end_status = ProcEndStatus.SIGNALED
            proc_exit_or_signal_val = int(proc_end_status_str.split("s")[1].strip())
        else:
            assert False and f"{proc_end_status_str}: error process end status\n"

        return proc_end_status, proc_exit_or_signal_val

    def getProcFromEqClass(self, eq_class_tuple_list, proc_end_status, proc_end_status_str=""):
        pass

    def getEqClass(self, eq_class_str):             # [id : val] [id : val] [id : val] ...
        eq_class = []
        for raw_str in eq_class_str.split("]"):
            if raw_str.strip():
                eq_class_mut_id = int(raw_str.split(":")[0].split("[")[1].strip())
                eq_class_value = int(raw_str.split(":")[1].strip())
                eq_class_tuple = (eq_class_mut_id, eq_class_value)
                eq_class.append(eq_class_tuple)
        return eq_class

    def getAllMutation(self, all_mutation_path):
        all_mutation = []
        with open(all_mutation_path, encoding='utf-8', errors='replace') as f:
            for l in f:
                if l.startswith('--'):
                    continue
                elif l.startswith('++'):
                    continue
                else:
                    m_str = l.split(":")
                    assert len(m_str) == 5 and f"mutation error: {l}"
                    m_type = int(m_str[0].strip())
                    sop = int(m_str[1].strip())
                    op_0 = int(m_str[2].strip())
                    op_1 = int(m_str[3].strip())
                    op_2 = int(m_str[4].strip())
                    all_mutation.append(Mutation(m_type, sop, op_0, op_1, op_2))
        assert len(all_mutation) >= 1 and f"empty file: {all_mutation_path}"
        return all_mutation
        

# case_in_run = {
# 	case_0 : case_0_path,
# 	case_1 : case_1_path,
# 	...
# }
def readCaseInRun(runlog_dir):
    case_in_run = {}
    items = os.listdir(runlog_dir)
    items.sort()
    for item in items:
        item_path = os.path.join(runlog_dir, item)
        assert os.path.isdir(item_path) and f"{item_path} not a case dir!\n"
        case_in_run[item] = item_path
    return case_in_run
        

# def cal_mutation_score(runlog_dir):
#     error_list = []
#     case_in_run = readCaseInRun(runlog_dir)
#     for case_dir in case_in_run.values():
#         try:
#             print(MACase(case_dir))
#         except Exception as e:
#             error_list.append(case_dir)
#             print("\033[91;1mError: \033[0m", case_dir)
#     print(error_list)
    
class Run:
    def __init__(self, runlog_dir) -> None:
        self.case_in_run = self.readCaseInRun(runlog_dir)

        self.maCaseList     = self.initMACaseList(self.case_in_run)
        self.maCaseStatList = self.initMACaseStatList(self.maCaseList)

        self.runStat = self.initRunStat(self.maCaseStatList)

    class RunStat:
        def __init__(self, case_num, 
                     ms_k2g_min, ms_k2g_med, ms_k2g_max, ms_k2g_am, ms_k2g_gm,
                     ms_k2c_min, ms_k2c_med, ms_k2c_max, ms_k2c_am, ms_k2c_gm) -> None:
            self.case_num = case_num
            self.ms_k2g_min = ms_k2g_min
            self.ms_k2g_med = ms_k2g_med
            self.ms_k2g_max = ms_k2g_max
            self.ms_k2g_am = ms_k2g_am
            self.ms_k2g_gm = ms_k2g_gm
            self.ms_k2c_min = ms_k2c_min
            self.ms_k2c_med = ms_k2c_med
            self.ms_k2c_max = ms_k2c_max
            self.ms_k2c_am = ms_k2c_am
            self.ms_k2c_gm = ms_k2c_gm


        def __str__(self) -> str:
            ret = f"In {self.case_num} cases: \n"
            ret += "      Min.      \t      Med.      \t      Max.      \t      A.M.      \t      G.M.      \n"
            ret += f"{self.ms_k2g_min * 100 :>6.2f}%({self.ms_k2c_min * 100 :>6.2f}%)"

            ret += "\t"
            ret += f"{self.ms_k2g_med * 100 :>6.2f}%({self.ms_k2c_med * 100 :>6.2f}%)"

            ret += "\t"
            ret += f"{self.ms_k2g_max * 100 :>6.2f}%({self.ms_k2c_max * 100 :>6.2f}%)"

            ret += "\t"
            ret += f"{self.ms_k2g_am * 100 :>6.2f}%({self.ms_k2c_am * 100 :>6.2f}%)"

            ret += "\t"
            ret += f"{self.ms_k2g_gm * 100 :>6.2f}%({self.ms_k2c_gm * 100 :>6.2f}%)"

            ret += ""
            return ret

    def geometric_mean(self, arr):
        if not arr:
            return None  # 处理空数组的情况，你可以根据实际需求返回适当的值

        product = 1
        non_zero_arr = [num for num in arr if not isclose(num, 0)]

        if not non_zero_arr:
            return 0
        for num in non_zero_arr:
            product *= num

        geometric_mean_value = exp(log(product) / len(non_zero_arr))
        return geometric_mean_value

    def initRunStat(self, maCaseStatList):
        sorted_by_ms_k2g = sorted(maCaseStatList, key=lambda x: x.ms_k2g)
        sorted_by_ms_k2c = sorted(maCaseStatList, key=lambda x: x.ms_k2c)

        ms_k2g_values = [case.ms_k2g for case in maCaseStatList]
        ms_k2c_values = [case.ms_k2c for case in maCaseStatList]

        ms_k2g_min = sorted_by_ms_k2g[0].ms_k2g
        ms_k2g_max = sorted_by_ms_k2g[-1].ms_k2g
        ms_k2g_med = sorted_by_ms_k2g[len(sorted_by_ms_k2g) // 2].ms_k2g
        ms_k2g_am = sum(ms_k2g_values) / len(ms_k2g_values)
        ms_k2g_gm = self.geometric_mean(ms_k2g_values)

        ms_k2c_min = sorted_by_ms_k2c[0].ms_k2c
        ms_k2c_max = sorted_by_ms_k2c[-1].ms_k2c
        ms_k2c_med = sorted_by_ms_k2c[len(sorted_by_ms_k2c) // 2].ms_k2c
        ms_k2c_am = sum(ms_k2c_values) / len(ms_k2c_values)
        ms_k2c_gm = self.geometric_mean(ms_k2c_values)

        return self.RunStat(len(maCaseStatList), 
                            ms_k2g_min, ms_k2g_med, ms_k2g_max, ms_k2g_am, ms_k2g_gm,
                            ms_k2c_min, ms_k2c_med, ms_k2c_max, ms_k2c_am, ms_k2c_gm)
    
    def getRunStat(self):
        return self.runStat
    
    def getRunStatJson(self):
        ret = {}
        ret['Summary'] = [
            # {
            #     'label': 'ms_min',
            #     'ms_k2g': self.runStat.ms_k2g_min,
            #     'ms_k2c': self.runStat.ms_k2c_min,
            # },
            # {
            #     'label': 'ms_med',
            #     'ms_k2g': self.runStat.ms_k2g_med,
            #     'ms_k2c': self.runStat.ms_k2c_med,
            # },
            # {
            #     'label': 'ms_max',
            #     'ms_k2g': self.runStat.ms_k2g_max,
            #     'ms_k2c': self.runStat.ms_k2c_max,
            # },
            # {
            #     'label': 'ms_avg',
            #     'ms_k2g': self.runStat.ms_k2g_avg,
            #     'ms_k2c': self.runStat.ms_k2c_avg,
            # },
            {
                'label': 'Min.',
                'ms_k2g': f"{self.runStat.ms_k2g_min * 100 :>6.2f}%",
                'ms_k2c': f"{self.runStat.ms_k2c_min * 100 :>6.2f}%",
            },
            {
                'label': 'Med.',
                'ms_k2g': f"{self.runStat.ms_k2g_med * 100 :>6.2f}%",
                'ms_k2c': f"{self.runStat.ms_k2c_med * 100 :>6.2f}%",
            },
            {
                'label': 'Max.',
                'ms_k2g': f"{self.runStat.ms_k2g_max * 100 :>6.2f}%",
                'ms_k2c': f"{self.runStat.ms_k2c_max * 100 :>6.2f}%",
            },
            {
                'label': 'A.M.',
                'ms_k2g': f"{self.runStat.ms_k2g_am * 100 :>6.2f}%",
                'ms_k2c': f"{self.runStat.ms_k2c_am * 100 :>6.2f}%",
            },
            {
                'label': 'G.M.',
                'ms_k2g': f"{self.runStat.ms_k2g_gm * 100 :>6.2f}%",
                'ms_k2c': f"{self.runStat.ms_k2c_gm * 100 :>6.2f}%",
            },
            
        ]

        # ret['Summary'] = {
        #     'ms_k2g_min': self.runStat.ms_k2g_min,
        #     'ms_k2c_min': self.runStat.ms_k2c_min,
        #     'ms_k2g_med': self.runStat.ms_k2g_med,
        #     'ms_k2c_med': self.runStat.ms_k2c_med,
        #     'ms_k2g_max': self.runStat.ms_k2g_max,
        #     'ms_k2c_max': self.runStat.ms_k2c_max,
        #     'ms_k2g_avg': self.runStat.ms_k2g_avg,
        #     'ms_k2c_avg': self.runStat.ms_k2c_avg,
        # }

        ret['MACases'] = {}
        self.maCaseList = sorted(self.maCaseList, key=lambda x: int(os.path.basename(x.case_dir ).strip().split('_')[1]))
        for maCase in self.maCaseList:
            isinstance(maCase, MACase)
            ret['MACases'][os.path.basename(maCase.case_dir)] = [
                {'label': 'case_name',                    'value': maCase.case_name,},
                {'label': 'generated',                    'value': maCase.generated,},
                {'label': 'killed',                       'value': maCase.killed,},
                {'label': 'uncovered',                    'value': maCase.uncovered,},
                {'label': 'covered',                      'value': maCase.covered,},
                {'label': 'ms_k2g',                       'value': f"{maCase.maCaseStat.ms_k2g * 100 :>6.2f}%",},
                {'label': 'ms_k2c',                       'value': f"{maCase.maCaseStat.ms_k2c * 100 :>6.2f}%",},
                {'label': 'killed_by_proc_output',        'value': maCase.killed_by_proc_output,},
                {'label': 'killed_by_proc_end_status',    'value': maCase.killed_by_proc_end_status,},
                {'label': 'killed_by_both',               'value': maCase.killed_by_both,},
                {'label': 'survived_not_affect_status',   'value': maCase.survived_not_affect_status,},
                {'label': 'survived_not_affect_output',   'value': maCase.survived_not_affect_output,},
                {'label': 'survived_by_both',             'value': maCase.survived_by_both,},
                {'label': 'survived_not_covered',         'value': maCase.survived_not_covered,},
            ]
            # ret['MACases'][os.path.basename(maCase.case_dir)] = {
            #     'case_name':                    maCase.case_name,
            #     'generated':                    maCase.generated,
            #     'killed':                       maCase.killed,
            #     'uncovered':                    maCase.uncovered,
            #     'covered':                      maCase.covered,
            #     'ms_k2g':                       maCase.maCaseStat.ms_k2g,
            #     'ms_k2c':                       maCase.maCaseStat.ms_k2c,
            #     'killed_by_proc_output':        maCase.killed_by_proc_output,
            #     'killed_by_proc_end_status':    maCase.killed_by_proc_end_status,
            #     'killed_by_both':               maCase.killed_by_both,
            #     'survived_not_affect_status':   maCase.survived_not_affect_status,
            #     'survived_not_affect_output':   maCase.survived_not_affect_output,
            #     'survived_by_both':             maCase.survived_by_both,
            #     'survived_not_covered':         maCase.survived_not_covered,

            # }
        return ret

    def initMACaseStatList(self, maCaseList):
        maCaseStatList = []
        for maCase in maCaseList:
            assert isinstance(maCase, MACase)
            maCaseStatList.append(maCase.maCaseStat)
        return maCaseStatList

    def initMACaseList(self, case_in_run):
        maCaseList = []
        error_list = []
        for case_dir in case_in_run.values():
            try:
                maCase = MACase(case_dir)
                print(maCase)
                maCaseList.append(maCase)
            except Exception as e:
                error_list.append(case_dir)
                print("\033[91;1mError: \033[0m", case_dir)
        print(error_list)
        # assert len(error_list) == 0 and "check error case list\n"
        return maCaseList

    def readCaseInRun(self, runlog_dir):
        case_in_run = {}
        items = os.listdir(runlog_dir)
        items = sorted(items, key=lambda x: int(x.split('_')[1]))
        for item in items:
            item_path = os.path.join(runlog_dir, item)
            assert os.path.isdir(item_path) and f"{item_path} not a case dir!\n"
            case_in_run[item] = item_path
        return case_in_run 

if __name__ == '__main__':
    try:
        runlog_dir = sys.argv[1]
        # cal_mutation_score(runlog_dir)
        run = Run(runlog_dir)
        print(run.getRunStat())

        
    except Exception as e:
        traceback.print_exc(file=sys.stdout)