#include <bits/stdc++.h>
#include <time.h>
using namespace std;

struct MachineOption
{
    int machine_id;
    int proc_time;
};

struct Operation
{
    vector<MachineOption> machines;
};

struct Job
{
    vector<Operation> operations;
};

struct Instance
{
    int num_jobs;
    int num_machines;
    vector<Job> jobs;
};

struct Solution
{
    vector<int> OS;
    vector<vector<int>> MS;
    vector<int> op_counter;
    int makespan;
};

struct OperationDetail
{
    int job_id;
    int op_id;
    int machine;
    int start_time;
    int end_time;
};

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
                {
                    op.machines.push_back(parseMachineOption(token));
                }
            }
            inst.jobs[current_job].operations.push_back(op);
        }
    }
    return inst;
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

int decode(const Instance &inst, const Solution &sol)
{
    int J = inst.num_jobs;
    int n = sol.OS.size();
    vector<int> job_end(J, 0), machine_end(inst.num_machines, 0);
    for (int i = 0; i < n; i++)
    {
        int j = sol.OS[i];
        int op_id = sol.op_counter[i];
        const auto &machines = inst.jobs[j].operations[op_id].machines;
        int m_idx = sol.MS[j][op_id];
        int machine = machines[m_idx].machine_id;
        int pt = machines[m_idx].proc_time;
        int finish = max(job_end[j], machine_end[machine]) + pt;
        job_end[j] = finish;
        machine_end[machine] = finish;
    }
    return *max_element(job_end.begin(), job_end.end());
}

Solution initSolution(const Instance &inst)
{
    Solution sol;
    for (int j = 0; j < inst.num_jobs; j++)
    {
        for (int k = 0; k < (int)inst.jobs[j].operations.size(); k++)
            sol.OS.push_back(j);
    }
    random_shuffle(sol.OS.begin(), sol.OS.end());
    rebuildOpCounter(sol, sol.op_counter);
    sol.MS.resize(inst.num_jobs);
    for (int j = 0; j < inst.num_jobs; j++)
    {
        int ops = inst.jobs[j].operations.size();
        sol.MS[j].resize(ops);
        for (int o = 0; o < ops; o++)
            sol.MS[j][o] = rand() % inst.jobs[j].operations[o].machines.size();
    }
    sol.makespan = decode(inst, sol);
    return sol;
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

Solution neighbor(const Instance &inst, const Solution &cur, double T, double T0)
{
    Solution nxt = cur;
    double ratio = T / T0;
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
    nxt.makespan = decode(inst, nxt);
    return nxt;
}

Solution simulatedAnnealing(const Instance &inst, double T0, double Tmin, double alpha, int L_factor)
{
    double T = T0;
    double T_min = Tmin;
    int L = L_factor * inst.num_jobs;
    Solution cur = initSolution(inst);
    Solution best = cur;
    while (T > T_min)
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
        double acc_rate = (double)accept / L;
        if (acc_rate < 0.2)
            alpha = 0.98;
        else if (acc_rate > 0.6)
            alpha = 0.90;
        T *= (1.0 - (1.0 - alpha));
    }
    return best;
}

struct ExperimentResult
{
    string name;
    int best, worst, avg, converges;
    double time;
};

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    freopen("instance.in", "r", stdin);
    Instance inst = readInstance(cin);

    vector<ExperimentResult> results;
    struct Config { string name; double T0, Tmin, alpha; int L_factor; };
    vector<Config> configs = {
        {"T0=1000_L=100", 1000, 1e-3, 0.95, 100},
        {"T0=500_L=100", 500, 1e-3, 0.95, 100},
        {"T0=1000_L=200", 1000, 1e-3, 0.95, 200},
        {"T0=1000_L=100_alpha=0.9", 1000, 1e-3, 0.90, 100},
        {"T0=500_L=200", 500, 1e-3, 0.95, 200},
    };

    const int RUNS = 20;
    cout << "Instance: " << inst.num_jobs << " jobs, " << inst.num_machines << " machines" << endl;
    cout << "Total operations: " << inst.jobs[0].operations.size() * inst.num_jobs << endl;
    cout << endl;

    for (int ci = 0; ci < (int)configs.size(); ci++)
    {
        string name = configs[ci].name;
        double T0 = configs[ci].T0;
        double Tmin = configs[ci].Tmin;
        double alpha = configs[ci].alpha;
        int L_factor = configs[ci].L_factor;
        vector<int> run_results(RUNS);
        double total_time = 0;
        for (int i = 0; i < RUNS; i++)
        {
            clock_t start = clock();
            Solution s = simulatedAnnealing(inst, T0, Tmin, alpha, L_factor);
            clock_t end = clock();
            run_results[i] = s.makespan;
            total_time += (double)(end - start) / CLOCKS_PER_SEC;
        }
        sort(run_results.begin(), run_results.end());
        int best = run_results[0];
        int worst = run_results[RUNS - 1];
        int avg = accumulate(run_results.begin(), run_results.end(), 0) / RUNS;
        int converges = count(run_results.begin(), run_results.end(), best);
        ExperimentResult r = {name, best, worst, avg, converges, total_time / RUNS};
        results.push_back(r);
        cout << "Config: " << name << endl;
        cout << "  Best: " << best << ", Worst: " << worst << ", Avg: " << avg << endl;
        cout << "  Convergence: " << converges << "/" << RUNS << " (" << fixed << setprecision(1) << 100.0 * converges / RUNS << "%)" << endl;
        cout << "  Avg time: " << fixed << setprecision(3) << total_time / RUNS << "s" << endl;
        cout << endl;
    }

    freopen("experiment_results.txt", "w", stdout);
    cout << "Instance: " << inst.num_jobs << " jobs, " << inst.num_machines << " machines" << endl;
    cout << endl;
    cout << "Config,Best,Worst,Avg,Converges,Time" << endl;
    for (auto &r : results)
    {
        cout << r.name << "," << r.best << "," << r.worst << "," << r.avg << "," << r.converges << "," << fixed << setprecision(3) << r.time << endl;
    }

    return 0;
}