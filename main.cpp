#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <random>
#include <fstream>
#include <time.h>
#include "include/pcg_random.hpp"
#define MAX_D 200
#define rand_eng pcg32

enum class AgentState
{
    S,
    E,
    I,
    R
};

class Agent
{
public:
    int age;
    AgentState state;

public:
    static Agent *createAgnt(int age, AgentState state)
    {
        return new Agent{age, state};
    }

private:
    Agent(int age_, AgentState state_)
        : age{age_}, state{state_} {}
};

class Net
{
public:
    int id;
    std::vector<Agent *> nodes;

    static Net *create(int id)
    {
        return new Net{id};
    }

private:
    Net(int id_) : id{id_} {}

public:
    void contact(double beta, rand_eng &rng)
    {
        for (int i = 0; i < nodes.size(); ++i)
            for (int j = i + 1; j < nodes.size(); ++j)
            {
                std::uniform_real_distribution<double> probDist(0.0, 1.0);
                double prob = probDist(rng);
                bool j2i = nodes[i]->state == AgentState::S && nodes[j]->state == AgentState::I;
                bool i2j = nodes[j]->state == AgentState::S && nodes[i]->state == AgentState::I;
                if ((i2j || j2i) && (prob < beta))
                {
                    if (j2i)
                        nodes[i]->state = AgentState::E;
                    else
                        nodes[j]->state = AgentState::E;
                }
            }
    }
};

class Env
{
public:
    int netsize;
    double beta;
    std::vector<Agent *> envAgnts;
    std::vector<Net *> nets;

public:
    void disAgnt(rand_eng &rng)
    {
        std::shuffle(envAgnts.begin(), envAgnts.end(), rng);

        for (size_t i = 0; i < envAgnts.size(); i += netsize)
        {
            Net *net = Net::create((int)nets.size());
            size_t upper = std::min(i + (size_t)netsize, envAgnts.size());
            for (size_t j = i; j < upper; ++j)
            {
                net->nodes.push_back(envAgnts[j]);
            }
            nets.push_back(net);
        }
    }
    void runEnv(rand_eng &rng, double itv)
    {
        for (Net *n : nets)
        {
            n->contact(itv * beta, rng);
        }
    }
};

class Sim
{
public:
    int population = 100000;
    int init_i = 10;
    int duration = 90;
    double sigma = 0.22, gamma = 0.08;

public:
    int cumi[MAX_D], numi[MAX_D];
    int ns[MAX_D], ni[MAX_D], ne[MAX_D], nr[MAX_D];
    std::vector<Agent *> agnts;
    std::vector<Env> envs;
    std::vector<std::pair<int, double>> itvs;
    clock_t runtime;

private:
    rand_eng rng;

public:
    void addEnv(Env newEnv) { envs.push_back(newEnv); }
    void gen()
    {
        cumi[0] = numi[0] = init_i;
        ns[0] = population;
        std::normal_distribution<double> ageDist(30.0, 10.0);
        for (int i = 0; i < init_i; ++i)
        {
            agnts.emplace_back(Agent::createAgnt(ageDist(rng), AgentState::I));
        }
        for (int i = init_i; i < population; ++i)
        {
            agnts.push_back(Agent::createAgnt(ageDist(rng), AgentState::S));
        }
    }
    void run()
    {
        // adding interventions
        // itvs.push_back({35, 0.1});
        // itvs.push_back({60, 1});

        clock_t start = clock();
        for (auto &env : envs)
            env.disAgnt(rng);

        for (int d = 1; d <= duration; ++d) // main loop
        {
            double cureff = 1.0;
            for (auto &itv : itvs)
            {
                int start_day = std::get<0>(itv);
                double effect = std::get<1>(itv);

                if (d >= start_day)
                    cureff = effect;
            }
            for (auto &env : envs)
                env.runEnv(rng, cureff);
            update(d);
        }

        runtime = clock() - start;
    }
    void prt()
    {
        std::ofstream csvFile("result.csv");
        if (!csvFile.is_open())
        {
            std::cerr << "Error opening file for writing" << std::endl;
            return;
        }

        csvFile << "day,cum_infections,n_infectious,S,E,I,R" << std::endl;
        for (int i = 0; i <= duration; ++i)
        {
            csvFile << i << "," << cumi[i] << "," << numi[i] << ","
                    << ns[i] << "," << ne[i] << "," << ni[i] << "," << nr[i]
                    << std::endl;
        }
        std::cout << "done! time: " << float(runtime) / CLOCKS_PER_SEC << " s\n"
                  << "parameters:\n"
                  << "population: " << population << "\n"
                  << "initial infectious: " << init_i << "\n"
                  << "duration: " << duration << "\n"
                  << "sigma: " << sigma << ", gamma: " << gamma
                  << std::endl;
    }
    void update(int d)
    {
        cumi[d] = cumi[d - 1];
        for (Agent *ag : agnts)
        {
            std::uniform_real_distribution<double> probDist(0.0, 1.0);
            double prob = probDist(rng);

            if (ag->state == AgentState::E && prob < sigma)
            {
                ag->state = AgentState::I;
                cumi[d]++;
            }
            if (ag->state == AgentState::I && prob < gamma)
            {
                ag->state = AgentState::R;
            }
            if (ag->state == AgentState::E)
                ne[d]++;
            if (ag->state == AgentState::I)
                numi[d]++;
            if (ag->state == AgentState::R)
                nr[d]++;
        }
        ni[d] = numi[d];
        ns[d] = population - ne[d] - ni[d] - nr[d];
    }
    Sim() : rng(std::random_device{}())
    {
        memset(cumi, 0, sizeof(cumi));
        memset(numi, 0, sizeof(numi));
        memset(ns, 0, sizeof(ns));
        memset(ne, 0, sizeof(ne));
        memset(ni, 0, sizeof(ni));
        memset(nr, 0, sizeof(nr));
    }
    ~Sim()
    {
    }
};

int main()
{
    Sim sim;

    Env home, work, schl, pblc;
    home.beta = 0.030, home.netsize = 5;
    work.beta = 0.015, work.netsize = 40;
    schl.beta = 0.015, schl.netsize = 60;
    pblc.beta = 0.005, pblc.netsize = 80;

    sim.gen();

    for (size_t i = 0; i < sim.agnts.size(); ++i)
    {
        if (sim.agnts[i]->age <= 20)
            schl.envAgnts.push_back(sim.agnts[i]);
        else
            work.envAgnts.push_back(sim.agnts[i]);
        home.envAgnts.push_back(sim.agnts[i]);
        pblc.envAgnts.push_back(sim.agnts[i]);
    }

    sim.addEnv(work);
    sim.addEnv(schl);
    sim.addEnv(pblc);
    sim.addEnv(home);

    sim.run();
    sim.prt();
}
