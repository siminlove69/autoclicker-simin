#!/usr/bin/env python3
"""
Simin AutoClicker v3 – Goated Edition (Python port)
Requires: pip install pynput
"""

import sys
import time
import threading
import tkinter as tk
from tkinter import ttk

try:
    from pynput import mouse as pmouse, keyboard as pkeyboard # type: ignore
    PYNPUT_OK = True
except ImportError:
    PYNPUT_OK = False

# ── Palette ────────────────────────────────────────────────────────────────────
C_BG      = "#08070f"
C_CARD    = "#12101e"
C_CARD2   = "#16142a"
C_BORD    = "#3c2d6e"
C_BORD_LT = "#6447b0"
C_VIO     = "#8b5cf6"
C_VIO_DK  = "#50329f"
C_VIO_LT  = "#b496ff"
C_FUCH    = "#d946ef"
C_GREEN   = "#34d399"
C_GREEN_DK= "#147850"
C_RED     = "#f05a5a"
C_RED_DK  = "#961e1e"
C_TEXT    = "#f0eeff"
C_SUB     = "#a094d2"
C_DIM     = "#645896"
C_INPUT   = "#18162e"

HOTKEY_MAP = {
    "F6": pkeyboard.Key.f6  if PYNPUT_OK else None,
    "F7": pkeyboard.Key.f7  if PYNPUT_OK else None,
    "F8": pkeyboard.Key.f8  if PYNPUT_OK else None,
    "F9": pkeyboard.Key.f9  if PYNPUT_OK else None,
}


# ── Styled widget helpers ──────────────────────────────────────────────────────
def styled_entry(parent, textvariable, width=8, **kw):
    return tk.Entry(
        parent, textvariable=textvariable, width=width,
        bg=C_INPUT, fg=C_TEXT, insertbackground=C_TEXT,
        relief="flat", font=("Segoe UI", 11),
        highlightthickness=1,
        highlightcolor=C_BORD_LT, highlightbackground=C_BORD,
        **kw,
    )


def styled_spinbox(parent, from_, to, textvariable, width=10, **kw):
    return tk.Spinbox(
        parent, from_=from_, to=to, textvariable=textvariable, width=width,
        bg=C_INPUT, fg=C_TEXT, insertbackground=C_TEXT,
        buttonbackground=C_CARD2, relief="flat", font=("Segoe UI", 11),
        highlightthickness=1,
        highlightcolor=C_BORD_LT, highlightbackground=C_BORD,
        **kw,
    )


def styled_combo(parent, textvariable, values, width=14):
    style = ttk.Style()
    style.theme_use("clam")
    style.configure(
        "Dark.TCombobox",
        fieldbackground=C_INPUT,
        background=C_CARD2,
        foreground=C_TEXT,
        selectbackground=C_BORD,
        selectforeground=C_TEXT,
        arrowcolor=C_VIO_LT,
        bordercolor=C_BORD,
        lightcolor=C_BORD,
        darkcolor=C_BORD,
    )
    style.map(
        "Dark.TCombobox",
        fieldbackground=[("readonly", C_INPUT)],
        foreground=[("readonly", C_TEXT)],
        background=[("readonly", C_CARD2)],
    )
    cb = ttk.Combobox(
        parent, textvariable=textvariable, values=values,
        state="readonly", width=width, style="Dark.TCombobox",
        font=("Segoe UI", 10),
    )
    return cb


# ── Main application ───────────────────────────────────────────────────────────
class SiminAutoClicker:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Autoclicker!!!1")
        self.root.configure(bg=C_BG)
        self.root.resizable(False, False)
        self.root.geometry("360x720")

        # State
        self.running        = False
        self.stop_event     = threading.Event()
        self.click_count    = 0
        self.cps            = 0.0
        self._cps_t0        = 0.0
        self._cps_n0        = 0
        self.click_thread: threading.Thread | None = None
        self.picking        = False
        self._pick_listener = None
        self._hk_listener   = None

        self._build_ui()

        if not PYNPUT_OK:
            self._warn_no_pynput()

        self._start_hotkey_listener()
        self._tick()

    # ── UI build ──────────────────────────────────────────────────────────────
    def _build_ui(self):
        # Header
        hdr = tk.Frame(self.root, bg="#1a1040", height=78)
        hdr.pack(fill="x"); hdr.pack_propagate(False)
        tk.Frame(hdr, bg=C_VIO, width=5).pack(side="left", fill="y")
        inner = tk.Frame(hdr, bg="#1a1040")
        inner.pack(side="left", fill="both", expand=True, padx=10, pady=8)
        tk.Label(inner, text="Autoclicker but on py",
                 font=("Segoe UI", 18, "bold"), fg=C_TEXT, bg="#1a1040"
                 ).pack(anchor="w")
        tk.Label(inner, text="v3.0  |  Goated Edition",
                 font=("Segoe UI", 10), fg=C_VIO_LT, bg="#1a1040"
                 ).pack(anchor="w")
        # Accent bar
        tk.Frame(self.root, bg=C_VIO, height=3).pack(fill="x")

        # Scrollable content wrapper
        outer = tk.Frame(self.root, bg=C_BG)
        outer.pack(fill="both", expand=True, padx=12, pady=6)

        # ── Card: Interval ────────────────────────────────────────
        self._section(outer, "CLICK INTERVAL (ms)", C_VIO)
        row = self._card_row(outer)
        self.interval_var = tk.IntVar(value=100)
        styled_spinbox(row, 10, 99999, self.interval_var, width=12
                       ).pack(side="left", padx=8, pady=6)
        tk.Label(row, text="milliseconds", font=("Segoe UI", 9),
                 fg=C_DIM, bg=C_CARD).pack(side="left")

        # ── Card: Mouse settings ──────────────────────────────────
        self._section(outer, "MOUSE SETTINGS", C_FUCH)
        row = self._card_row(outer)
        self._dim_label(row, "BUTTON")
        self.button_var = tk.StringVar(value="Left Button")
        styled_combo(row, self.button_var,
                     ["Left Button", "Right Button", "Middle Button"],
                     width=13).pack(side="left", padx=6, pady=5)
        self._dim_label(row, "TYPE")
        self.type_var = tk.StringVar(value="Single Click")
        styled_combo(row, self.type_var,
                     ["Single Click", "Double Click"],
                     width=13).pack(side="left", padx=6, pady=5)

        # ── Card: Location ────────────────────────────────────────
        self._section(outer, "CLICK LOCATION", C_VIO)
        row = self._card_row(outer)
        self._dim_label(row, "MODE")
        self.loc_var = tk.StringVar(value="Follow cursor position")
        loc_cb = styled_combo(row, self.loc_var,
                              ["Follow cursor position", "Fixed screen position"],
                              width=22)
        loc_cb.pack(side="left", padx=6, pady=5)
        loc_cb.bind("<<ComboboxSelected>>", self._on_loc_change)

        xy_row = self._card_row(outer)
        self._dim_label(xy_row, "X")
        self.x_var = tk.IntVar(value=0)
        self.x_entry = styled_entry(xy_row, self.x_var, width=7)
        self.x_entry.pack(side="left", padx=(2, 8), pady=5)
        self._dim_label(xy_row, "Y")
        self.y_var = tk.IntVar(value=0)
        self.y_entry = styled_entry(xy_row, self.y_var, width=7)
        self.y_entry.pack(side="left", padx=(2, 8), pady=5)
        self.pick_btn = tk.Button(
            xy_row, text="📍 Pick Pos",
            font=("Segoe UI", 9, "bold"),
            bg=C_CARD2, fg=C_VIO_LT, activebackground=C_BORD,
            activeforeground=C_TEXT, relief="flat", cursor="hand2",
            bd=0, padx=8, pady=3,
            command=self._pick_pos,
        )
        self.pick_btn.pack(side="left", padx=2, pady=5)
        # disabled until fixed mode selected
        self.x_entry.config(state="disabled")
        self.y_entry.config(state="disabled")
        self.pick_btn.config(state="disabled")

        # ── Card: Click limit ─────────────────────────────────────
        self._section(outer, "CLICK LIMIT", C_FUCH)
        row = self._card_row(outer)
        self.limit_var = tk.BooleanVar(value=False)
        tk.Checkbutton(
            row, text="Enable click limit", variable=self.limit_var,
            font=("Segoe UI", 10), fg=C_TEXT, bg=C_CARD,
            activeforeground=C_TEXT, activebackground=C_CARD,
            selectcolor=C_INPUT, command=self._on_limit_change,
        ).pack(side="left", padx=8, pady=5)
        self._dim_label(row, "COUNT")
        self.limit_count_var = tk.IntVar(value=100)
        self.limit_spin = styled_spinbox(
            row, 1, 9_999_999, self.limit_count_var, width=9, state="disabled")
        self.limit_spin.pack(side="left", padx=6, pady=5)

        # ── Card: Hotkey ──────────────────────────────────────────
        self._section(outer, "TOGGLE HOTKEY", C_VIO)
        row = self._card_row(outer)
        self._dim_label(row, "KEY")
        self.hotkey_var = tk.StringVar(value="F6")
        hk_cb = styled_combo(row, self.hotkey_var, ["F6", "F7", "F8", "F9"], width=6)
        hk_cb.pack(side="left", padx=6, pady=5)
        hk_cb.bind("<<ComboboxSelected>>", lambda _e: self._start_hotkey_listener())

        # ── Status panel ──────────────────────────────────────────
        stat = tk.Frame(outer, bg=C_CARD2, pady=10,
                        highlightthickness=1, highlightbackground=C_BORD_LT)
        stat.pack(fill="x", pady=(10, 4))
        tk.Frame(stat, bg=C_VIO, height=2).pack(fill="x")
        self.status_lbl = tk.Label(
            stat, text="IDLE", font=("Segoe UI", 18, "bold"),
            fg=C_VIO_LT, bg=C_CARD2)
        self.status_lbl.pack(pady=(6, 0))
        self.cps_lbl = tk.Label(
            stat, text="0.0 clicks / sec",
            font=("Consolas", 14, "bold"), fg=C_DIM, bg=C_CARD2)
        self.cps_lbl.pack()
        self.count_lbl = tk.Label(
            stat, text="Total: 0",
            font=("Consolas", 11), fg=C_DIM, bg=C_CARD2)
        self.count_lbl.pack(pady=(0, 6))

        # ── Start / Stop buttons ──────────────────────────────────
        btn_row = tk.Frame(outer, bg=C_BG)
        btn_row.pack(fill="x", pady=(6, 2))
        self.start_btn = tk.Button(
            btn_row, text="▶  START",
            font=("Segoe UI", 13, "bold"),
            bg="#6e46dc", fg=C_TEXT,
            activebackground=C_VIO, activeforeground=C_TEXT,
            relief="flat", cursor="hand2", pady=11,
            command=self.start_clicking,
        )
        self.start_btn.pack(side="left", expand=True, fill="x", padx=(0, 4))
        self.stop_btn = tk.Button(
            btn_row, text="■  STOP",
            font=("Segoe UI", 13, "bold"),
            bg=C_RED_DK, fg=C_TEXT,
            activebackground=C_RED, activeforeground=C_TEXT,
            relief="flat", cursor="hand2", pady=11, state="disabled",
            command=self.stop_clicking,
        )
        self.stop_btn.pack(side="left", expand=True, fill="x", padx=(4, 0))

    # ── Section / card helpers ────────────────────────────────────────────────
    def _section(self, parent, label: str, accent: str):
        f = tk.Frame(parent, bg=C_CARD,
                     highlightthickness=1, highlightbackground=C_BORD)
        f.pack(fill="x", pady=(8, 0))
        top = tk.Frame(f, bg=C_CARD)
        top.pack(fill="x")
        tk.Frame(top, bg=accent, width=4).pack(side="left", fill="y")
        tk.Label(top, text=label, font=("Segoe UI", 9, "bold"),
                 fg=accent, bg=C_CARD, padx=8, pady=3).pack(side="left")
        return f

    def _card_row(self, parent) -> tk.Frame:
        f = tk.Frame(parent, bg=C_CARD)
        f.pack(fill="x")
        return f

    def _dim_label(self, parent, text: str):
        tk.Label(parent, text=text, font=("Segoe UI", 8),
                 fg=C_DIM, bg=C_CARD).pack(side="left", padx=(10, 2))

    # ── Widget callbacks ──────────────────────────────────────────────────────
    def _on_loc_change(self, _event=None):
        fixed = self.loc_var.get() == "Fixed screen position"
        s = "normal" if fixed else "disabled"
        self.x_entry.config(state=s)
        self.y_entry.config(state=s)
        self.pick_btn.config(state=s)

    def _on_limit_change(self):
        s = "normal" if self.limit_var.get() else "disabled"
        self.limit_spin.config(state=s)

    def _pick_pos(self):
        if not PYNPUT_OK:
            return
        self.picking = True
        self.pick_btn.config(text="Click anywhere…")
        self.root.iconify()

        def on_click(x, y, button, pressed):
            if pressed and self.picking:
                self.picking = False
                self.root.after(0, lambda: self._pick_done(x, y))
                return False

        self._pick_listener = pmouse.Listener(on_click=on_click)
        self._pick_listener.daemon = True
        self._pick_listener.start()

    def _pick_done(self, x: int, y: int):
        self.x_var.set(x)
        self.y_var.set(y)
        self.pick_btn.config(text="📍 Pick Pos")
        self.root.deiconify()
        self.root.lift()

    # ── Click logic ───────────────────────────────────────────────────────────
    def _do_click(self):
        if not PYNPUT_OK:
            return
        btn_str = self.button_var.get()
        btn = {
            "Left Button":   pmouse.Button.left,
            "Right Button":  pmouse.Button.right,
            "Middle Button": pmouse.Button.middle,
        }.get(btn_str, pmouse.Button.left)
        ctrl = pmouse.Controller()
        count = 2 if self.type_var.get() == "Double Click" else 1
        ctrl.click(btn, count)

    def _click_worker(self, interval_ms: int, fixed: bool,
                      fx: int, fy: int, limited: bool, max_n: int):
        interval_s = interval_ms / 1000.0
        n = 0
        while not self.stop_event.is_set():
            if fixed and PYNPUT_OK:
                pmouse.Controller().position = (fx, fy)
            self._do_click()
            n += 1
            self.click_count = n
            if limited and n >= max_n:
                break
            # Interruptible sleep
            end = time.monotonic() + interval_s
            while time.monotonic() < end and not self.stop_event.is_set():
                time.sleep(0.01)

        self.root.after(0, self._on_thread_done)

    def _on_thread_done(self):
        self.running = False
        self.cps = 0.0
        self.status_lbl.config(text="IDLE", fg=C_VIO_LT)
        self.cps_lbl.config(text="0.0 clicks / sec", fg=C_DIM)
        self.count_lbl.config(text=f"Total: {self.click_count}")
        self._update_btns()

    # ── Start / Stop ──────────────────────────────────────────────────────────
    def start_clicking(self):
        if self.running:
            return
        try:
            interval = max(10, int(self.interval_var.get()))
        except Exception:
            interval = 100
        fixed   = self.loc_var.get() == "Fixed screen position"
        fx      = int(self.x_var.get())
        fy      = int(self.y_var.get())
        limited = self.limit_var.get()
        max_n   = int(self.limit_count_var.get())

        self.click_count = 0
        self.cps         = 0.0
        self._cps_t0     = time.monotonic()
        self._cps_n0     = 0
        self.running     = True
        self.stop_event.clear()

        self.click_thread = threading.Thread(
            target=self._click_worker,
            args=(interval, fixed, fx, fy, limited, max_n),
            daemon=True,
        )
        self.click_thread.start()

        self.status_lbl.config(text="RUNNING", fg=C_GREEN)
        self.cps_lbl.config(fg=C_GREEN)
        self._update_btns()

    def stop_clicking(self):
        self.stop_event.set()
        self.running = False
        self.cps     = 0.0
        self.status_lbl.config(text="IDLE", fg=C_VIO_LT)
        self.cps_lbl.config(text="0.0 clicks / sec", fg=C_DIM)
        self._update_btns()

    def _update_btns(self):
        if self.running:
            self.start_btn.config(state="disabled", bg=C_VIO_DK)
            self.stop_btn.config(state="normal",    bg=C_RED_DK)
        else:
            self.start_btn.config(state="normal",   bg="#6e46dc")
            self.stop_btn.config(state="disabled",  bg="#3a1010")

    # ── Hotkey listener ───────────────────────────────────────────────────────
    def _start_hotkey_listener(self):
        if not PYNPUT_OK:
            return
        if self._hk_listener:
            try:
                self._hk_listener.stop()
            except Exception:
                pass

        target = HOTKEY_MAP.get(self.hotkey_var.get(), pkeyboard.Key.f6)

        def on_press(key):
            if key == target:
                if self.running:
                    self.root.after(0, self.stop_clicking)
                else:
                    self.root.after(0, self.start_clicking)

        self._hk_listener = pkeyboard.Listener(on_press=on_press)
        self._hk_listener.daemon = True
        self._hk_listener.start()

    # ── Periodic UI update ────────────────────────────────────────────────────
    def _tick(self):
        if self.running:
            now = time.monotonic()
            dt  = now - self._cps_t0
            if dt >= 0.6:
                delta     = self.click_count - self._cps_n0
                self.cps  = delta / dt
                self._cps_n0 = self.click_count
                self._cps_t0 = now
                self.cps_lbl.config(text=f"{self.cps:.1f} clicks / sec")
                self.count_lbl.config(text=f"Total: {self.click_count}")
        self.root.after(100, self._tick)

    # ── Misc ──────────────────────────────────────────────────────────────────
    def _warn_no_pynput(self):
        warn = tk.Label(
            self.root,
            text="⚠ pynput not installed – clicking disabled\n"
                 "Run:  pip install pynput",
            font=("Segoe UI", 9), fg="#f5a623", bg="#1e1200",
            pady=4,
        )
        warn.pack(fill="x")

    def on_close(self):
        self.stop_clicking()
        if self._hk_listener:
            try:
                self._hk_listener.stop()
            except Exception:
                pass
        self.root.destroy()


# ── Entry point ────────────────────────────────────────────────────────────────
def main():
    root = tk.Tk()
    app  = SiminAutoClicker(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()