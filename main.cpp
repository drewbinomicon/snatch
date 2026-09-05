/*
* Copyright (C) 2026 [Your Name or Organization]
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://gnu.org>.
*/

#include <tcl.h>
#include <tk.h>
#include <cstdlib>
#include <string>
#include <thread>
#include <chrono>
#include <sys/stat.h>

int TakeScreenshotCmd(ClientData clientData, Tcl_Interp *interp, int argc, const char *argv[]) {
    if (system("which xwd > /dev/null 2>&1") != 0) {
        Tcl_Eval(interp, "tk_messageBox -icon error -title \"Error\" -message \"'xwd' command not found.\\nInstall it via: sudo apt install x11-apps\"");
        return TCL_OK;
    }

    const char *fmt_cstr = Tcl_GetVar(interp, "save_format", TCL_GLOBAL_ONLY);
    const char *mode_cstr = Tcl_GetVar(interp, "capture_mode", TCL_GLOBAL_ONLY);
    std::string fmt = fmt_cstr ? fmt_cstr : "png";
    std::string mode = mode_cstr ? mode_cstr : "root";

    if (fmt != "xwd" && system("which convert > /dev/null 2>&1") != 0) {
        Tcl_Eval(interp, "tk_messageBox -icon error -title \"Error\" -message \"ImageMagick 'convert' command not found.\\nInstall it via: sudo apt install imagemagick\"");
        return TCL_OK;
    }

    int delay_sec = 0;
    if (mode == "root") {
        Tcl_Eval(interp, "set _delay [.mode_frame.delay_frame.spn get]");
        const char *delay_str = Tcl_GetString(Tcl_GetObjResult(interp));
        try {
            delay_sec = std::stoi(delay_str);
        } catch (...) {
            delay_sec = 0;
        }
    }

    std::string ext = "." + fmt;
    std::string dialog_cmd = "tk_getSaveFile -title \"Save Screenshot As\" -defaultextension " + ext + " -filetypes {{\"" + fmt + " Image\" {*" + ext + "}} {\"All Files\" {*.*}}}";
    
    if (Tcl_Eval(interp, dialog_cmd.c_str()) == TCL_ERROR) {
        return TCL_OK;
    }
    
    std::string out_file = Tcl_GetString(Tcl_GetObjResult(interp));
    if (out_file.empty()) {
        return TCL_OK;
    }

    Tcl_Eval(interp, "wm withdraw .");
    Tcl_Eval(interp, "update");

    int total_ms = (delay_sec > 0) ? (delay_sec * 1000) : 500;
    std::this_thread::sleep_for(std::chrono::milliseconds(total_ms));

    std::string tmp_xwd = "/tmp/Snatch_shot.xwd";
    std::string xwd_cmd = "xwd ";
    if (mode == "root") {
        xwd_cmd += "-root ";
    } else {
        xwd_cmd += "-frame ";
    }
    xwd_cmd += "-out " + tmp_xwd + " > /dev/null 2>&1";

    int ret = system(xwd_cmd.c_str());
    if (ret != 0) {
        Tcl_Eval(interp, "wm deiconify .");
        Tcl_Eval(interp, "tk_messageBox -icon error -title \"Capture Error\" -message \"xwd capture failed.\"");
        return TCL_OK;
    }

    if (fmt == "xwd") {
        std::string move_cmd = "mv " + tmp_xwd + " \"" + out_file + "\"";
        system(move_cmd.c_str());
        Tcl_Eval(interp, "wm deiconify .");
        Tcl_Eval(interp, "tk_messageBox -icon info -title \"Success\" -message \"Raw XWD saved successfully!\"");
        return TCL_OK;
    }

    std::string conv_cmd = "convert " + tmp_xwd + " \"" + out_file + "\"";
    int conv_ret = system(conv_cmd.c_str());
    struct stat buffer;
    if (stat(tmp_xwd.c_str(), &buffer) == 0) {
        remove(tmp_xwd.c_str());
    }

    Tcl_Eval(interp, "wm deiconify .");

    if (conv_ret == 0) {
        Tcl_Eval(interp, "tk_messageBox -icon info -title \"Success\" -message \"Screenshot saved successfully!\"");
    } else {
        Tcl_Eval(interp, "tk_messageBox -icon error -title \"Conversion Error\" -message \"Failed to convert screenshot image format.\"");
    }

    return TCL_OK;
}

int main(int argc, char **argv) {
    Tcl_FindExecutable(argv[0]);
    Tcl_Interp *interp = Tcl_CreateInterp();

    if (Tcl_Init(interp) == TCL_ERROR) return 1;
    if (Tk_Init(interp) == TCL_ERROR) return 1;

    Tcl_CreateCommand(interp, "TakeScreenshotCpp", TakeScreenshotCmd, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    const char *ui_script = 
        "wm title . \"Snatch\"\n"
        "wm geometry . \"380x310\"\n"
        "wm resizable . 0 0\n"
        "option add *background #d9d9d9\n"
        "option add *foreground #000000\n"
        "option add *Label*background #d9d9d9\n"
        "option add *Label*foreground #000000\n"
        "option add *Labelframe*background #d9d9d9\n"
        "option add *Labelframe*foreground #333333\n"
        "option add *Radiobutton*background #d9d9d9\n"
        "option add *Radiobutton*foreground #000000\n"
        "option add *Radiobutton*selectColor #ffffff\n"
        "option add *Frame*background #d9d9d9\n"
        ". configure -background #d9d9d9\n"
        "label .title -text \"Snatch\" -font {Helvetica 13 bold}\n"
        "pack .title -pady 10\n"
        "labelframe .mode_frame -text \" Capture Mode \" -font {Helvetica 9}\n"
        "pack .mode_frame -fill x -padx 15 -pady 5\n"
        "global capture_mode\n"
        "set capture_mode \"root\"\n"
        "radiobutton .mode_frame.root -text \"Whole Screen\" -variable capture_mode -value \"root\" -command toggle_delay\n"
        "radiobutton .mode_frame.window -text \"Select Window (Click)\" -variable capture_mode -value \"window\" -command toggle_delay\n"
        "pack .mode_frame.root -anchor w -padx 10 -pady 2\n"
        "pack .mode_frame.window -anchor w -padx 10 -pady 2\n"
        "frame .mode_frame.delay_frame -height 30\n"
        "pack .mode_frame.delay_frame -fill x -padx 10 -pady 5\n"
        "pack propagate .mode_frame.delay_frame 0\n"
        "label .mode_frame.delay_frame.lbl -text \"Delay (seconds):\" -font {Helvetica 9}\n"
        "spinbox .mode_frame.delay_frame.spn -from 0 -to 60 -width 5\n"
        ".mode_frame.delay_frame.spn delete 0 end\n"
        ".mode_frame.delay_frame.spn insert 0 0\n"
        "pack .mode_frame.delay_frame.lbl -side left\n"
        "pack .mode_frame.delay_frame.spn -side left -padx 8\n"
        "labelframe .fmt_frame -text \" Save Format \" -font {Helvetica 9}\n"
        "pack .fmt_frame -fill x -padx 15 -pady 5\n"
        "global save_format\n"
        "set save_format \"png\"\n"
        "frame .fmt_frame.inner\n"
        "pack .fmt_frame.inner -fill x -padx 10 -pady 5\n"
        "foreach {name val} {PNG png JPEG jpg BMP bmp \"Raw XWD\" xwd} {\n"
        "    radiobutton .fmt_frame.inner.$val -text $name -variable save_format -value $val\n"
        "    pack .fmt_frame.inner.$val -side left -padx 5\n"
        "}\n"
        "frame .btn_frame\n"
        "pack .btn_frame -pady 10\n"
        "button .btn_frame.capture -text \"Capture Screenshot\" -command TakeScreenshotCpp -bg #007acc -fg white -relief flat -font {Helvetica 9 bold} -padx 12 -pady 6\n"
        "pack .btn_frame.capture\n"
        "proc toggle_delay {} {\n"
        "    global capture_mode\n"
        "    if {$capture_mode eq \"root\"} {\n"
        "        pack .mode_frame.delay_frame.lbl -side left\n"
        "        pack .mode_frame.delay_frame.spn -side left -padx 8\n"
        "    } else {\n"
        "        pack forget .mode_frame.delay_frame.lbl\n"
        "        pack forget .mode_frame.delay_frame.spn\n"
        "    }\n"
        "}\n";

    if (Tcl_Eval(interp, ui_script) == TCL_ERROR) {
        const char *err = Tcl_GetStringResult(interp);
        fprintf(stderr, "Tcl Error: %s\n", err);
        return 1;
    }

    Tk_MainLoop();
    return 0;
}
