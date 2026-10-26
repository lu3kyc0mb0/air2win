import os
from pathlib import Path
import queue
import socket
import subprocess
import threading
import time

root = Path(__file__).resolve().parents[1]
runtime = root / "dist/Air2Win/runtime"
data = root / "work/runtime-check"
data.mkdir(exist_ok=True)
(data / "uxplayrc").write_text("")
env = os.environ.copy()
env["PATH"] = str(runtime) + ";" + str(Path(os.environ["SystemRoot"]) / "System32")
env["GST_PLUGIN_PATH_1_0"] = str(runtime / "plugins")
env["GST_PLUGIN_SYSTEM_PATH_1_0"] = str(runtime / "plugins")
env["GST_REGISTRY_1_0"] = str(data / "registry.bin")
env["GST_REGISTRY_FORK"] = "no"
env["XDG_CACHE_HOME"] = str(data / "cache")
env["XDG_CONFIG_HOME"] = str(data)
for feature in ["h264parse", "decodebin", "avdec_h264", "avdec_aac", "avdec_alac",
                "d3d11videosink", "wasapisink", "level", "audioconvert", "audioresample"]:
    result = subprocess.run([str(runtime / "gst-inspect-1.0.exe"), feature], env=env,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    if result.returncode:
        raise RuntimeError(feature + ": " + result.stdout.decode(errors="replace"))
print("Required media elements: OK", flush=True)

command = [
    str(runtime / "uxplay.exe"), "-n", "Air2Win Check", "-nh", "-p", "7100",
    "-rc", str(data / "uxplayrc"), "-key", str(data / "identity.key"),
    "-pw", "1234", "-vs", "d3d11videosink", "-as", "wasapisink"
]
process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    cwd=data, env=env, creationflags=subprocess.CREATE_NO_WINDOW)
lines = queue.Queue()


def read_lines():
    for line in process.stdout:
        lines.put(line.decode(errors="replace").strip())


threading.Thread(target=read_lines, daemon=True).start()
transcript = []
try:
    deadline = time.monotonic() + 15
    ready = False
    while time.monotonic() < deadline:
        try:
            line = lines.get(timeout=0.2)
            transcript.append(line)
            if line == "AIR2WIN_READY":
                ready = True
                break
        except queue.Empty:
            if process.poll() is not None:
                break
    if not ready:
        raise RuntimeError("No readiness event:\n" + "\n".join(transcript))
    # Actual socket round trip checks more than a process/log marker.
    # -p N assigns mirror=N, control=N+1, auxiliary=N+2 in this UxPlay revision.
    with socket.create_connection(("127.0.0.1", 7101), timeout=3) as connection:
        connection.sendall(b"OPTIONS * RTSP/1.0\r\nCSeq: 1\r\n\r\n")
        response = connection.recv(8192)
        if b"200 OK" not in response or b"CSeq: 1" not in response:
            raise RuntimeError("Unexpected RTSP reply: " + repr(response))
    print("Loopback RTSP OPTIONS: 200 OK", flush=True)
    process.stdin.write(b"stop\n")
    process.stdin.flush()
    code = process.wait(timeout=5)
    if code != 0:
        raise RuntimeError("Receiver did not shut down cleanly: " + str(code))
    print("Graceful receiver shutdown: OK", flush=True)
finally:
    if process.poll() is None:
        process.kill()
        process.wait(timeout=5)

with socket.socket() as occupied:
    occupied.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
    occupied.bind(("0.0.0.0", 7101))
    occupied.listen(1)
    failed = subprocess.run(command, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, cwd=data, env=env, timeout=10,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    if failed.returncode == 0 or b"AIR2WIN_READY" in failed.stdout:
        raise RuntimeError("Occupied port was incorrectly reported ready")
print("Occupied port fails without false readiness: OK", flush=True)
