from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
source = root / "work/uxplay"
expected = "82ad05ce34ec862851fa6ab1240f50f58fb7fa5a"
revision = subprocess.check_output(["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
if revision != expected:
    raise SystemExit("Unexpected UxPlay revision: " + revision)
path = source / "uxplay.cpp"
text = subprocess.check_output(["git", "-C", str(source), "show", "HEAD:uxplay.cpp"]).decode()


def replace(old, new):
    global text
    if text.count(old) != 1:
        raise RuntimeError("Patch context missing or ambiguous: " + old[:90])
    text = text.replace(old, new)


replace('extern "C" void audio_process (void *cls, raop_ntp_t *ntp, audio_decode_struct *data) {',
        '''// Air2Win bridge: only report streaming when a media packet is received.
static void air2win_media_received() {
    if (g_atomic_int_compare_and_exchange(&air2win_streaming, 0, 1)) {
        fprintf(stdout, "AIR2WIN_STREAMING\\n");
        fflush(stdout);
    }
}
extern "C" void audio_process (void *cls, raop_ntp_t *ntp, audio_decode_struct *data) {
    air2win_media_received();''')
replace('extern "C" void video_process (void *cls, raop_ntp_t *ntp, video_decode_struct *data) {',
        'extern "C" void video_process (void *cls, raop_ntp_t *ntp, video_decode_struct *data) {\n    air2win_media_received();')
replace('    if (open_connections == 0) {\n        remote_clock_offset = 0;',
        '''    if (open_connections == 0) {
        g_atomic_int_set(&air2win_streaming, 0);
        fprintf(stdout, "AIR2WIN_IDLE\\n");
        fflush(stdout);
        remote_clock_offset = 0;''')
replace('    std::vector<char> server_hw_addr;\n    std::string config_file = "";',
        '''    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    std::vector<char> server_hw_addr;
    std::string config_file = "";''')
replace('''    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
        LOGE("Could not set control handler");
        exit(1);
    }''', '''    // GUI-owned helper has no console. Shutdown is also accepted on stdin.
    SetConsoleCtrlHandler(CtrlHandler, TRUE);''')
replace('static void main_loop()  {', '''static gint air2win_streaming = 0;
#ifdef _WIN32
static gboolean air2win_poll_control(gpointer) {
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD available = 0;
    if (PeekNamedPipe(input, NULL, 0, NULL, &available, NULL) && available >= 5) {
        char command[5];
        DWORD received = 0;
        if (ReadFile(input, command, 5, &received, NULL) && received == 5 &&
            memcmp(command, "stop\\n", 5) == 0) {
            CtrlHandler(CTRL_C_EVENT);
            return G_SOURCE_REMOVE;
        }
    }
    return G_SOURCE_CONTINUE;
}
#endif

static void main_loop()  {''')
replace('''    reconnect:
    compression_type = 0;''', '''#ifdef _WIN32
    g_timeout_add(100, air2win_poll_control, NULL);
#endif
    fprintf(stdout, "AIR2WIN_READY\\n");
    fflush(stdout);
    reconnect:
    compression_type = 0;''')
replace('    raop_start_httpd(raop, &raop_port);', '''    if (raop_start_httpd(raop, &raop_port) < 0) {
        LOGE("Air2Win: cannot open the receiver control port");
        exit(1);
    }''')
path.write_text(text, encoding="utf-8", newline="\n")
print("Patched UxPlay " + expected)
