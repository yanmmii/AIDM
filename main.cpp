#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <random>
#include <fstream>
#include <time.h>
#define MAX_D 200

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

public:
    void contact(Agent *a, Agent *b, double beta, double prob)
    {
        if ((a->state == AgentState::I && b->state == AgentState::S) ||
            (b->state == AgentState::I && a->state == AgentState::S))
        {

            if (prob < beta)
            {
                if (a->state == AgentState::S)
                    a->state = AgentState::E;
                else
                    b->state = AgentState::E;
            }
        }
    }
    void interact(double beta, std::default_random_engine &rng)
    {
        for (int i = 0; i < nodes.size(); ++i)
            for (int j = i + 1; j < nodes.size(); ++j)
            {
                std::uniform_real_distribution<double> probDist(0.0, 1.0);
                double prob = probDist(rng);
                contact(nodes[i], nodes[j], beta, prob);
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
    void disAgnt(std::default_random_engine &rng)
    {
        std::shuffle(envAgnts.begin(), envAgnts.end(), rng);

        for (size_t i = 0; i < envAgnts.size(); i += netsize)
        {
            Net *n = new Net;
            n->id = nets.size();
            size_t upper = std::min(i + (size_t)netsize, envAgnts.size());
            for (size_t j = i; j < upper; ++j)
            {
                n->nodes.push_back(envAgnts[j]);
            }
            nets.push_back(n);
        }
    }
    void runEnv(std::default_random_engine &rng, double itv)
    {
        for (Net *n : nets)
        {
            n->interact(itv * beta, rng);
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
    std::default_random_engine rng;
    clock_t runtime;

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
        clock_t start = clock();
        for (auto &env : envs)
        {
            env.disAgnt(rng);
        }
        for (int d = 1; d <= duration; ++d)
        {
            double current_effect = 1.0;
            for (const auto &itv : itvs)
            {
                int start_day = std::get<0>(itv);
                double effect = std::get<1>(itv);

                if (d >= start_day)
                    current_effect = effect;
            }
            for (auto &env : envs)
            {
                env.runEnv(rng, current_effect);
            }
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

    // interventions:
    sim.itvs.push_back({35, 0.1});
    sim.itvs.push_back({60, 1});

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
