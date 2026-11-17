#pragma once

#include "mapdata/map.hpp"

namespace mapdata
{

// QGIS 없이 쓰는 수작업 테스트 지도 (M2). 24 x 16 km.
//
//   y=15 +------------------------+
//        |           ~            |   ~ : 강 (x = 12, y = 8에 다리)
//        |           ~       MMMM |   M : 산
//        |  FFFF     ~            |   F : 숲
//   y=8  |  B-----J==~==J-----B   |   B : 기지, J : 분기점(보급소), - : 철도, = : 철도 + 도로
//        |  FFFF     ~            |
//   y=0  +------------------------+
//        West (Blue)  | East (Red)
Baked MakeTestMap();

}
