#pragma once

namespace sim
{

// 실제 시간(초) -> 이번 프레임에 돌릴 tick 수. 배속은 이 횟수로만 구현한다 (4.2).
// 1x = 실시간 1초에 1 tick (1장). 결과에 영향이 없으므로 SimState에 넣지 않는다.
// sim 라이브러리에 두는 이유: SDL 없이 테스트하려고. 실제 시각은 호출하는 app이 잰다.
class TickPacer
{
public:
    void SetSpeed(int ticksPerSecond); // 0 = 일시정지
    int Speed() const;
    int Advance(double realSeconds);   // 이번 프레임에 돌릴 tick 수

private:
    int speed = 1;
    double pendingTicks = 0.0;
};

}
