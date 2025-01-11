# WinMut-Turbo

An Effective Mutation Analysis Tool for Large-scale C Programs.

You can check out our [video demostration](https://www.youtube.com/watch?v=jwkg1h_2OoY) of WinMut-Turbo

## 📚 Usage

### 1. Install

You can build WinMut++ from source.
For specific build method and detailed information, please refer to [WinMut-README](WinMut-README.md).

### 2. Build & Execute

There is a script called `run.sh` to build and execute the subjects.

The usage is
```text
./run.sh subject-name <build/run> <WinMut/WinMutNo/AccMut/AccMutNo/timing/validate> [time-limit/process-limit]
```
Examples:

If you want to run pcre2 with WinMut and restrict the max cases to 10, you should run
```shell
./run.sh pcre2 build WinMut
```
![build pcre2](build-pcre2.gif)


```shell
./run.sh pcre2 run WinMut 10
```
![build pcre2](execute-pcre2.gif)

More usage can be found in [here](WinMut-README.md#running-the-experiments-from-the-paper).

### 3. Mutation Score

In order to analyze the running process and calculate the mutation score, WinMut++ outputs some log files to the directory `winmut-log-dir/run` when running.

We provided a python script to analyze the results:

`experiments/analysis/mutation_score.py` reads the ``winmut-log-dir/run` directory to analyze mutation execution results and calculate mutation score.


The script will output two mutation scores, corresponding to `killed/generated` and `killed/covered`.  
Notes on other script output information:
   
   | Output                             | Description                                      |
   |-----------------------------------|------------------------------------------|
   | KILLED_BY_PROC_OUTPUT            | Mutants killed by inconsistency with original program output                |
   | KILLED_BY_PROC_END_STATUS        | Mutants killed by inconsistency with original program end status            |
   | KILLED_BY_BOTH                   | Mutants killed by inconsistency with original program output and end status|
   | SURVIVED_NOT_AFFECT_STATUS       | Mutants survived because they do not affect the program's end status        |
   | SURVIVED_NOT_AFFECT_OUTPUT       | Mutants survived because they do not affect the program's output        |
   | SURVIVED_BY_BOTH                 | Mutants survived because they do not affect the program's end status and end status|
   | SURVIVED_NOT_COVERED             | Mutants survived because they are not covered                |


Examples:
```shell
python3.8 ../analysis/mutation_score.py pcre2/WinMut/winmut-log-dir/run
```
![mutation score pcre2](mutation_score-pcre2.gif)


