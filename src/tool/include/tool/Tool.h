// Copyright (c) 2021, The Regents of the University of California
// All rights reserved.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <tcl.h>
#include <vector>

#include "ord/OpenRoad.hh"
#include "odb/db.h"
#include "tool/Cell.h"
#include "tool/FlipFlop.h"

namespace sta {
class dbNetwork;
}

namespace tool {
class FlipFlop;
class Cell;

class Tool
{
public:
  Tool();
  ~Tool() = default;
  void init(
    odb::dbDatabase *db,
    utl::Logger *logger,
    sta::dbNetwork* network,
    ord::OpenRoad* openROAD);
  void printAllFlops();
  void orderFlops();
  void clusterFlops();

  void run();

private:
  odb::dbDatabase *db_ = nullptr;
  utl::Logger *log_ = nullptr;
  sta::dbNetwork* network_ = nullptr;
  ord::OpenRoad* openROAD_ = nullptr;

  std::vector<FlipFlop> flipflops_;
  std::vector<Cell> cells_;
};

}
