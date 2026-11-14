// SPDX-License-Identifier: MIT
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <deque>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>
struct Frame {
    ULONGLONG at;
    double ms;
};
struct Stream {
    DWORD pid = 0;
    std::string name;
    std::deque<Frame> frames;
};
std::map<std::string, Stream> streams;
std::mutex mx;
std::vector<std::string> csv(const std::string &s) {
    std::vector<std::string> a;
    std::string b;
    bool q = false;
    for (char ch : s) {
        if (ch == '"')
            q = !q;
        else if (ch == ',' && !q) {
            a.push_back(b);
            b.clear();
        } else if (ch != '\r')
            b += ch;
    }
    a.push_back(b);
    return a;
}
std::string lower(std::string s) {
    for (auto &ch : s)
        ch = (char)tolower((unsigned char)ch);
    return s;
}
bool exclude(std::string name) {
    name = lower(name);
    const char *bad[] = {"dwm.exe",
                         "sunshine.exe",
                         "rainmeter.exe",
                         "codex.exe",
                         "chrome.exe",
                         "msedge.exe",
                         "firefox.exe",
                         "steam.exe",
                         "steamwebhelper.exe",
                         "explorer.exe",
                         "searchhost.exe",
                         "applicationframehost.exe",
                         "textinputhost.exe",
                         "discord.exe",
                         "obs64.exe",
                         "msedgewebview2.exe",
                         "shellexperiencehost.exe",
                         "startmenuexperiencehost.exe",
                         "systemsettings.exe",
                         "nvidia overlay.exe",
                         "nvcontainer.exe",
                         "windowsterminal.exe",
                         "powershell.exe",
                         "pwsh.exe",
                         "python.exe",
                         "presentmon.exe",
                         "fpsbridge.exe"};
    for (auto *b : bad)
        if (name == b)
            return true;
    return false;
}
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\RainmeterPhoneFPSBridge");
    if (!mutex)
        return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return 0;
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring dir = self;
    dir = dir.substr(0, dir.find_last_of(L"\\/"));
    auto write = [&](double fps, double low, double ft, std::string name) {
        std::ofstream f((dir + L"\\fps-state.tmp").c_str(), std::ios::trunc);
        f << GetTickCount64() << '|' << fps << '|' << low << '|' << ft << '|' << name;
        f.close();
        MoveFileExW((dir + L"\\fps-state.tmp").c_str(), (dir + L"\\fps-state.txt").c_str(),
                    MOVEFILE_REPLACE_EXISTING);
    };
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE rd, wr;
    if (!CreatePipe(&rd, &wr, &sa, 0))
        return 2;
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = wr;
    si.hStdError = wr;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    std::wstring cmd =
        L"\"" + dir +
        L"\\PresentMon.exe\" --output_stdout --no_console_stats --v1_metrics --no_track_display "
        L"--no_track_input --no_track_gpu --session_name RainmeterPhoneFPS --stop_existing_session";
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ji{};
    ji.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job || !SetInformationJobObject(job, JobObjectExtendedLimitInformation, &ji, sizeof(ji)))
        return 3;
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, dir.c_str(), &si, &pi)) {
        write(0, 0, 0, "CAPTURE START FAILED");
        return 2;
    }
    if (!AssignProcessToJobObject(job, pi.hProcess)) {
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(job);
        return 4;
    }
    ResumeThread(pi.hThread);
    CloseHandle(wr);
    CloseHandle(pi.hThread);
    std::thread reader([&] {
        char b[8192];
        DWORD got;
        std::string pending;
        std::map<std::string, int> headers;
        // A bounded local diagnostic log helps distinguish ETW errors from no active game.
        std::ofstream diagnostic((dir + L"\\fps-diagnostics.log").c_str(), std::ios::trunc);
        unsigned diagnosticLines = 0;
        while (ReadFile(rd, b, sizeof(b), &got, nullptr) && got) {
            pending.append(b, got);
            size_t pos;
            while ((pos = pending.find('\n')) != std::string::npos) {
                std::string l = pending.substr(0, pos);
                pending.erase(0, pos + 1);
                if (diagnosticLines++ < 12) {
                    diagnostic << l << '\n';
                    diagnostic.flush();
                }
                if (l.rfind("\xef\xbb\xbf", 0) == 0)
                    l.erase(0, 3);
                auto v = csv(l);
                if (l.rfind("Application,", 0) == 0) {
                    headers.clear();
                    for (size_t i = 0; i < v.size(); i++)
                        headers[v[i]] = static_cast<int>(i);
                    continue;
                }
                if (!headers.count("Application") || !headers.count("ProcessID") ||
                    !headers.count("SwapChainAddress") || !headers.count("MsBetweenPresents") ||
                    v.size() <= static_cast<size_t>(headers["MsBetweenPresents"]))
                    continue;
                try {
                    std::string name = v[headers["Application"]];
                    if (exclude(name))
                        continue;
                    DWORD pid = std::stoul(v[headers["ProcessID"]]);
                    double ms = std::stod(v[headers["MsBetweenPresents"]]);
                    if (ms <= 0 || ms > 1000)
                        continue;
                    std::string key = std::to_string(pid) + ":" + v[headers["SwapChainAddress"]];
                    auto now = GetTickCount64();
                    std::lock_guard<std::mutex> lock(mx);
                    auto &s = streams[key];
                    s.pid = pid;
                    s.name = name;
                    s.frames.push_back({now, ms});
                    while (!s.frames.empty() && now - s.frames.front().at > 30000)
                        s.frames.pop_front();
                } catch (...) {
                }
            }
        }
    });
    ULONGLONG started = GetTickCount64(), lastSkin = started;
    while (WaitForSingleObject(pi.hProcess, 1000) == WAIT_TIMEOUT) {
        auto now = GetTickCount64();
        // Exit when the panel has been unloaded for 30 seconds.
        std::wstring skin = dir.substr(0, dir.find_last_of(L"\\/"));
        bool has = false;
        for (auto name : {L"Blue White.ini", L"Cloud.ini", L"Graphite.ini"})
            if (FindWindowW(L"RainmeterMeterWindow", (skin + L"\\" + name).c_str()))
                has = true;
        if (has)
            lastSkin = now;
        else if (now - lastSkin > 30000)
            break;
        DWORD fg = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &fg);
        double fps = 0, low = 0, ft = 0;
        std::string name;
        size_t best = 0;
        {
            std::lock_guard<std::mutex> lock(mx);
            for (auto it = streams.begin(); it != streams.end();) {
                if (it->second.frames.empty() || now - it->second.frames.back().at > 30000)
                    it = streams.erase(it);
                else
                    ++it;
            }
            bool foregroundFound = false;
            for (auto &[key, s] : streams) {
                if (now - s.frames.back().at > 2500)
                    continue;
                size_t n = 0;
                for (auto &f : s.frames)
                    if (now - f.at < 1500)
                        n++;
                if (n < 3)
                    continue;
                bool isFg = s.pid == fg;
                if (foregroundFound && !isFg)
                    continue;
                if (isFg && !foregroundFound) {
                    best = 0;
                    foregroundFound = true;
                }
                if (n <= best)
                    continue;
                best = n;
                double sum = 0;
                std::vector<double> times;
                for (auto &f : s.frames) {
                    if (now - f.at < 1500)
                        sum += f.ms;
                    times.push_back(f.ms);
                }
                fps = n * 1000.0 / sum;
                ft = sum / n;
                std::sort(times.rbegin(), times.rend());
                size_t slow = std::max<size_t>(1, (size_t)std::ceil(times.size() * .01));
                double worst = 0;
                for (size_t i = 0; i < slow; i++)
                    worst += times[i];
                low = 1000.0 / (worst / slow);
                name = s.name;
            }
        }
        write(fps, low, ft, name);
    }
    CloseHandle(job);
    WaitForSingleObject(pi.hProcess, 3000);
    reader.join();
    CloseHandle(rd);
    CloseHandle(pi.hProcess);
    write(0, 0, 0, "");
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return 0;
}
