#include "tfp/tfp.h"
#include "ord/OpenRoad.hh"
#include <tcl.h>

extern "C" {
extern int Tfp_Init(Tcl_Interp* interp);
}

namespace ord {

// Command to read and process technology file
static int read_tech_file_cmd(ClientData instanceData, Tcl_Interp* interp,
                             int objc, Tcl_Obj* const objv[]) {
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "tech_file");
        return TCL_ERROR;
    }
    
    const char* filename = Tcl_GetString(objv[1]);
    
    try {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        odb::dbDatabase* db = openroad->getDb();
        utl::Logger* logger = openroad->getLogger();
        
        tfp::TechFileParser parser(logger);
        parser.readTechFile(db, filename);
        
        return TCL_OK;
        
    } catch (const std::exception& e) {
        Tcl_SetResult(interp, const_cast<char*>(e.what()), TCL_VOLATILE);
        return TCL_ERROR;
    }
}

// Command to parse technology file without integration
static int parse_tech_file_cmd(ClientData instanceData, Tcl_Interp* interp,
                              int objc, Tcl_Obj* const objv[]) {
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "tech_file");
        return TCL_ERROR;
    }
    
    const char* filename = Tcl_GetString(objv[1]);
    
    try {
        ord::OpenRoad* openroad = ord::OpenRoad::openRoad();
        utl::Logger* logger = openroad->getLogger();
        
        tfp::TechFileParser parser(logger);
        tfp::TechFileData data = parser.parseTechFile(filename);
        
        // Return basic file info
        Tcl_Obj* result = Tcl_NewDictObj();
        
        Tcl_Obj* techName = Tcl_NewStringObj(data.technology.name.c_str(), -1);
        Tcl_DictObjPut(interp, result, Tcl_NewStringObj("name", -1), techName);
        
        Tcl_Obj* numLayers = Tcl_NewIntObj(static_cast<int>(data.layers.size()));
        Tcl_DictObjPut(interp, result, Tcl_NewStringObj("layers", -1), numLayers);
        
        Tcl_Obj* numColors = Tcl_NewIntObj(static_cast<int>(data.colors.size()));
        Tcl_DictObjPut(interp, result, Tcl_NewStringObj("colors", -1), numColors);
        
        Tcl_SetObjResult(interp, result);
        return TCL_OK;
        
    } catch (const std::exception& e) {
        Tcl_SetResult(interp, const_cast<char*>(e.what()), TCL_VOLATILE);
        return TCL_ERROR;
    }
}

} // namespace ord

// Initialize TFP Tcl commands
extern "C" int Tfp_Init(Tcl_Interp* interp) {
    Tcl_CreateObjCommand(interp, "read_tech_file", ord::read_tech_file_cmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "parse_tech_file", ord::parse_tech_file_cmd, nullptr, nullptr);
    
    return TCL_OK;
}
