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

#include "tool/Tool.h"
#include "tool/FlipFlop.h"
#include "db_sta/dbNetwork.hh"
#include "sta/StaMain.hh"
#include "sta/Liberty.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char *tool_tcl_inits[];
}

namespace tool {

extern "C" {
extern int Tool_Init(Tcl_Interp *interp);
}

Tool::Tool()
{
}

void Tool::run()
{
  log_->report("Runing tool:");
  odb::dbBlock* block = db_->getChip()->getBlock();
  // TODO: Cell delay and power info are on the input file different than LEF/DEF
  for(odb::dbInst* cell : block->getInsts()) {
    // Check if cell is a flip flop
    sta::Cell* masterCell = network_->dbToSta(cell->getMaster());
    if (masterCell) {
      sta::LibertyCell* libCell = network_->libertyCell(masterCell);
      if (libCell) {
        if (!libCell->hasSequentials()) {
          // if it is not a non sequential element ignore (for now)
          // I think this is the only way of knowing
          continue;
        }
      }
    }

    std::string name = cell->getName();
  
    int cell_x, cell_y;
    cell->getLocation(cell_x, cell_y);

    odb::dbBox* cell_bbox = cell->getBBox();
    int cell_dx = cell_bbox->getDX();
    int cell_dy = cell_bbox->getDY();
    int area = cell_dx * cell_dy;

    int id = flipflops_.size();
    flipflops_.emplace_back(name, id, cell_x, cell_y, 1, 0, 0, area);
  }

  printAllFlops();
  // Pelo que eu entendi da definição do problema não vai ter arquivos LIB.
  // Tem um arquivo com todas as celulas e vai mostrar o slack delay, power e area. 
}

void Tool::printAllFlops()
{
  for(auto f : flipflops_) {
    int x, y;
    f.getLocation(x, y);
    log_->report("Flip-Flop {} at ({} {}) has delay = {}", f.getName(), x, y, f.getDelay());
  }
}


void Tool::init(
	   odb::dbDatabase *db,
     utl::Logger *logger,
     sta::dbNetwork* network,
     ord::OpenRoad *openROAD)
{
  db_ = db;
  log_ = logger;
  network_ = network;
  openROAD_ = openROAD;

}

}
