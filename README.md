# Agent-based Infectious Disease Model

使用 C++ 編寫的代理人基 SEIR 流行病模型。透過模擬不同社交環境中個體間的互動來追蹤疾病的傳播動態，並考慮干預措施對的影響。

## 功能概述
透過多個層物件來模擬人口中疾病的傳播：
- **Agent**：個體，具有年齡和 SEIR 狀態。
- **Net**：  由代理人組成的網絡，用於模擬網絡中的個體接觸及傳染。
- **Env**：  社交環境，包含多個網絡並負責分配代理人到各自的網絡中。
- **Sim**：  負責初始化人口、設定環境、運行模擬、記錄與輸出結果。

## 系統需求
- C++ compiler

## 參數配置
- **Sim 類別參數**：
  - population：人口總數（預設 100000 人）。
  - init_i：    初始感染者數量（預設 10 人）。
  - duration：  模擬天數（預設 90 天）。
  - sigma：     潛伏期轉感染的機率（預設 0.22）。
  - gamma：     感染者康復的機率（預設 0.08）。

- **Env 環境配置**：
  - 設置傳染參數 beta 和網絡大小 netsize，使用 sim.addEnv() 添加環境：
  - 
cpp
    Env env;
    env.beta = 0.016, env.netsize = 20;
    sim.addEnv(env);


- **干預措施**：
  - 透過 sim.itvs 設定干預策略，例如在第 35 天時降低接觸效應、在第 60 天恢復正常：
    
cpp
    sim.itvs.push_back({35, 0.1});
    sim.itvs.push_back({60, 1});

    
## 輸出結果
模擬結束後會生成一個名為 result.csv 的文件，其中包含：
- day：           模擬的第幾天
- cum_infections：累積感染人數
- n_infectious：  目前感染人數（不包含潛伏）
- S, E, I, R：    當天各狀態人數
