// 基于极限完工时间最小化的FJSP模拟退火算法
#include <bits/stdc++.h>
#include <time.h>
using namespace std;

// 机器选项：某道工序可在不同机器上加工，每台机器加工时间不同
struct MachineOption
{
    int machine_id; // 机器编号
    int proc_time;  // 该机器上的加工时间
};

// 工序：包含所有可选机器列表
struct Operation
{
    vector<MachineOption> machines;
};

// 工件：由多个有序工序组成
struct Job
{
    vector<Operation> operations;
};

// 问题实例：包含工件和机器信息
struct Instance
{
    int num_jobs;     // 工件数量
    int num_machines; // 机器数量
    vector<Job> jobs; // 各工件工序信息
};

// 解：工序序列OS + 机器选择MS
struct Solution
{
    vector<int> OS;         // 工序序列，按顺序排列各job的工序
    vector<vector<int>> MS; // MS[j][op] = 选择的机器索引
    vector<int> op_counter; // 缓存：OS中该位置是第几次出现该job
    int makespan;          // 目标函数值：最大完工时间
};

// 去除换行符
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

// 解析 "M0,147" 格式的机器选项
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

// 从输入流解析问题实例
Instance readInstance(istream &in)
{
    Instance inst;
    string line;

    // 读取 JOBS, MACHINES, JOB_SECTION
    while (getline(in, line))
    {
        line = trim(line);
        if (line.find("JOBS") != string::npos)
            inst.num_jobs = stoi(line.substr(line.find(':') + 1));
        else if (line.find("MACHINES") != string::npos)
            inst.num_machines = stoi(line.substr(line.find(':') + 1));
        else if (line.find("JOB_SECTION") != string::npos)
            break;
    }

    inst.jobs.resize(inst.num_jobs);
    int current_job = -1;

    // 读取每个Job和Operation
    while (getline(in, line))
    {
        line = trim(line);
        if (line.empty())
            continue;

        if (line.find("Job") != string::npos)
        {
            current_job++;
            continue;
        }

        if (line.find("Operation") != string::npos)
        {
            Operation op;
            int colon = line.find(':');
            string rest = line.substr(colon + 1);
            stringstream ss(rest);
            string token;
            while (ss >> token)
            {
                if (token.front() == '(')
                    op.machines.push_back(parseMachineOption(token));
            }
            inst.jobs[current_job].operations.push_back(op);
        }
    }

    return inst;
}

// 调度详情：每道工序的分配信息
struct OperationDetail
{
    int job_id;      // 工件编号（0起）
    int op_id;       // 工序序号（0起）
    int machine;    // 分配的机器
    int start_time; // 开始时间
    int end_time;   // 结束时间
};

// 解码：根据OS和MS计算调度，返回makespan
int decode(const Instance &inst, const Solution &sol, vector<OperationDetail> &details)
{
    int J = inst.num_jobs;
    int n = sol.OS.size();

    // 跟踪每个工件和机器的完工时间
    vector<int> job_end(J, 0), machine_end(inst.num_machines, 0);
    details.clear();

    // 依次处理每道工序
    for (int i = 0; i < n; i++)
    {
        int j = sol.OS[i];           // 当前工序属于工件j
        int op_id = sol.op_counter[i]; // 该工件的第几道工序
        const auto &machines = inst.jobs[j].operations[op_id].machines;
        int m_idx = sol.MS[j][op_id]; // 选择的机器索引
        int machine = machines[m_idx].machine_id;
        int pt = machines[m_idx].proc_time;

        // 找到该工序的最早可开始时间
        int start = max(job_end[j], machine_end[machine]);
        int finish = start + pt;
        job_end[j] = finish;
        machine_end[machine] = finish;

        // 记录调度详情
        details.push_back({j, op_id, machine, start, finish});
    }

    // makespan = 所有工件的 max 完工时间
    return *max_element(job_end.begin(), job_end.end());
}

// 仅计算makespan的简易接口
int decode(const Instance &inst, const Solution &sol)
{
    vector<OperationDetail> details;
    return decode(inst, sol, details);
}

void rebuildOpCounter(const Solution &sol, vector<int> &op_counter);

// 生成初始解：随机工序序列 + 随机机器选择
Solution initSolution(const Instance &inst)
{
    Solution sol;

    // 构造OS：每个job的所有工序依次加入
    for (int j = 0; j < inst.num_jobs; j++)
    {
        for (int k = 0; k < (int)inst.jobs[j].operations.size(); k++)
        {
            sol.OS.push_back(j);
        }
    }

    // 随机打乱工序顺序
    random_shuffle(sol.OS.begin(), sol.OS.end());

    // 重建op_counter用于快速定位
    rebuildOpCounter(sol, sol.op_counter);

    // 为每个工序随机选择机器
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

    // 计算初始makespan
    sol.makespan = decode(inst, sol);
    return sol;
}

// 重建op_counter：统计OS中每个位置是第几次出现该job
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

// 邻域操作1：交换两个工序的位置
void swapOp(Solution &sol)
{
    int n = sol.OS.size();
    int i = rand() % n;
    int j = rand() % n;
    swap(sol.OS[i], sol.OS[j]);
    rebuildOpCounter(sol, sol.op_counter);
}

// 邻域操作2：将一个工序插入到新位置
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

// 邻域操作3：更改某工序的机器选择
void changeMachine(const Instance &inst, Solution &sol)
{
    int pos = rand() % sol.OS.size();
    int j = sol.OS[pos];
    int op_id = sol.op_counter[pos];

    int sz = inst.jobs[j].operations[op_id].machines.size();
    sol.MS[j][op_id] = rand() % sz;
}

// 生成邻域解：根据温度选择扰动程度
Solution neighbor(const Instance &inst, const Solution &cur, double T, double T0)
{
    Solution nxt = cur;

    double ratio = T / T0;

    // 高温时采用大扰动（swap/insert），低温时精细调整（changeMachine）
    if (ratio > 0.6)
    {
        if (rand() % 2)
            swapOp(nxt);
        else
            insertOp(nxt);
    }
    else
    {
        if (rand() % 2)
            changeMachine(inst, nxt);
        else
            swapOp(nxt);
    }

    // 计算新解的makespan
    nxt.makespan = decode(inst, nxt);
    return nxt;
}

// 模拟退火算法主函数
Solution simulatedAnnealing(const Instance &inst)
{
    // 算法参数
    double T0 = 1000;     // 初始温度
    double T = T0;
    double Tmin = 1e-3;   // 终止温度
    double alpha = 0.95;   // 降温系数

    int L = 100 * inst.num_jobs;  // 每个温度下的迭代次数

    Solution cur = initSolution(inst);
    Solution best = cur;

    // 外循环：降温过程
    while (T > Tmin)
    {
        int accept = 0;

        // 内循环：在当前温度下搜索
        for (int i = 0; i < L; i++)
        {
            Solution nxt = neighbor(inst, cur, T, T0);

            int delta = nxt.makespan - cur.makespan;

            // Metropolis准则：劣解以概率接受
            if (delta < 0 || (double)rand() / RAND_MAX < exp(-delta / T))
            {
                cur = nxt;
                accept++;
            }

            // 更新全局最优
            if (cur.makespan < best.makespan)
                best = cur;
        }

        // 自适应调整降温速度：根据接受率调整alpha
        double acc_rate = (double)accept / L;
        if (acc_rate < 0.2)
            alpha = 0.98;    // 接受率低，减慢降温
        else if (acc_rate > 0.6)
            alpha = 0.90;    // 接受率高，加快降温
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

    // --schedule 模式：输出调度详情用于绘图
    bool gen_schedule = (argc > 1 && string(argv[1]) == "--schedule");

    freopen("instance.in", "r", stdin);
    Instance inst = readInstance(cin);

    if (gen_schedule)
    {
        // 运行多次SA取最优解，输出详细调度信息
        freopen("schedule.txt", "w", stdout);
        const int RUNS = 20;
        Solution bestSol;
        int bestMs = INT_MAX;
        for (int i = 0; i < RUNS; i++)
        {
            Solution s = simulatedAnnealing(inst);
            if (s.makespan < bestMs)
            {
                bestMs = s.makespan;
                bestSol = s;
            }
        }
        // 输出：工件数 机器数 makespan
        vector<OperationDetail> details;
        int makespan = decode(inst, bestSol, details);
        cout << inst.num_jobs << " " << inst.num_machines << " " << makespan << endl;
        // 输出每道工序：job op machine start end
        for (const auto &d : details)
        {
            cout << d.job_id << " " << d.op_id << " " << d.machine << " " << d.start_time << " " << d.end_time << endl;
        }
    }
    else
    {
        // 默认模式：运行多次SA，统计收敛性能
        freopen("instance.out", "w", stdout);
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

        cout << "Jobs = " << inst.num_jobs << endl;
        cout << "Machines = " << inst.num_machines << endl;
        cout << "Best makespan: " << best << endl;
        cout << "Worst makespan: " << worst << endl;
        cout << "Average makespan: " << accumulate(results.begin(), results.end(), 0) / RUNS << endl;
        cout << "Converged value: " << best << endl;
        cout << "Convergence count: " << count(results.begin(), results.end(), best) << "/" << RUNS << endl;
        cout << "Average time: " << fixed << setprecision(3) << total_time / RUNS << "s" << endl;
    }

    return 0;
}