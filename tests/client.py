import queue
import re
import subprocess
import sys
import threading
from dataclasses import dataclass
from typing import Callable

_RES_RE = re.compile(r"^RES\s+(OK|ERR)\s*(.*)?$")
_EVT_RE = re.compile(r"^EVT\s+(\w+)\s*(.*)$")


@dataclass
class CmdResult:
    ok: bool
    payload: dict[str, str]


@dataclass
class Event:
    kind: str
    payload: dict[str, str]


class StubProc:
    def __init__(
        self,
        cmd: list[str] = ["./build/stub/stub_device"],
        joined: bool = True,
        freeze_time: bool = True,
        device_config: str | None = None,
    ) -> None:
        self.cmd = [*cmd]
        if device_config:
            self.cmd += ["--device-config", device_config]
        if not joined:
            self.cmd += ["--not-joined"]
        if freeze_time:
            self.cmd += ["--freeze-time"]
        self.proc: subprocess.Popen | None = None
        self._res_q: queue.Queue[CmdResult] = queue.Queue()
        self._evt_q: queue.Queue[Event] = queue.Queue()
        self._reader_thread: threading.Thread | None = None
        self._stderr_forward_thread: threading.Thread | None = None
        self.on_event: list[Callable[[Event], None]] = []

    def __enter__(self) -> "StubProc":
        return self.start()

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.stop()

    def start(self) -> "StubProc":
        self.proc = subprocess.Popen(
            self.cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        self._reader_thread = threading.Thread(target=self._read_loop, daemon=True)
        self._reader_thread.start()
        self._stderr_forward_thread = threading.Thread(
            target=self._stderr_forward, daemon=True
        )
        self._stderr_forward_thread.start()
        # enter machine mode; ignore output but ensure we’re synced
        self.exec("machine on", timeout=2.0)
        return self

    def stop(self, kill_after: float = 0.02) -> None:
        if not self.proc or not self.is_running():
            return
        try:
            self.exec("q", timeout=0.2)
        except Exception:
            pass
        try:
            self.proc.wait(timeout=kill_after)
        except subprocess.TimeoutExpired:
            self.proc.kill()
            self.proc.wait()
        self.proc = None

    def is_running(self) -> bool:
        return self.proc is not None and self.proc.poll() is None

    def wait_for_exit(self, timeout: float = 2.0) -> bool:
        if not self.proc:
            return True
        try:
            self.proc.wait(timeout=timeout)
            return True
        except subprocess.TimeoutExpired:
            return False

    def _stderr_forward(self):
        assert self.proc and self.proc.stderr

        for line in self.proc.stderr:
            print(line.rstrip(), file=sys.stderr)

    def _read_loop(self) -> None:
        assert self.proc and self.proc.stdout and self.proc.stderr

        for raw in self.proc.stdout:
            line = raw.rstrip("\n")
            print(line.rstrip(), file=sys.stdout)
            if "RES" in line:
                print("!")  # mark RES lines in output
            m = _RES_RE.match(line)
            if m:
                ok = m.group(1) == "OK"
                payload = m.group(2)
                self._res_q.put(
                    CmdResult(
                        ok=ok, payload=self._parse_kw_from_firmware(payload or "")
                    )
                )
                continue
            m = _EVT_RE.match(line)
            if m:
                event = Event(
                    kind=m.group(1),
                    payload=self._parse_kw_from_firmware(m.group(2) or ""),
                )
                for cb in self.on_event:
                    cb(event)
                continue
            # Ignore unknown lines

    def _parse_kw_from_firmware(self, data: str) -> dict[str, str]:
        result = {}
        tokens = data.split()
        for token in tokens:
            if "=" in token:
                key, value = token.split("=", 1)
                result[key] = value
            else:
                result[token] = ""
        return result

    def exec(self, cmd: str, timeout: float = 1.0) -> CmdResult:
        if not self.proc or not self.proc.stdin:
            raise RuntimeError("Process not running")
        while not self._res_q.empty():
            try:
                self._res_q.get_nowait()
            except queue.Empty:
                break
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()
        try:
            return self._res_q.get(timeout=timeout)
        except queue.Empty:
            raise TimeoutError(f"Timeout waiting for response to: {cmd}")
