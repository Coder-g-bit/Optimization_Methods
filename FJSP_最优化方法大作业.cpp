// 基于极限完工时间最小化的初始化方式实现FJSP问题的模拟退火算法
#include <bits/stdc++.h>
#include <time.h>
using namespace std;

// 记录机器选项
struct MachineOption
{
    int machine_id; // 机器编号
    int proc_time;  // 加工时间
};

// 记录工序选项
struct Operation
{
    vector<MachineOption> machines; // 可选机器列表
};

// 记录工件流程
struct Job
{
    vector<Operation> operations; // 工序列表
};

// 解析输入数据
struct Instance
{
    int num_jobs;     // 工件数量
    int num_machines; // 机器数量
    vector<Job> jobs; // 工件列表
};

// 解结构
struct Solution
{
    vector<int> OS;         // 工序序列（job id）
    vector<vector<int>> MS; // 每道工序选择的机器（索引）
    vector<int> op_counter; // 每个job的当前工序计数（用于快速定位）
    int makespan;
};

// 去掉字符串中的多余字符
string trim(const string &s)
{
    string res;
    for (char c : s)
    {
        if (c != '\r' && c != '\n')
            res += c;
    }
    return res;
}

// 解析机器编号和加工时间
MachineOption parseMachineOption(const string &token)
{
    MachineOption opt;

    int m_pos = token.find('M');
    int comma = token.find(',');
    int rparen = token.find(')');

    opt.machine_id = stoi(token.substr(m_pos + 1, comma - m_pos - 1));
    opt.proc_time = stoi(token.substr(comma + 1, rparen - comma - 1));

    return opt;
}

Instance readInstance(istream &in)
{
    Instance inst;
    string line;

    // JOBS
    while (getline(in, line))
    {
        line = trim(line);
        if (line.find("JOBS") != string::npos)
        {
            inst.num_jobs = stoi(line.substr(line.find(':') + 1));
        }
        else if (line.find("MACHINES") != string::npos)
        {
            inst.num_machines = stoi(line.substr(line.find(':') + 1));
        }
        else if (line.find("JOB_SECTION") != string::npos)
        {
            break;
        }
    }

    inst.jobs.resize(inst.num_jobs);

    int current_job = -1;

    while (getline(in, line))
    {
        line = trim(line);
        if (line.empty())
            continue;

        // 解析 Job
        if (line.find("Job") != string::npos)
        {
            current_job++;
            continue;
        }

        // 解析 Operation
        if (line.find("Operation") != string::npos)
        {
            Operation op;

            // 找到 ":" 后的部分
            int colon = line.find(':');
            string rest = line.substr(colon + 1);

            stringstream ss(rest);
            string token;

            while (ss >> token)
            {
                if (token.front() == '(')
                {
                    MachineOption opt = parseMachineOption(token);
                    op.machines.push_back(opt);
                }
            }

            inst.jobs[current_job].operations.push_back(op);
        }
    }

    return inst;
}

struct OperationDetail
{
    int job_id;
    int op_id;
    int machine;
    int start_time;
    int end_time;
};

int decode(const Instance &inst, const Solution &sol, vector<OperationDetail> &details)
{
    int J = inst.num_jobs;
    int n = sol.OS.size();

    vector<int> job_end(J, 0), machine_end(inst.num_machines, 0);
    details.clear();

    for (int i = 0; i < n; i++)
    {
        int j = sol.OS[i];
        int op_id = sol.op_counter[i];

        const auto &machines = inst.jobs[j].operations[op_id].machines;
        int m_idx = sol.MS[j][op_id];
        int machine = machines[m_idx].machine_id;
        int pt = machines[m_idx].proc_time;

        int start = max(job_end[j], machine_end[machine]);
        int finish = start + pt;
        job_end[j] = finish;
        machine_end[machine] = finish;

        OperationDetail od;
        od.job_id = j;
        od.op_id = op_id;
        od.machine = machine;
        od.start_time = start;
        od.end_time = finish;
        details.push_back(od);
    }

    return *max_element(job_end.begin(), job_end.end());
}

int decode(const Instance &inst, const Solution &sol)
{
    vector<OperationDetail> details;
    return decode(inst, sol, details);
}

void rebuildOpCounter(const Solution &sol, vector<int> &op_counter);

Solution initSolution(const Instance &inst)
{
    Solution sol;

    for (int j = 0; j < inst.num_jobs; j++)
    {
        for (int k = 0; k < (int)inst.jobs[j].operations.size(); k++)
        {
            sol.OS.push_back(j);
        }
    }

    random_shuffle(sol.OS.begin(), sol.OS.end());

    rebuildOpCounter(sol, sol.op_counter);

    sol.MS.resize(inst.num_jobs);
    for (int j = 0; j < inst.num_jobs; j++)
    {
        int ops = inst.jobs[j].operations.size();
        sol.MS[j].resize(ops);
        for (int o = 0; o < ops; o++)
        {
            sol.MS[j][o] = rand() % inst.jobs[j].operations[o].machines.size();
        }
    }

    sol.makespan = decode(inst, sol);
    return sol;
}

void rebuildOpCounter(const Solution &sol, vector<int> &op_counter)
{
    op_counter.assign(sol.OS.size(), 0);
    vector<int> job_occur(sol.OS.size(), 0);
    for (int i = 0; i < (int)sol.OS.size(); i++)
    {
        int j = sol.OS[i];
        op_counter[i] = job_occur[j];
        job_occur[j]++;
    }
}

void swapOp(Solution &sol)
{
    int n = sol.OS.size();
    int i = rand() % n;
    int j = rand() % n;
    swap(sol.OS[i], sol.OS[j]);
    rebuildOpCounter(sol, sol.op_counter);
}

void insertOp(Solution &sol)
{
    int n = sol.OS.size();
    int i = rand() % n;
    int j = rand() % n;

    if (i == j)
        return;

    int job = sol.OS[i];

    sol.OS.erase(sol.OS.begin() + i);
    sol.OS.insert(sol.OS.begin() + j, job);
    rebuildOpCounter(sol, sol.op_counter);
}

void changeMachine(const Instance &inst, Solution &sol)
{
    int pos = rand() % sol.OS.size();
    int j = sol.OS[pos];
    int op_id = sol.op_counter[pos];

    int sz = inst.jobs[j].operations[op_id].machines.size();
    sol.MS[j][op_id] = rand() % sz;
}

// 生成邻域解
// 传参：实例数据，当前解，当前温度，初始温度
Solution neighbor(const Instance &inst, const Solution &cur, double T, double T0)
{
    Solution nxt = cur;

    double ratio = T / T0;

    if (ratio > 0.6)
    {
        // 高温：大扰动
        if (rand() % 2)
            swapOp(nxt);
        else
            insertOp(nxt);
    }
    else
    {
        // 低温：精细
        if (rand() % 2)
            changeMachine(inst, nxt);
        else
            swapOp(nxt);
    }

    nxt.makespan = decode(inst, nxt);
    return nxt;
}

// 模拟退火算法
// 传参：实例数据
Solution simulatedAnnealing(const Instance &inst)
{
    double T0 = 1000;
    double T = T0;
    double Tmin = 1e-3;
    double alpha = 0.95;

    int L = 100 * inst.num_jobs;

    Solution cur = initSolution(inst);
    Solution best = cur;

    // 迭代直到温度降到 Tmin
    while (T > Tmin)
    {
        int accept = 0;

        for (int i = 0; i < L; i++)
        {
            Solution nxt = neighbor(inst, cur, T, T0);

            int delta = nxt.makespan - cur.makespan;

            if (delta < 0 || (double)rand() / RAND_MAX < exp(-delta / T))
            {
                cur = nxt;
                accept++;
            }

            if (cur.makespan < best.makespan)
                best = cur;
        }

        // 自适应降温
        double acc_rate = (double)accept / L;
        if (acc_rate < 0.2)
            alpha = 0.98;
        else if (acc_rate > 0.6)
            alpha = 0.90;
        else
            alpha = 0.95;

        T *= alpha;
    }

    return best;
}

int main(int argc, char* argv[])
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    bool gen_schedule = (argc > 1 && string(argv[1]) == "--schedule");

    freopen("instance.in", "r", stdin);
    if (gen_schedule)
    {
        freopen("schedule.txt", "w", stdout);
        Instance inst = readInstance(cin);
        Solution bestSol = simulatedAnnealing(inst);
        vector<OperationDetail> details;
        int makespan = decode(inst, bestSol, details);
        cout << inst.num_jobs << " " << inst.num_machines << " " << makespan << endl;
        for (const auto &d : details)
        {
            cout << d.job_id << " " << d.op_id << " " << d.machine << " " << d.start_time << " " << d.end_time << endl;
        }
    }
    else
    {
        freopen("instance.out", "w", stdout);
        Instance inst = readInstance(cin);
        cout << "Jobs = " << inst.num_jobs << endl;
        cout << "Machines = " << inst.num_machines << endl;

        const int RUNS = 20;
        vector<int> results(RUNS);
        double total_time = 0;

        for (int i = 0; i < RUNS; i++)
        {
            clock_t start = clock();
            Solution s = simulatedAnnealing(inst);
            clock_t end = clock();
            results[i] = s.makespan;
            total_time += (double)(end - start) / CLOCKS_PER_SEC;
        }

        sort(results.begin(), results.end());
        int best = results[0];
        int worst = results[RUNS - 1];

        cout << "Best makespan: " << best << endl;
        cout << "Worst makespan: " << worst << endl;
        cout << "Average makespan: " << accumulate(results.begin(), results.end(), 0) / RUNS << endl;
        cout << "Converged value: " << best << endl;
        cout << "Convergence count: " << count(results.begin(), results.end(), best) << "/" << RUNS << endl;
        cout << "Average time: " << fixed << setprecision(3) << total_time / RUNS << "s" << endl;
    }

    return 0;
}