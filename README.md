# Trading Platform — C++17

Plateforme de trading algorithmique complète simulant le cycle de vie d'un trade, du signal au reporting post-trade.

---

## 1. Présentation du projet

La plateforme lit un flux CSV de données de marché, maintient un carnet d'ordres en temps réel, génère des signaux via une stratégie configurable, exécute le matching, suit le portefeuille et produit un rapport de performance complet (P&L, Sharpe, Max Drawdown, Win Rate).

Le pipeline séquentiel est le suivant :

```
CSV → MarketDataFeed → OrderBook → [snapshot] → Strategy → [signal]
                                                              ↓
                               Portfolio ← [trade] ← MatchingEngine
                                   ↓
                               Reporting
```

---

## 2. Diagramme d'architecture

```
+---------------------------+
|     MarketDataFeed        |
|  (parse CSV ligne/ligne)  |
|  émet FeedEvent :         |
|    - OrderAdd             |
|    - OrderRemove          |
+------------+--------------+
             |
             v
+---------------------------+     snapshot()      +------------------------+
|        OrderBook          | ------------------> |    Strategy Engine     |
|                           |   MarketData        |                        |
|  bids_: map desc<price>   |   {bid,ask,last,ts} |  - MeanReversion       |
|  asks_: map asc<price>    |                     |    (Bollinger Bands)   |
|  order_index_: hashmap    |                     |  - TrendFollowing      |
|                           | <----------------   |    (EMA Crossover)     |
|  match(Order)             |   Order (BUY/SELL)  |                        |
+------------+--------------+                     +------------------------+
             |
             | Trade exécuté
             v
+---------------------------+
|  Portfolio & Risk Manager |
|  cash, positions, avg cost|
|  P&L réalisé              |
|  contrôle pré-trade       |
+------------+--------------+
             |
             v
+---------------------------+
|        Reporting          |
|  P&L, Sharpe, MaxDD,      |
|  Win Rate, nb trades      |
+---------------------------+
```

### Événements échangés

| Source | Destination | Événement | Contenu |
|--------|-------------|-----------|---------|
| Feed → | OrderBook | `OrderAdd` | order_id, side, price, qty, ts |
| Feed → | OrderBook | `OrderRemove` | order_id |
| OrderBook → | Strategy | `MarketData` | best_bid, best_ask, last_price, volume, ts |
| Strategy → | OrderBook | `Order` | order_id, type, side, price, qty, symbol |
| OrderBook → | Portfolio | `Trade` | order_id, side, price, qty, ts, symbol |

---

## 3. Instructions de build

### Prérequis

- CMake ≥ 3.15
- Compilateur C++17 (GCC 9+, Clang 10+, MSVC 2019+)
- Git (pour FetchContent télécharger Google Test automatiquement)




### Compilation

```bash
git clone https://github.com/stsbof/Trading-Platform.git
cd Trading-Platform

mkdir build && cd build
cmake ..
make -j4
```

L'exécutable est généré dans `build/trading_sim`.  
Les tests sont dans `build/tests/trading_tests`.

---

## 4. Guide d'utilisation

### Lancer une simulation

```bash
cd build
./trading_sim [csv_path] [strategy]
```

| Argument | Valeur | Défaut |
|----------|--------|--------|
| `csv_path` | chemin vers le fichier CSV | `../data/market_data.csv` |
| `strategy` | `mean_reversion` ou `trend_following` | `mean_reversion` |

**Exemples :**

```bash
# Stratégie mean reversion (Bollinger Bands)
./trading_sim ../data/market_data.csv mean_reversion

# Stratégie trend following (EMA crossover)
./trading_sim ../data/market_data.csv trend_following
```

**Important :** le chemin CSV par défaut est relatif au dossier depuis lequel le programme est lancé. Le lancement recommandé est donc depuis `build/` avec `./trading_sim`. Si vous double-cliquez directement sur l'exécutable, le système peut utiliser un autre dossier de travail et le programme peut afficher `market_data.csv introuvable`. Dans ce cas, lancez depuis un terminal ou passez explicitement le chemin du CSV.

### Lancer les tests unitaires

```bash
cd build
./tests/trading_tests
```

### Modifier les paramètres

Dans `src/main.cpp`, les constantes configurables sont :

```cpp
double  initial_cash  = 1'000'000.0;  // capital initial
int64_t max_position  = 50;           // position maximale autorisée
int64_t order_qty     = 1;            // quantité par ordre
```

Et les paramètres des stratégies :

```cpp
// Mean Reversion : fenêtre=15, k=1.0
strategy = std::make_unique<MeanReversionStrategy>(15, 1.0);

// Trend Following : EMA rapide=10, EMA lente=30
strategy = std::make_unique<TrendFollowingStrategy>(10, 30);
```

---

## 5. Description des événements

### Format CSV

```
timestamp,event_type,order_id,side,price,quantity
1700000001,ADD,1001,BID,42500.00,5
1700000002,ADD,1002,ASK,42510.00,3
1700000004,REMOVE,1001,,,
```

| Champ | Type | Description |
|-------|------|-------------|
| `timestamp` | int64 | Unix timestamp |
| `event_type` | string | `ADD` ou `REMOVE` |
| `order_id` | int64 | Identifiant unique de l'ordre |
| `side` | string | `BID` ou `ASK` (vide pour REMOVE) |
| `price` | double | Prix limite (vide pour REMOVE) |
| `quantity` | int64 | Quantité (vide pour REMOVE) |

### Déroulement des événements

1. `MarketDataFeed::next_event()` lit une ligne et retourne `std::optional<FeedEvent>` où `FeedEvent = std::variant<OrderAdd, OrderRemove>`.
2. `std::visit` dispatch vers `book.on_add()` ou `book.on_remove()`.
3. `book.snapshot()` génère un `MarketData` avec best_bid, best_ask, last_price.
4. La stratégie reçoit ce `MarketData` et retourne un `Signal`.
5. Si signal ≠ HOLD, un `Order` est construit et passé à `book.match()`.
6. `match()` retourne `std::optional<Trade>` (nullopt si pas de liquidité ou prix incompatible).
7. Le trade est enregistré dans `Portfolio::on_trade()`.

---

## 6. Description des stratégies

### MeanReversionStrategy — Bollinger Bands

**Logique :** Le prix a tendance à revenir vers sa moyenne. On achète quand il s'éloigne trop en dessous, on vend quand il s'éloigne trop en dessus.

**Paramètres :**
- `window` (défaut: 15) : taille de la fenêtre glissante
- `k` (défaut: 1.0) : nombre d'écarts-types pour les bandes

**Signaux :**
- `BUY`  si `mid_price < mean - k × stddev`
- `SELL` si `mid_price > mean + k × stddev`
- `HOLD` sinon ou si la fenêtre n'est pas encore remplie

**Implémentation :** utilise `TimeSeries<double>` (template) pour le calcul en fenêtre glissante.

---

### TrendFollowingStrategy — EMA Crossover

**Logique :** On suit la tendance. Quand l'EMA rapide croise l'EMA lente vers le haut, on achète. Quand elle croise vers le bas, on vend.

**Paramètres :**
- `fast_period` (défaut: 10) : période EMA rapide
- `slow_period` (défaut: 30) : période EMA lente
- `α = 2 / (period + 1)` : facteur de lissage

**Signaux :**
- `BUY`  au croisement ascendant (fast > slow, alors que fast ≤ slow à t-1)
- `SELL` au croisement descendant
- `HOLD` sinon ou pendant le warm-up (ticks < slow_period)

---

## 7. Résultats

Exemple de sortie sur le fichier fourni `data/market_data.csv` (300 itérations sinusoïdales, amplitude ±800, période 50) :

### Trend Following (EMA 10/30)

```
[TRADE #  1] BUY   qty=1  @42799.50  pos=1   pnl=0.00
[TRADE #  2] SELL  qty=1  @43293.42  pos=0   pnl=493.92
[TRADE #  3] BUY   qty=1  @41706.58  pos=1   pnl=493.92
[TRADE #  4] SELL  qty=1  @43293.42  pos=0   pnl=2080.76
...

Simulation complete. Events: 1184 | Trades: 13

=======================================================
  POST-TRADE REPORT  |  Strategy: TrendFollowing-EMACrossover
=======================================================
  P&L Total (realised)           8428.1200
  Number of Trades                      13
  Sharpe Ratio (annualised)        54.8475
  Max Drawdown                      0.0000
  Win Rate                       100.0000%
=======================================================
```

> Le Sharpe élevé s'explique par la nature synthétique des données (sinusoïde parfaite), en données réelles il serait normalement bien inférieur.

### Mean Reversion (Bollinger Bands, window=15, k=1.0)

```
Simulation complete. Events: 1184 | Trades: 578

=======================================================
  POST-TRADE REPORT  |  Strategy: MeanReversion-BollingerBands
=======================================================
  P&L Total (realised)         -72334.3296
  Number of Trades                     578
  Sharpe Ratio (annualised)       -12.3736
  Max Drawdown                  72334.3296
  Win Rate                        25.1799%
=======================================================
```

La mean reversion sous-performe sur un marché sinusoïdal (tendance claire), tandis que la trend following excelle.

**Note méthodologique — Sharpe :** le Sharpe est calculé sur les **rendements en %** par trade fermant (`r_t = PnL_t / equity_{t-1}`), conformément à la formule suivante `S = (r̄ − rf) / σr × √252`.

---

## 8. Comment ajouter une nouvelle stratégie

Il suffit d'hériter de `Strategy` et d'implémenter deux méthodes :

**Étape 1 — Créer le header `include/MyStrategy.h` :**

```cpp
#pragma once
#include "Strategy.h"

class MyStrategy : public Strategy {
public:
    explicit MyStrategy(/* vos paramètres */);
    Signal on_market_data(const MarketData& data) override;
    std::string name() const override { return "MyStrategy"; }
private:
    // votre état interne
};
```

**Étape 2 — Implémenter `src/MyStrategy.cpp` :**

```cpp
#include "MyStrategy.h"

Signal MyStrategy::on_market_data(const MarketData& data) {
    double mid = (data.best_bid + data.best_ask) / 2.0;
    // votre logique ici...
    if (/* condition achat */) return Signal::BUY;
    if (/* condition vente */) return Signal::SELL;
    return Signal::HOLD;
}
```

**Étape 3 — Ajouter à `CMakeLists.txt` dans `LIB_SOURCES` :**

```cmake
set(LIB_SOURCES
    ...
    src/MyStrategy.cpp   # ← ajouter ici
)
```

**Étape 4 — Utiliser dans `main.cpp` :**

```cpp
#include "MyStrategy.h"
// ...
if (strategy_name == "my_strategy")
    strategy = std::make_unique<MyStrategy>(/* params */);
```

**Étape 5 — Compiler et lancer :**

```bash
cd build && make -j4
./trading_sim ../data/market_data.csv my_strategy
```

Aucune modification du reste de la plateforme n'est nécessaire vu que le polymorphisme s'occupe du reste.

---

## Structure du projet

```
TradingPlatform/
├── CMakeLists.txt
├── README.md
├── data/
│   └── market_data.csv
├── include/
│   ├── Types.h                  # Order, Trade, Signal, MarketData, enums
│   ├── Events.h                 # FeedEvent = std::variant<OrderAdd,OrderRemove>
│   ├── TimeSeries.h             # Template TimeSeries<T> — fenêtre glissante
│   ├── MarketDataFeed.h
│   ├── OrderBook.h
│   ├── Strategy.h               # Classe abstraite
│   ├── MeanReversionStrategy.h
│   ├── TrendFollowingStrategy.h
│   ├── Portfolio.h
│   └── Reporting.h
├── src/
│   ├── main.cpp
│   ├── MarketDataFeed.cpp
│   ├── OrderBook.cpp
│   ├── MeanReversionStrategy.cpp
│   ├── TrendFollowingStrategy.cpp
│   ├── Portfolio.cpp
│   └── Reporting.cpp
└── tests/
    ├── CMakeLists.txt
    ├── test_order_book.cpp
    ├── test_market_feed.cpp
    └── test_portfolio.cpp
```
