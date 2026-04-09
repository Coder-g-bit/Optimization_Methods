// 基于极限完工时间最小化的初始化方式实现FJSP问题的模拟退火算法
#include <bits/stdc++.h>
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

// 解码 + Makespan计算（使用Giffler–Thompson-like 解码）
// 传参：实例数据，解结构
int decode(const Instance &inst, Solution &sol)
{
    int J = inst.num_jobs;

    vector<int> job_ptr(J, 0); // 每个job当前工序
    vector<int> job_end(J, 0); // job时间
    vector<int> machine_end(inst.num_machines, 0);

    for (int i = 0; i < sol.OS.size(); i++)
    {
        int j = sol.OS[i];
        int op_id = job_ptr[j];

        auto &op = inst.jobs[j].operations[op_id];
        int m_idx = sol.MS[j][op_id]; // 机器选择（在候选列表中的index）

        int machine = op.machines[m_idx].machine_id;
        int pt = op.machines[m_idx].proc_time;

        int start = max(job_end[j], machine_end[machine]);
        int finish = start + pt;

        job_end[j] = finish;
        machine_end[machine] = finish;

        job_ptr[j]++;
    }

    return *max_element(job_end.begin(), job_end.end());
}

// 生成初始解
// 传参：实例数据
Solution initSolution(const Instance &inst)
{
    Solution sol;

    // 构造OS
    for (int j = 0; j < inst.num_jobs; j++)
    {
        for (int k = 0; k < inst.jobs[j].operations.size(); k++)
        {
            sol.OS.push_back(j);
        }
    }

    random_shuffle(sol.OS.begin(), sol.OS.end());

    // 构造MS（随机选机器）
    for (int i = 0; i < sol.OS.size(); i++)
    {
        int j = sol.OS[i];
        int op_id = count(sol.OS.begin(), sol.OS.begin() + i, j);

        int sz = inst.jobs[j].operations[op_id].machines.size();
        sol.MS.resize(inst.num_jobs);

        for (int j = 0; j < inst.num_jobs; j++)
        {
            int ops = inst.jobs[j].operations.size();
            sol.MS[j].resize(ops);

            for (int o = 0; o < ops; o++)
            {
                int sz = inst.jobs[j].operations[o].machines.size();
                sol.MS[j][o] = rand() % sz;
            }
        }
    }

    sol.makespan = decode(inst, sol);
    return sol;
}

// 交换操作
// 传参：当前解
void swapOp(Solution &sol)
{
    int n = sol.OS.size();
    int i = rand() % n;
    int j = rand() % n;
    swap(sol.OS[i], sol.OS[j]);
}

// 插入操作
// 传参：当前解
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
}

// 修改机器选择
// 传参：实例数据，当前解
void changeMachine(const Instance &inst, Solution &sol)
{
    int i = rand() % sol.OS.size();

    int j = sol.OS[i];
    int op_id = count(sol.OS.begin(), sol.OS.begin() + i, j);

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

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    freopen("instance.in", "r", stdin);
    freopen("instance.out", "w", stdout);
    Instance inst = readInstance(cin);
    cout << "Jobs = " << inst.num_jobs << endl;
    cout << "Machines = " << inst.num_machines << endl;
    int best = INT_MAX;
    for(int i=0;i<10;++i){
        Solution s = simulatedAnnealing(inst);
        best = min(best, s.makespan);
    }
    cout << "Best makespan: " << best << "\n";
    return 0;
}