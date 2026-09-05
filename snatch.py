#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only or GPL-3.0-or-later
#
# Brief description of the software written in C or C++ or
#  or C# of Rust or Node.JS or a million other languages.
#
# Copyright (C) 2023 Andrew D. Harris <andrew2325@gmail.com>.
# Gemini was used to create this frontend to xwd.  I am not a software engineer!
# This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

# code...
import os
import subprocess
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

class XwdGui(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Snatch")
        self.geometry("380x310")
        self.resizable(False, False)
        self.configure(bg="#1e1e1e")

        style = ttk.Style()
        style.theme_use("clam")

        title_label = tk.Label(self, text="Snatch", fg="#ffffff", bg="#1e1e1e", font=("Helvetica", 13, "bold"))
        title_label.pack(pady=10)

        # Mode Frame (Window vs Screen + Delay)
        mode_frame = tk.LabelFrame(self, text=" Capture Mode ", fg="#cccccc", bg="#1e1e1e", font=("Helvetica", 9))
        mode_frame.pack(fill="x", padx=15, pady=5)

        self.mode_var = tk.StringVar(value="root")
        tk.Radiobutton(mode_frame, text="Whole Screen", variable=self.mode_var, value="root", command=self.toggle_delay, fg="#ffffff", bg="#1e1e1e", selectcolor="#333333", activebackground="#1e1e1e", activeforeground="#ffffff").pack(anchor="w", padx=10, pady=2)
        tk.Radiobutton(mode_frame, text="Select Window (Click)", variable=self.mode_var, value="window", command=self.toggle_delay, fg="#ffffff", bg="#1e1e1e", selectcolor="#333333", activebackground="#1e1e1e", activeforeground="#ffffff").pack(anchor="w", padx=10, pady=2)

        # Delay Row (container stays stable, inner widgets toggle to prevent layout glitches)
        self.delay_frame = tk.Frame(mode_frame, bg="#1e1e1e", height=30)
        self.delay_frame.pack(fill="x", padx=10, pady=5)
        self.delay_frame.pack_propagate(False)

        self.delay_label = tk.Label(self.delay_frame, text="Delay (seconds):", fg="#cccccc", bg="#1e1e1e", font=("Helvetica", 9))
        self.delay_label.pack(side="left")
        
        self.delay_spin = tk.Spinbox(self.delay_frame, from_=0, to=60, width=5)
        self.delay_spin.delete(0, "end")
        self.delay_spin.insert(0, "0")
        self.delay_spin.pack(side="left", padx=8)

        # Format Frame
        fmt_frame = tk.LabelFrame(self, text=" Save Format ", fg="#cccccc", bg="#1e1e1e", font=("Helvetica", 9))
        fmt_frame.pack(fill="x", padx=15, pady=5)

        self.format_var = tk.StringVar(value="png")
        formats = [("PNG", "png"), ("JPEG", "jpg"), ("BMP", "bmp"), ("Raw XWD", "xwd")]
        
        f_inner = tk.Frame(fmt_frame, bg="#1e1e1e")
        f_inner.pack(fill="x", padx=10, pady=5)
        for text, val in formats:
            tk.Radiobutton(f_inner, text=text, variable=self.format_var, value=val, fg="#ffffff", bg="#1e1e1e", selectcolor="#333333", activebackground="#1e1e1e", activeforeground="#ffffff").pack(side="left", padx=5)

        # Capture Button
        btn_frame = tk.Frame(self, bg="#1e1e1e")
        btn_frame.pack(pady=10)

        capture_btn = tk.Button(btn_frame, text="Capture Screenshot", command=self.take_screenshot, bg="#007acc", fg="white", padx=12, pady=6, relief="flat", font=("Helvetica", 9, "bold"))
        capture_btn.pack()

    def toggle_delay(self):
        if self.mode_var.get() == "root":
            self.delay_label.pack(side="left")
            self.delay_spin.pack(side="left", padx=8)
        else:
            self.delay_label.pack_forget()
            self.delay_spin.pack_forget()
        self.update_idletasks()

    def take_screenshot(self):
        if subprocess.run(["which", "xwd"], capture_output=True).returncode != 0:
            messagebox.showerror("Error", "'xwd' command not found.\nInstall it via: sudo apt install x11-apps")
            return

        fmt = self.format_var.get()
        if fmt != "xwd" and subprocess.run(["which", "convert"], capture_output=True).returncode != 0:
            messagebox.showerror("Error", "ImageMagick 'convert' command not found (needed for format conversion).\nInstall it via: sudo apt install imagemagick")
            return

        try:
            delay_sec = int(self.delay_spin.get() or 0) if self.mode_var.get() == "root" else 0
        except ValueError:
            delay_sec = 0

        ext = f".{fmt}"
        out_file = filedialog.asksaveasfilename(
            title="Save Screenshot As",
            initialfile=f"screenshot{ext}",
            defaultextension=ext,
            filetypes=[(f"{fmt.upper()} Image", f"*{ext}"), ("All Files", "*.*")]
        )
        if not out_file:
            return

        # Hide main GUI window temporarily so it isn't captured
        self.withdraw()
        
        # Apply delay if specified (multiplied by 1000 for ms), otherwise slight 500ms buffer
        total_delay = max(delay_sec * 1000, 500)
        self.after(total_delay, lambda: self._execute_capture(out_file, fmt))

    def _execute_capture(self, out_file, fmt):
        tmp_xwd = "/tmp/Snatch_shot.xwd"
        mode = self.mode_var.get()

        cmd = ["xwd"]
        if mode == "root":
            cmd.append("-root")
        else:
            cmd.append("-frame")
        
        cmd.extend(["-out", tmp_xwd])

        res = subprocess.run(cmd, capture_output=True, text=True)
        
        if res.returncode != 0:
            self.deiconify()
            messagebox.showerror("Capture Error", f"xwd failed:\n{res.stderr.strip() or res.stdout.strip()}")
            return

        if fmt == "xwd":
            os.replace(tmp_xwd, out_file)
            self.deiconify()
            messagebox.showinfo("Success", f"Raw XWD saved to:\n{out_file}")
            return

        # Convert using ImageMagick
        conv_cmd = ["convert", tmp_xwd, out_file]
        conv_res = subprocess.run(conv_cmd, capture_output=True, text=True)

        if os.path.exists(tmp_xwd):
            os.remove(tmp_xwd)

        self.deiconify()

        if conv_res.returncode == 0:
            messagebox.showinfo("Success", f"Screenshot saved to:\n{out_file}")
        else:
            err = conv_res.stderr.strip() or conv_res.stdout.strip()
            messagebox.showerror("Conversion Error", f"Failed to convert screenshot:\n{err}")

if __name__ == "__main__":
    app = XwdGui()
    app.mainloop()
