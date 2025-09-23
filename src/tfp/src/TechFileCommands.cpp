#include "../include/tfp/tfp.h"
#include <tcl.h>

extern "C" {
extern int Tfp_Init(Tcl_Interp* interp);
}

static int read_tech_file_cmd(ClientData instanceData, Tcl_Interp* interp,
                             int objc, Tcl_Obj* const objv[]) {
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "tech_file");
        return TCL_ERROR;
    }
    
    const char* filename = Tcl_GetString(objv[1]);
    
    try {
        utl::Logger logger;
        tfp::TechFileParser parser(&logger);
        tfp::TechFileData data = parser.parseTechFile(filename);
        
        // Use provided name or default
        std::string techName = data.technology.name.empty() ? "tech" : data.technology.name;
        
        std::string script = 
            "set db [ord::get_db]\n"
            "set tech [$db getTech]\n"
            "if { $tech == \"NULL\" } {\n"
            "  set tech [odb::dbTech_create $db \"" + techName + "\"]\n"
            "}\n"
            "$tech setDbUnitsPerMicron " + std::to_string(data.technology.lengthPrecision) + "\n"
            "$tech setLefUnits " + std::to_string(data.technology.lengthPrecision) + "\n";
        
        // Create layers from .tf data
        for (const auto& layerInfo : data.layers) {
            std::string layerType;
            
            // Determine layer type
            if (layerInfo.name.find("VIA") != std::string::npos || 
                layerInfo.name.find("CONT") != std::string::npos ||
                layerInfo.name == "CPO" || layerInfo.name == "CTM1" || 
                layerInfo.name == "VIA0" || layerInfo.name == "VIARDL") {
                layerType = "CUT";
            } else if (layerInfo.name == "DIFF" || layerInfo.name == "PO" || 
                      layerInfo.name == "NWELL" || layerInfo.name == "DNW" ||
                      layerInfo.name == "PIMP" || layerInfo.name == "NIMP" ||
                      layerInfo.name.find("IMP") != std::string::npos ||
                      layerInfo.name == "FIN" || layerInfo.name == "SBLK") {
                layerType = "IMPLANT";
            } else if (layerInfo.name.find("M") == 0 && std::isdigit(layerInfo.name[1])) {
                layerType = "ROUTING";
            } else if (layerInfo.name == "MRDL") {
                layerType = "ROUTING";
            } else {
                layerType = "OVERLAP";
            }
            
            // Check if layer exists
            script += "set existing_layer [$tech findLayer \"" + layerInfo.name + "\"]\n";
            script += "if { $existing_layer == \"NULL\" } {\n";
            script += "  set layer_" + layerInfo.name + " [odb::dbTechLayer_create $tech \"" + 
                     layerInfo.name + "\" \"" + layerType + "\"]\n";
            script += "  puts \"Created layer: " + layerInfo.name + " (" + layerType + ")\"\n";
            script += "} else {\n";
            script += "  set layer_" + layerInfo.name + " $existing_layer\n";
            script += "  puts \"Layer already exists: " + layerInfo.name + "\"\n";
            script += "}\n";
            
            // Set layer properties
            if (layerInfo.minWidth > 0) {
                int minWidthDBU = static_cast<int>(layerInfo.minWidth * data.technology.lengthPrecision);
                script += "if { $layer_" + layerInfo.name + " != \"NULL\" } {\n";
                script += "  $layer_" + layerInfo.name + " setMinWidth " + std::to_string(minWidthDBU) + "\n";
                script += "}\n";
            }
            
            if (layerInfo.minSpacing > 0) {
                int minSpacingDBU = static_cast<int>(layerInfo.minSpacing * data.technology.lengthPrecision);
                script += "if { $layer_" + layerInfo.name + " != \"NULL\" } {\n";
                script += "  $layer_" + layerInfo.name + " setSpacing " + std::to_string(minSpacingDBU) + "\n";
                script += "}\n";
            }
            
            if (layerInfo.pitch > 0) {
                int pitchDBU = static_cast<int>(layerInfo.pitch * data.technology.lengthPrecision);
                script += "if { $layer_" + layerInfo.name + " != \"NULL\" } {\n";
                script += "  $layer_" + layerInfo.name + " setPitch " + std::to_string(pitchDBU) + "\n";
                script += "}\n";
            }
            
            if (layerInfo.defaultWidth > 0) {
                int defaultWidthDBU = static_cast<int>(layerInfo.defaultWidth * data.technology.lengthPrecision);
                script += "if { $layer_" + layerInfo.name + " != \"NULL\" } {\n";
                script += "  $layer_" + layerInfo.name + " setWidth " + std::to_string(defaultWidthDBU) + "\n";
                script += "}\n";
            }
            
            // Set routing direction
            if (layerType == "ROUTING") {
                std::string direction;
                if (layerInfo.name == "M1" || layerInfo.name == "M3" || 
                    layerInfo.name == "M5" || layerInfo.name == "M7" || 
                    layerInfo.name == "M9" || layerInfo.name == "MRDL" ||
                    layerInfo.name == "PO") {
                    direction = "HORIZONTAL";
                } else if (layerInfo.name == "M2" || layerInfo.name == "M4" || 
                          layerInfo.name == "M6" || layerInfo.name == "M8") {
                    direction = "VERTICAL";
                }
                
                if (!direction.empty()) {
                    script += "if { $layer_" + layerInfo.name + " != \"NULL\" } {\n";
                    script += "  $layer_" + layerInfo.name + " setDirection \"" + direction + "\"\n";
                    script += "}\n";
                }
            }
        }
        
        // Final info
        script += "set final_layer_count [llength [$tech getLayers]]\n";
        script += "puts \"Technology setup complete. Total layers: $final_layer_count\"\n";
        
        // Execute TCL script
        int result = Tcl_Eval(interp, const_cast<char*>(script.c_str()));
        
        if (result != TCL_OK) {
            std::string error = "Tcl Error: ";
            error += Tcl_GetStringResult(interp);
            Tcl_SetResult(interp, const_cast<char*>(error.c_str()), TCL_VOLATILE);
            return TCL_ERROR;
        }
        
        char resultMsg[512];
        snprintf(resultMsg, sizeof(resultMsg), 
                "Technology '%s' loaded\n"
                "- DBU: %d units/micron\n"
                "- Layers: %zu\n"
                "- Vias: %zu\n"
                "Next: read_lef <file.lef> and read_def <file.def>",
                techName.c_str(),
                data.technology.lengthPrecision,
                data.layers.size(),
                data.vias.size());
        
        Tcl_SetResult(interp, resultMsg, TCL_VOLATILE);
        return TCL_OK;
        
    } catch (const std::exception& e) {
        std::string error = "Error: ";
        error += e.what();
        Tcl_SetResult(interp, const_cast<char*>(error.c_str()), TCL_VOLATILE);
        return TCL_ERROR;
    }
}

static int show_tech_info_cmd(ClientData instanceData, Tcl_Interp* interp,
                             int objc, Tcl_Obj* const objv[]) {
    const char* script = 
        "set db [ord::get_db]\n"
        "set tech [$db getTech]\n"
        "if { $tech != \"NULL\" } {\n"
        "  set layers [$tech getLayers]\n"
        "  set dbu [$tech getDbUnitsPerMicron]\n"
        "  set layer_count [llength $layers]\n"
        "  puts \"Technology:\"\n"
        "  puts \"  DBU: $dbu\"\n"
        "  puts \"  Layers: $layer_count\"\n"
        "  foreach layer $layers {\n"
        "    set name [$layer getName]\n"
        "    set type [$layer getType]\n"
        "    puts \"    $name ($type)\"\n"
        "  }\n"
        "  return \"$layer_count layers\"\n"
        "} else {\n"
        "  return \"No technology loaded\"\n"
        "}";
    
    return Tcl_Eval(interp, const_cast<char*>(script));
}

static int test_tech_cmd(ClientData instanceData, Tcl_Interp* interp,
                        int objc, Tcl_Obj* const objv[]) {
    const char* script = 
        "set db [ord::get_db]\n"
        "set tech [$db getTech]\n"
        "if { $tech == \"NULL\" } {\n"
        "  set tech [odb::dbTech_create $db \"test_tech\"]\n"
        "  if { $tech != \"NULL\" } {\n"
        "    $tech setDbUnitsPerMicron 1000\n"
        "    return \"Tech created\"\n"
        "  } else {\n"
        "    return \"Tech creation failed\"\n"
        "  }\n"
        "} else {\n"
        "  return \"Tech exists\"\n"
        "}";
    
    return Tcl_Eval(interp, const_cast<char*>(script));
}

extern "C" int Tfp_Init(Tcl_Interp* interp) {
    Tcl_CreateObjCommand(interp, "read_tech_file", read_tech_file_cmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "show_tech_info", show_tech_info_cmd, nullptr, nullptr);
    Tcl_CreateObjCommand(interp, "test_tech", test_tech_cmd, nullptr, nullptr);
    
    if (Tcl_PkgProvide(interp, "tfp", "1.0") != TCL_OK) {
        return TCL_ERROR;
    }
    
    return TCL_OK;
}
