#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Windows.h>
#include <cstdio>
#include <algorithm>
#include <deque>

using namespace geode::prelude;

// ============================================================================
//  USER SETTINGS
// ============================================================================

// True Wave Spam Guard: only bypasses the buffer if you do at least
// SPAM_STREAK_REQUIRED consecutive inputs faster than SPAM_INTERVAL_TICKS apart.
// This keeps isolated micro-clicks and spike/sawblade dodges 100% protected!
constexpr int SPAM_INTERVAL_TICKS = 8;
constexpr int SPAM_STREAK_REQUIRED = 3;

// ============================================================================

class TrajectoryBufferSim {
private:
    PlayerObject* m_simPlayer1 = nullptr;
    PlayerObject* m_simPlayer2 = nullptr;

    bool m_simulating = false;
    bool m_firingBuffered = false;
    bool m_simulationDead = false;

    // Tracks actual physical mouse state vs what PlayLayer currently has active
    bool m_physicalHolding = false;
    bool m_gameHolding = false;
    // If death occurs while the physical key is held, wait for a real release
    // before accepting another press. This prevents synthetic re-presses on respawn.
    bool m_waitForReleaseAfterDeath = false;

    // F6 cycle starts on Stage 1: 1 tick Normal / 1 tick Wave.
    int  m_presetIndex = 0;
    bool m_enabled = true;
    // Experimental ship helper remains disabled; F8 also explicitly forces it OFF.
    bool m_shipPrecisionEnabled = false;
    int  m_maxTicksNormal = 1;
    int  m_maxTicksWave = 1;

    // Buffer state
    int  m_waitTicks = -1;          // -1 = inactive, >= 0 = countdown ticks, -2 = queued opposite tick
    bool m_bufferedPush = true;     // true = buffered PRESS, false = buffered RELEASE
    std::deque<bool> m_queuedInputs; // preserve every alternating edge while a buffered input is pending
    float m_frameDt = 0.25f;        // GD 240 TPS normalized dt is 0.25f

    // Streak-based Spam Guard state
    int  m_ticksSinceLastInput = 999;
    int  m_rapidInputStreak = 0;

    PlayerObject* createTrajectoryPlayer() {
        PlayLayer* pl = PlayLayer::get();
        if (!pl) return nullptr;

        PlayerObject* player = PlayerObject::create(1, 1, pl, pl, true);
        player->retain();
        player->setPosition({0, 105});
        player->setVisible(false);
        pl->m_objectLayer->addChild(player);
        return player;
    }

    void resetCollisionLog(PlayerObject* self) {
        self->m_collisionLogTop->removeAllObjects();
        self->m_collisionLogBottom->removeAllObjects();
        self->m_collisionLogLeft->removeAllObjects();
        self->m_collisionLogRight->removeAllObjects();
    }

    // Matches Show Trajectory's exact attribute + hitbox size synchronization
    void syncSimPlayer(PlayerObject* sim, PlayerObject* real) {
        sim->setVisible(false);
        sim->copyAttributes(real);
        sim->setScale(real->getScale());
        sim->setContentSize(real->getContentSize());
        sim->m_width = real->m_width;
        sim->m_height = real->m_height;
        sim->m_gravityMod = real->m_gravityMod;
        sim->m_isOnGround = real->m_isOnGround;
        sim->m_isUpsideDown = real->m_isUpsideDown;
        sim->m_vehicleSize = real->m_vehicleSize;
        sim->m_playerSpeed = real->m_playerSpeed;
    }

    float getValidDt() const {
        if (m_frameDt > 0.001f && m_frameDt < 2.0f) {
            return m_frameDt;
        }
        return 0.25f;
    }

    // Clear transient input state between attempts. In particular, never carry
    // a buffered edge or rapid-input streak through death/restart.
    void resetAttemptInputState(bool physicalHeld = false) {
        m_waitTicks = -1;
        m_bufferedPush = true;
        m_queuedInputs.clear();
        m_physicalHolding = physicalHeld;
        m_gameHolding = false;
        m_waitForReleaseAfterDeath = physicalHeld;
        m_firingBuffered = false;
        m_ticksSinceLastInput = 999;
        m_rapidInputStreak = 0;
        m_simulating = false;
        m_simulationDead = false;
    }

public:
    void init() {
        PlayLayer* pl = PlayLayer::get();
        if (!pl) return;

        m_simPlayer1 = createTrajectoryPlayer();
        m_simPlayer2 = createTrajectoryPlayer();
        resetAttemptInputState(false);
    }

    void quit() {
        if (m_simPlayer1) {
            m_simPlayer1->release();
            m_simPlayer1 = nullptr;
        }
        if (m_simPlayer2) {
            m_simPlayer2->release();
            m_simPlayer2 = nullptr;
        }
        resetAttemptInputState(false);
    }

    // Cycles through 6 F6 stages:
    // 1: 1/1 -> 2: 2/2 -> 3: 4/4 -> 4: 8/8 -> 5: 16/8 -> 6: OFF.
    void cyclePreset() {
        m_presetIndex = (m_presetIndex + 1) % 6;
        m_waitTicks = -1;
        m_queuedInputs.clear();

        switch (m_presetIndex) {
            case 0:
                m_enabled = true;
                m_maxTicksNormal = 1;
                m_maxTicksWave = 1;
                printf("[InputBuffer] F6 Cycle -> STAGE 1: 1 Tick All | 1 Tick Wave (0.25f @ 60fps)\n");
                break;
            case 1:
                m_enabled = true;
                m_maxTicksNormal = 2;
                m_maxTicksWave = 2;
                printf("[InputBuffer] F6 Cycle -> STAGE 2: 2 Ticks All | 2 Ticks Wave (0.5f @ 60fps)\n");
                break;
            case 2:
                m_enabled = true;
                m_maxTicksNormal = 4;
                m_maxTicksWave = 4;
                printf("[InputBuffer] F6 Cycle -> STAGE 3: 4 Ticks All | 4 Ticks Wave (1.0f @ 60fps)\n");
                break;
            case 3:
                m_enabled = true;
                m_maxTicksNormal = 8;
                m_maxTicksWave = 8;
                printf("[InputBuffer] F6 Cycle -> STAGE 4: 8 Ticks All | 8 Ticks Wave (2.0f @ 60fps)\n");
                break;
            case 4:
                m_enabled = true;
                m_maxTicksNormal = 16;
                m_maxTicksWave = 8;
                printf("[InputBuffer] F6 Cycle -> STAGE 5: 16 Ticks All | 8 Ticks Wave (4.0f / 2.0f @ 60fps)\n");
                break;
            case 5:
                m_enabled = false;
                m_maxTicksNormal = 0;
                m_maxTicksWave = 0;
                printf("[InputBuffer] F6 Cycle -> STAGE 6: OFF (100%% Raw Input)\n");
                break;
        }
        fflush(stdout);
    }

    bool isSimulating() const { return m_simulating; }
    bool isFiringBuffered() const { return m_firingBuffered; }

    bool checkSimulationDeath(PlayerObject* player) {
        if (m_simulating && (player == m_simPlayer1 || player == m_simPlayer2)) {
            m_simulationDead = true;
            return true;
        }
        // Real player died -> discard pending actions. If the physical key is
        // still held, require a genuine release before allowing a new press.
        // This avoids the game-state resync synthesizing a press after respawn.
        resetAttemptInputState(m_physicalHolding);
        return false;
    }

    void setFrameDelta(float dt) {
        PlayLayer* pl = PlayLayer::get();
        if (pl && pl->m_gameState.m_timeWarp > 0.f) {
            m_frameDt = dt / pl->m_gameState.m_timeWarp;
        } else {
            m_frameDt = dt;
        }
    }

    int getSurvivalTicks(PlayLayer* pl, PlayerObject* realP1, PlayerObject* realP2, bool isDual, int delayFrames, bool actionPush, int maxTicks) {
        if (!pl || !m_simPlayer1 || !realP1) return maxTicks;

        m_simulating = true;
        m_simulationDead = false;

        syncSimPlayer(m_simPlayer1, realP1);
        if (isDual && m_simPlayer2 && realP2) {
            syncSimPlayer(m_simPlayer2, realP2);
        }

        // Before delayFrames, maintain the pre-input button state
        if (delayFrames > 0) {
            if (!actionPush) {
                m_simPlayer1->pushButton(PlayerButton::Jump);
                if (isDual && m_simPlayer2) {
                    m_simPlayer2->pushButton(PlayerButton::Jump);
                }
            } else {
                m_simPlayer1->releaseButton(PlayerButton::Jump);
                if (isDual && m_simPlayer2) {
                    m_simPlayer2->releaseButton(PlayerButton::Jump);
                }
            }
        }

        float dt = getValidDt();
        int survived = 0;
        bool isContinuousMode = realP1->m_isDart || realP1->m_isBird || realP1->m_isSwing || realP1->m_isShip ||
                                (isDual && realP2 && (realP2->m_isDart || realP2->m_isBird || realP2->m_isSwing || realP2->m_isShip));
        int airborneTicks = 0;
        int safeLandedTicks = 0;

        for (int i = 0; i < delayFrames + maxTicks; i++) {
            if (i == delayFrames) {
                if (actionPush) {
                    m_simPlayer1->pushButton(PlayerButton::Jump);
                    if (isDual && m_simPlayer2) {
                        m_simPlayer2->pushButton(PlayerButton::Jump);
                    }
                } else {
                    m_simPlayer1->releaseButton(PlayerButton::Jump);
                    if (isDual && m_simPlayer2) {
                        m_simPlayer2->releaseButton(PlayerButton::Jump);
                    }
                }
            }

            m_simPlayer1->update(dt);
            m_simPlayer1->updateRotation(dt);
            if (isDual && m_simPlayer2) {
                m_simPlayer2->update(dt);
                m_simPlayer2->updateRotation(dt);
            }
            if (m_simulationDead) break;

            resetCollisionLog(m_simPlayer1);
            pl->checkCollisions(m_simPlayer1, dt, false);
            if (m_simulationDead) break;

            if (isDual && m_simPlayer2 && realP2) {
                resetCollisionLog(m_simPlayer2);
                pl->checkCollisions(m_simPlayer2, dt, false);
                if (m_simulationDead) break;
            }

            if (i >= delayFrames) {
                survived++;
                if (!isContinuousMode) {
                    bool p1Grounded = m_simPlayer1->m_isOnGround;
                    bool p2Grounded = (!isDual || !m_simPlayer2) ? true : m_simPlayer2->m_isOnGround;
                    if (!p1Grounded || !p2Grounded) {
                        airborneTicks++;
                        safeLandedTicks = 0;
                    } else if (airborneTicks >= 15) {
                        safeLandedTicks++;
                        if (safeLandedTicks >= 10) {
                            survived = maxTicks;
                            break;
                        }
                    }
                }
            }
        }

        m_simulating = false;
        return survived;
    }

    bool interceptInput(GJBaseGameLayer* layer, bool push, bool isPlayer1) {
        PlayLayer* pl = PlayLayer::get();
        if (!pl) return true;

        bool isDual = pl->m_gameState.m_isDualMode && pl->m_player2 != nullptr;
        bool isTwoPlayerControl = pl->m_level && pl->m_level->m_twoPlayerMode;

        // Keep Player 2 synced with Player 1 in 1-player Dual mode
        if (!isPlayer1) {
            if (isDual && !isTwoPlayerControl && (m_waitTicks >= 0 || m_waitTicks == -2)) {
                return false;
            }
            return true;
        }

        // Capture the prior physical state before updating it. Some input paths
        // can report the same button edge more than once; treating a duplicate
        // DOWN as a fresh edge can create an extra click (e.g. 1 CPS -> 2 CPS).
        const bool wasPhysicalHolding = m_physicalHolding;
        m_physicalHolding = push;

        // After a death during a held input, ignore presses until a real release
        // arrives. Pass the release through and then re-arm the buffer.
        if (m_waitForReleaseAfterDeath) {
            if (!push) {
                m_waitForReleaseAfterDeath = false;
                m_gameHolding = false;
                m_waitTicks = -1;
                m_queuedInputs.clear();
                m_ticksSinceLastInput = 999;
                m_rapidInputStreak = 0;
                return true;
            }
            return false;
        }

        if (!m_enabled) {
            m_gameHolding = push;
            return true;
        }

        // Only real state transitions are valid edges. Consume duplicate DOWN /
        // UP notifications while the buffer is active, before they can enter the
        // prediction path or queue. Raw-input mode (F6 OFF) remains untouched.
        if (push == wasPhysicalHolding) {
            return false;
        }

        PlayerObject* p1 = layer->m_player1;
        PlayerObject* p2 = isDual ? pl->m_player2 : nullptr;
        if (!p1 || p1->m_isDead) {
            // Inputs during death/restart are passed through raw, but must not
            // leave a pending buffered action or spam-guard history behind.
            m_waitTicks = -1;
            m_queuedInputs.clear();
            m_bufferedPush = true;
            m_gameHolding = push;
            m_ticksSinceLastInput = 999;
            m_rapidInputStreak = 0;
            return true;
        }

        // Ship stays on raw input. This helper is intentionally disabled; no ship
        // inputs are buffered by the current build.
        bool isShip = p1->m_isShip || (isDual && p2 && p2->m_isShip);
        if (isShip && !m_shipPrecisionEnabled) {
            m_waitTicks = -1;
            m_queuedInputs.clear();
            m_gameHolding = push;
            return true;
        }

        // Keep edge order while a predicted input is pending. If another physical
        // edge arrives before the prediction delay expires, flush the pending edge
        // and all queued transitions immediately, in order. Waiting for one edge per
        // PlayerObject::update can make a tight Wave press/release arrive too late.
        auto enqueueEdge = [this](bool state) {
            bool lastScheduled = m_queuedInputs.empty()
                ? (m_waitTicks >= 0 ? m_bufferedPush : m_gameHolding)
                : m_queuedInputs.back();
            if (state != lastScheduled) m_queuedInputs.push_back(state);
        };
        auto flushPendingEdges = [this, pl]() {
            if (m_waitTicks >= 0) {
                const bool pendingState = m_bufferedPush;
                m_waitTicks = -1;
                if (m_gameHolding != pendingState) {
                    m_SimFireButton(pl, pendingState);
                }
            }
            while (!m_queuedInputs.empty()) {
                const bool nextState = m_queuedInputs.front();
                m_queuedInputs.pop_front();
                if (m_gameHolding != nextState) {
                    m_SimFireButton(pl, nextState);
                }
            }
            // The physical state is authoritative; recover if the queue and game
            // state somehow diverged while processing a burst of edges.
            if (m_gameHolding != m_physicalHolding) {
                m_SimFireButton(pl, m_physicalHolding);
            }
        };

        if (m_waitTicks >= 0) {
            enqueueEdge(push);
            flushPendingEdges();
            m_ticksSinceLastInput = 0;
            return false;
        }

        // A queue should normally be empty outside a pending prediction. If an
        // older pending sequence exists, resolve it now rather than dropping a
        // new Wave edge or leaving the queue behind the real input.
        if (!m_queuedInputs.empty()) {
            enqueueEdge(push);
            flushPendingEdges();
            m_ticksSinceLastInput = 0;
            return false;
        }

        bool isWave = p1->m_isDart || (isDual && p2 && p2->m_isDart);
        bool isMiniWave = (p1->m_isDart && p1->m_vehicleSize < 1.0f) ||
                          (isDual && p2 && p2->m_isDart && p2->m_vehicleSize < 1.0f);

        // 3. STREAK-BASED WAVE SPAM GUARD:
        // Only bypasses if you perform 3+ rapid inputs in a row (< 8 ticks apart).
        // Single micro-clicks and normal Wave turns around spikes/sawblades stay 100% active!
        if (m_ticksSinceLastInput < SPAM_INTERVAL_TICKS) {
            m_rapidInputStreak++;
        } else {
            m_rapidInputStreak = 0;
        }
        m_ticksSinceLastInput = 0;

        if (isWave && m_rapidInputStreak >= SPAM_STREAK_REQUIRED) {
            m_gameHolding = push;
            return true;
        }

        // Release changes trajectory for Wave, Robot, and Ship.
        bool releaseAffectsPhysics = p1->m_isDart || p1->m_isRobot || p1->m_isShip ||
                                     (isDual && p2 && (p2->m_isDart || p2->m_isRobot || p2->m_isShip));
        if (!push && !releaseAffectsPhysics) {
            m_gameHolding = false;
            return true;
        }

        m_queuedInputs.clear();
        const char* act = push ? "PRESS" : "RELEASE";

        // Extended trajectory lookahead. The collision-object scratch vector below
        // is reused to reduce allocations while scanning longer horizons.
        // Ship prediction deliberately uses a short horizon. Long simulated
        // straight-fly predictions drift from live state and must not trigger
        // buffering. Only a near-term, clearly survivable delayed edge qualifies.
        int targetHorizon = isShip ? 40 : (isMiniWave ? 60 : (isWave ? 80 : 200));
        int maxDelayFrames = isWave ? m_maxTicksWave : m_maxTicksNormal;

        // 4. Check Frame 0 (immediate input) across the full trajectory horizon
        int ticksFrame0 = getSurvivalTicks(pl, p1, p2, isDual, 0, push, targetHorizon);
        if (ticksFrame0 >= targetHorizon) {
            m_gameHolding = push;
            return true;
        }

        // 5. Frame 0 hits a spike, sawblade, or slope within the horizon!
        // Scan +1..maxDelayFrames for the earliest delay that genuinely clears the obstacle.
        int bestDelay = 0;
        int bestTicks = ticksFrame0;

        for (int delay = 1; delay <= maxDelayFrames; delay++) {
            int ticks = getSurvivalTicks(pl, p1, p2, isDual, delay, push, targetHorizon);

            if (isWave) {
                // In Wave mode, delaying a turn inside a flat corridor naturally adds ~(2 * delay)
                // ticks against the opposite wall. Require at least (2 * delay + 10) extra ticks
                // so we ONLY buffer when the delayed trajectory actually clears past a spike,
                // sawblade, or corner!
                int minClearedGain = (delay * 2) + 10;
                if (ticks >= targetHorizon && (ticks - ticksFrame0) >= (delay * 2) + 6) {
                    bestDelay = delay;
                    bestTicks = ticks;
                    break;
                }
                if (ticks >= ticksFrame0 + minClearedGain && ticks > bestTicks + 2) {
                    bestDelay = delay;
                    bestTicks = ticks;
                    if (ticks >= targetHorizon) break;
                }
            } else if (isShip) {
                // Ship Precision is intentionally conservative: only buffer if
                // the current edge predicts a collision very soon, and a small
                // delay survives the entire short horizon. Never optimize a
                // long straight flight based on speculative distant collisions.
                if (ticksFrame0 <= 20 && ticks >= targetHorizon) {
                    bestDelay = delay;
                    bestTicks = ticks;
                    break;
                }
            } else {
                if (ticks >= targetHorizon) {
                    bestDelay = delay;
                    bestTicks = ticks;
                    break;
                }
                if (ticks > bestTicks + 12) {
                    bestDelay = delay;
                    bestTicks = ticks;
                }
            }
        }

        if (bestDelay > 0) {
            printf("[InputBuffer] *** SAVED +%d TICKS (%.2f frames @ 60fps) %s! (Frame0: %d -> Delayed: %d) [Limit: %d] ***\n",
                bestDelay, bestDelay / 4.0f, act, ticksFrame0, bestTicks, maxDelayFrames);
            m_bufferedPush = push;
            m_waitTicks = bestDelay;
            return false;
        }

        m_gameHolding = push;
        return true;
    }

    void onPlayerUpdate(PlayerObject* player) {
        PlayLayer* pl = PlayLayer::get();
        if (!pl || player != pl->m_player1) return;

        // F6 cycles stages, F7 toggles console, F8 disables all stages and ship prediction.
        if (GetAsyncKeyState(VK_F6) & 1) {
            cyclePreset();
        }
        if (GetAsyncKeyState(VK_F8) & 1) {
            m_presetIndex = 5;
            m_enabled = false;
            m_maxTicksNormal = 0;
            m_maxTicksWave = 0;
            m_shipPrecisionEnabled = false;
            m_waitTicks = -1;
            m_queuedInputs.clear();
            m_rapidInputStreak = 0;
            m_ticksSinceLastInput = 999;
            // Release/press any state left pending before switching to raw input.
            if (m_gameHolding != m_physicalHolding) {
                m_SimFireButton(pl, m_physicalHolding);
            }
            printf("[InputBuffer] F8 -> ALL STAGES + SHIP PREDICTIVE OFF (raw input)\n");
            fflush(stdout);
        }
        if (GetAsyncKeyState(VK_F7) & 1) {
            HWND console = GetConsoleWindow();
            if (console) {
                static bool consoleVisible = true;
                consoleVisible = !consoleVisible;
                ShowWindow(console, consoleVisible ? SW_SHOW : SW_HIDE);
            }
        }

        if (!m_enabled) return;

        // Do not let the normal state-sync path manufacture a press while the
        // physical button is still held from the previous attempt.
        if (m_waitForReleaseAfterDeath) return;

        if (m_ticksSinceLastInput < 999) {
            m_ticksSinceLastInput++;
        }

        bool isShip = player->m_isShip || (pl->m_gameState.m_isDualMode && pl->m_player2 && pl->m_player2->m_isShip);
        if (isShip && !m_shipPrecisionEnabled) {
            m_waitTicks = -1;
            m_queuedInputs.clear();
            if (m_gameHolding != m_physicalHolding) {
                m_SimFireButton(pl, m_physicalHolding);
            }
            return;
        }

        if (m_waitTicks == 0) {
            m_SimFireButton(pl, m_bufferedPush);
            m_waitTicks = -1;
        } else if (m_waitTicks > 0) {
            m_waitTicks--;
        } else if (m_waitTicks == -1 && !m_queuedInputs.empty()) {
            // Drain exactly one edge per update and only dispatch an actual state
            // transition. This preserves press/release order without generating
            // duplicate clicks when the queue and live state have converged.
            bool nextState = m_queuedInputs.front();
            m_queuedInputs.pop_front();
            if (m_gameHolding != nextState) {
                m_SimFireButton(pl, nextState);
            }
        } else if (m_waitTicks == -1) {
            // Failsafe: keep the game state synchronized with the real physical state.
            if (m_gameHolding != m_physicalHolding) {
                m_SimFireButton(pl, m_physicalHolding);
            }
        }
    }

    void m_SimFireButton(PlayLayer* pl, bool push) {
        m_firingBuffered = true;
        m_gameHolding = push;
        pl->handleButton(push, 1, true);
        if (pl->m_gameState.m_isDualMode && (!pl->m_level || !pl->m_level->m_twoPlayerMode)) {
            pl->handleButton(push, 1, false);
        }
        m_firingBuffered = false;
    }
};

static TrajectoryBufferSim s_sim;

$on_mod(Loaded) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    SetConsoleTitleA("Geode Predictive Input Buffer - Live Console");
    printf("============================================================\n");
    printf(" [InputBuffer] Prototype v0.9 loaded\n");
    printf(" [InputBuffer] Starting Mode: STAGE 1 (1 All / 1 Wave)\n");
    printf(" [InputBuffer] F6: cycle 1/1 -> 2/2 -> 4/4 -> 8/8 -> 16/8 -> OFF\n");
    printf(" [InputBuffer] F7: hide/show this console\n");
    printf(" [InputBuffer] F8: force ALL stages + ship prediction OFF (raw input)\n");
    printf(" [InputBuffer] Ship inputs remain raw; ship helper disabled\n");
    printf(" [InputBuffer] Edge-flush safety: pending predictions flush in order on a new physical edge\n");
    printf(" [InputBuffer] Extended lookahead: Normal/Ship 200 / Wave 80 / Mini Wave 60 ticks\n");
    printf("============================================================\n");
    fflush(stdout);
}

class $modify(BufPlayLayer, PlayLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("PlayLayer::destroyPlayer", -1000000);
        (void)self.setHookPriority("PlayLayer::playEndAnimationToPos", -1000000);
        (void)self.setHookPriority("PlayLayer::flipGravity", -1000000);
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        s_sim.init();
        return true;
    }

    void destroyPlayer(PlayerObject* player, GameObject* gameObject) {
        if (s_sim.checkSimulationDeath(player)) return;
        PlayLayer::destroyPlayer(player, gameObject);
    }

    void playEndAnimationToPos(cocos2d::CCPoint p0) {
        if (s_sim.isSimulating()) return;
        PlayLayer::playEndAnimationToPos(p0);
    }

    void flipGravity(PlayerObject* p0, bool p1, bool p2) {
        if (s_sim.isSimulating()) return;
        PlayLayer::flipGravity(p0, p1, p2);
    }

    void onQuit() {
        PlayLayer::onQuit();
        s_sim.quit();
    }
};

class $modify(BufEditorLayer, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool unk) {
        bool res = LevelEditorLayer::init(level, unk);
        s_sim.quit();
        return res;
    }
};

class $modify(BufHardStreak, HardStreak) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("HardStreak::addPoint", -1000000);
    }

    void addPoint(cocos2d::CCPoint p0) {
        if (s_sim.isSimulating()) return;
        HardStreak::addPoint(p0);
    }
};

class $modify(BufEffectGameObject, EffectGameObject) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("EffectGameObject::triggerObject", -1000000);
    }

    void triggerObject(GJBaseGameLayer* p0, int p1, const gd::vector<int>* p2) {
        if (s_sim.isSimulating()) return;
        EffectGameObject::triggerObject(p0, p1, p2);
    }
};

class $modify(BufGameObject, GameObject) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("GameObject::playShineEffect", -1000000);
    }

    void playShineEffect() {
        if (s_sim.isSimulating()) return;
        GameObject::playShineEffect();
    }
};

class $modify(BufPlayerObject, PlayerObject) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("PlayerObject::playSpiderDashEffect", -1000000);
        (void)self.setHookPriority("PlayerObject::incrementJumps", -1000000);
        (void)self.setHookPriority("PlayerObject::ringJump", -1000000);
    }

    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
        if (s_sim.isSimulating()) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }

    void incrementJumps() {
        if (s_sim.isSimulating()) return;
        PlayerObject::incrementJumps();
    }

    void update(float dt) {
        if (!s_sim.isSimulating()) {
            s_sim.onPlayerUpdate(this);
        }
        PlayerObject::update(dt);
        if (PlayLayer::get() && !s_sim.isSimulating()) {
            s_sim.setFrameDelta(dt);
        }
    }

    void ringJump(RingObject* p0, bool p1) {
        if (s_sim.isSimulating()) return;
        PlayerObject::ringJump(p0, p1);
    }
};

class $modify(BufBaseGameLayer, GJBaseGameLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriority("GJBaseGameLayer::canBeActivatedByPlayer", -1000000);
        (void)self.setHookPriority("GJBaseGameLayer::handleButton", -1000000);
        (void)self.setHookPriority("GJBaseGameLayer::collisionCheckObjects", -1000000);
        (void)self.setHookPriority("GJBaseGameLayer::playerTouchedRing", -1000000);
    }

    bool canBeActivatedByPlayer(PlayerObject* p0, EffectGameObject* p1) {
        if (s_sim.isSimulating()) return false;
        return GJBaseGameLayer::canBeActivatedByPlayer(p0, p1);
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        if (!s_sim.isSimulating() && !s_sim.isFiringBuffered() && button == 1 && PlayLayer::get()) {
            if (!s_sim.interceptInput(this, down, isPlayer1)) {
                return;
            }
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* vec, int objectsCount, float dt) {
        if (s_sim.isSimulating()) {
            // Reuse the same temporary vector on every simulated tick to avoid
            // repeated heap allocations during trajectory prediction.
            static gd::vector<GameObject*> extra;
            extra.clear();
            if (extra.capacity() < static_cast<size_t>(objectsCount)) {
                extra.reserve(objectsCount);
            }
            for (int i = 0; i < objectsCount; i++) {
                GameObject* obj = vec->at(i);
                if (!obj) continue;
                if (obj->m_objectType == GameObjectType::Solid ||
                    obj->m_objectType == GameObjectType::Hazard ||
                    obj->m_objectType == GameObjectType::AnimatedHazard ||
                    obj->m_objectType == GameObjectType::CollisionObject ||
                    obj->m_objectType == GameObjectType::Slope) {
                    extra.push_back(obj);
                }
            }
            GJBaseGameLayer::collisionCheckObjects(player, &extra, extra.size(), dt);
            return;
        }
        GJBaseGameLayer::collisionCheckObjects(player, vec, objectsCount, dt);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (s_sim.isSimulating()) return;
        GJBaseGameLayer::playerTouchedRing(player, ring);
    }
};
