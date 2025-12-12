#include <iostream>
#include <vector>
#include <iomanip>
#include <limits>
#include <string>
#include <cmath>
#include <sstream>
using namespace std;

// ANSI COLOR CODES
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

struct Process {
    int id;
    double arrival, burst, remaining;
    double startTime = -1, finishTime = -1;
};

struct Slot {
    double start, end;
    int pid;
    double freq;
    double power;
    double energy;
};

// Power model
double powerForFreq(double f) {
    const double k = 45.0;
    return k * f * f * f;
}

// Frequency labels
string freqLabel(double f) {
    if (f <= 0.55)
        return "\033[30;102m LOW \033[0m";
    if (f <= 0.80)
        return "\033[30;103m MEDIUM \033[0m";
    return "\033[97;101m HIGH \033[0m";
}

string fmt(double x){
    ostringstream s;
    s << fixed << setprecision(2) << x;
    return s.str();
}

int main() {
    int n;
    cout << CYAN << BOLD << "\n\nEnergy-Efficient CPU Scheduling (DVFS + SRTF)\n" << RESET;
    cout << "Enter number of processes: ";
    cin >> n;

    vector<Process> procs(n);
    for (int i = 0; i < n; i++) {
        cout << BLUE << "\nProcess P" << i+1 << ":" << RESET;
        cout << "\n  Arrival Time : ";
        cin >> procs[i].arrival;
        cout << "  Burst Time   : ";
        cin >> procs[i].burst;

        procs[i].id = i+1;
        procs[i].remaining = procs[i].burst;
    }

    cout << "\n";

    double time = 0;
    int completed = 0;
    const double INF = numeric_limits<double>::infinity();
    vector<Slot> timeline;
    double totalEnergy = 0.0;

    cout << MAGENTA << BOLD << "\n========== Scheduling Started ==========\n\n" << RESET;

    while (completed < n) {

        vector<int> ready;
        for (int i = 0; i < n; i++)
            if (procs[i].arrival <= time && procs[i].remaining > 0)
                ready.push_back(i);

        if (ready.empty()) {
            double nextA = INF;
            for (int i = 0; i < n; i++)
                if (procs[i].remaining > 0 && procs[i].arrival > time)
                    nextA = min(nextA, procs[i].arrival);
            time = nextA;
            continue;
        }

        // DVFS frequency selection
        double freq;
        double totalRem = 0;
        for (int r : ready) totalRem += procs[r].remaining;

        if (ready.size() == 1) freq = 0.50;
        else if (totalRem < 3.0) freq = 0.65;
        else if (ready.size() >= 3) freq = 1.00;
        else freq = 0.80;

        // SRTF - choose shortest remaining process
        int pick = ready[0];
        for (int r : ready)
            if (procs[r].remaining < procs[pick].remaining)
                pick = r;

        Process &p = procs[pick];
        if (p.startTime < 0) p.startTime = time;

        double workRate = freq;
        double dt;
        double start = time;

        if (p.remaining <= workRate) {
            dt = p.remaining / workRate;
            time += dt;
            p.remaining = 0;
            p.finishTime = time;
            completed++;
        }
        else {
            dt = 1.0;
            time += dt;
            p.remaining -= workRate;
        }

        double power = powerForFreq(freq);
        double energy = power * dt;
        totalEnergy += energy;

        timeline.push_back({start, time, p.id, freq, power, energy});
    }

    // Summary Table
    cout << MAGENTA << BOLD << "\n========== Process Summary ==========\n\n" << RESET;

    cout << BOLD
         << left << setw(6) << "PID"
         << setw(10) << "Arr"
         << setw(10) << "Burst"
         << setw(12) << "Start"
         << setw(12) << "Finish"
         << setw(12) << "TAT"
         << setw(12) << "WT"
         << RESET << "\n";

    double totalTAT = 0, totalWT = 0;

    for (auto &p : procs) {
        double tat = p.finishTime - p.arrival;
        double wt = tat - p.burst;
        totalTAT += tat;
        totalWT += wt;

        cout << left
             << setw(6) << ("P" + to_string(p.id))
             << setw(10) << fmt(p.arrival)
             << setw(10) << fmt(p.burst)
             << setw(12) << fmt(p.startTime)
             << setw(12) << fmt(p.finishTime)
             << YELLOW << setw(12) << fmt(tat) << RESET
             << CYAN   << setw(12) << fmt(wt)  << RESET
             << "\n";
    }

    cout << "\n";
    cout << GREEN  << "Average Turnaround Time : " << fmt(totalTAT/n) << "\n" << RESET;
    cout << CYAN   << "Average Waiting Time    : " << fmt(totalWT/n) << "\n" << RESET;
    cout << RED    << "Total Energy Consumed   : " << fmt(totalEnergy) << " units\n" << RESET;

    // Clean Gantt Chart (NO BAR)
    cout << MAGENTA << BOLD << "\n========== Gantt Chart ==========\n\n" << RESET;

    for (auto &s : timeline) {
        cout << BOLD "[" << fmt(s.start) << " -> " << fmt(s.end) << "] " << RESET
             << CYAN << "P" << s.pid << RESET
             << "  |  freq: " << freqLabel(s.freq)
             << "  |  Power: " << fmt(s.power)
             << "  |  Energy: " << fmt(s.energy)
             << "\n\n";
    }

    return 0;
}

