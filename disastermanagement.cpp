#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

// ------------------------- CITY GRAPH -------------------------
class CityGraph {
public:
    struct Edge {
        int to;
        int distance;
    };

private:
    vector<string> names;
    vector<vector<Edge>> adj;
    unordered_map<string, int> id;

public:
    int addLocation(const string& name) {
        auto it = id.find(name);
        if (it != id.end()) return it->second;

        int newId = static_cast<int>(names.size());
        names.push_back(name);
        adj.emplace_back();
        id[name] = newId;
        return newId;
    }

    bool hasLocation(const string& name) const {
        return id.find(name) != id.end();
    }

    int getId(const string& name) const {
        auto it = id.find(name);
        return (it == id.end()) ? -1 : it->second;
    }

    string getName(int locationId) const {
        if (locationId < 0 || locationId >= static_cast<int>(names.size()))
            return "Unknown";
        return names[locationId];
    }

    int size() const {
        return static_cast<int>(names.size());
    }

    void addRoad(const string& a, const string& b, int distance) {
        if (distance <= 0) {
            cout << "Distance must be positive.\n";
            return;
        }

        int u = addLocation(a);
        int v = addLocation(b);
        adj[u].push_back({v, distance});
        adj[v].push_back({u, distance});
    }

    bool closeRoad(const string& a, const string& b) {
        int u = getId(a);
        int v = getId(b);
        if (u == -1 || v == -1) return false;

        auto removeEdge = [](vector<Edge>& edges, int target) {
            auto oldSize = edges.size();
            edges.erase(
                remove_if(edges.begin(), edges.end(),
                          [target](const Edge& e) { return e.to == target; }),
                edges.end()
            );
            return edges.size() != oldSize;
        };

        bool first = removeEdge(adj[u], v);
        bool second = removeEdge(adj[v], u);
        return first || second;
    }

    // Dijkstra: returns shortest distance and fills the route.
    int shortestPath(int source, int destination, vector<int>& route) const {
        const int INF = numeric_limits<int>::max() / 4;
        int n = size();

        if (source < 0 || destination < 0 || source >= n || destination >= n)
            return INF;

        vector<int> dist(n, INF);
        vector<int> parent(n, -1);

        using State = pair<int, int>; // {distance, vertex}
        priority_queue<State, vector<State>, greater<State>> pq;

        dist[source] = 0;
        pq.push({0, source});

        while (!pq.empty()) {
            int currentDistance = pq.top().first;
            int u=pq.top().second;
            pq.pop();

            if (currentDistance != dist[u]) continue;
            if (u == destination) break;

            for (const Edge& edge : adj[u]) {
                int v = edge.to;
                int newDistance = currentDistance + edge.distance;

                if (newDistance < dist[v]) {
                    dist[v] = newDistance;
                    parent[v] = u;
                    pq.push({newDistance, v});
                }
            }
        }

        route.clear();
        if (dist[destination] == INF) return INF;

        for (int at = destination; at != -1; at = parent[at])
            route.push_back(at);

        reverse(route.begin(), route.end());
        return dist[destination];
    }

    void display() const {
        cout << "\n--- CITY ROAD NETWORK (distance in km) ---\n";
        for (int u = 0; u < size(); ++u) {
            cout << names[u] << " -> ";
            if (adj[u].empty()) {
                cout << "(no open roads)";
            } else {
                for (const Edge& e : adj[u])
                    cout << names[e.to] << " (" << e.distance << " km)  ";
            }
            cout << '\n';
        }
    }
};

// ------------------------- INCIDENT -------------------------
struct Incident {
    int id{};
    int locationId{};
    int severity{};       // 4 = Critical, 1 = Low
    int peopleAffected{};
    string description;
    bool dispatched = false;
    bool resolved = false;
    int assignedTeamId = -1;
};

struct IncidentCompare {
    bool operator()(const Incident& a, const Incident& b) const {
        if (a.severity != b.severity)
            return a.severity < b.severity; // higher severity first
        return a.id > b.id;                 // earlier report first
    }
};

// ------------------------- RESCUE TEAM -------------------------
struct RescueTeam {
    int id{};
    string name;
    int locationId{};
    bool available = true;
    int currentIncidentId = -1;
};

// ------------------------- RESOURCES -------------------------
struct Supplies {
    int medicalKits = 100;
    int foodPackets = 300;
    int waterBottles = 300;
};

struct Allocation {
    int medicalKits = 0;
    int foodPackets = 0;
    int waterBottles = 0;
};

Allocation allocateSupplies(Supplies& stock, int people, int severity) {
    // Example policy for a classroom simulation, not real emergency guidance.
    int medicalNeed = people / 10 + severity * 2;
    int foodNeed = people * 2;
    int waterNeed = people * 2;

    Allocation given;
    given.medicalKits = min(stock.medicalKits, medicalNeed);
    given.foodPackets = min(stock.foodPackets, foodNeed);
    given.waterBottles = min(stock.waterBottles, waterNeed);

    stock.medicalKits -= given.medicalKits;
    stock.foodPackets -= given.foodPackets;
    stock.waterBottles -= given.waterBottles;

    return given;
}

// ------------------------- APPLICATION -------------------------
class DisasterRescueSystem {
private:
    CityGraph city;
    vector<Incident> incidents;
    priority_queue<Incident, vector<Incident>, IncidentCompare> pending;
    vector<RescueTeam> teams;
    Supplies stock;
    int nextIncidentId = 1;

    int readInt(const string& prompt, int minimum, int maximum) {
        int value;
        while (true) {
            cout << prompt;
            if (cin >> value && value >= minimum && value <= maximum)
                return value;

            cout << "Invalid input. Enter a number from "
                 << minimum << " to " << maximum << ".\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }

    string readLine(const string& prompt) {
        cout << prompt;
        string value;
        getline(cin >> ws, value);
        return value;
    }

    int findIncidentIndex(int incidentId) const {
        for (int i = 0; i < static_cast<int>(incidents.size()); ++i)
            if (incidents[i].id == incidentId) return i;
        return -1;
    }

    void loadSampleCity() {
        city.addRoad("Central", "North", 4);
        city.addRoad("Central", "South", 5);
        city.addRoad("Central", "East", 3);
        city.addRoad("North", "West", 6);
        city.addRoad("East", "South", 2);
        city.addRoad("West", "South", 7);
        city.addRoad("North", "East", 5);

        teams.push_back({1, "Rescue Team Alpha", city.getId("Central"), true, -1});
        teams.push_back({2, "Rescue Team Bravo", city.getId("North"), true, -1});
        teams.push_back({3, "Rescue Team Charlie", city.getId("East"), true, -1});
    }

    void reportIncident() {
        cout << "\nAvailable locations:\n";
        for (int i = 0; i < city.size(); ++i)
            cout << " - " << city.getName(i) << '\n';

        string location = readLine("Enter affected location: ");
        int locationId = city.getId(location);

        if (locationId == -1) {
            cout << "Location not found. Use a location listed above.\n";
            return;
        }

        cout << "\nSeverity: 1=Low, 2=Medium, 3=High, 4=Critical\n";
        int severity = readInt("Enter severity: ", 1, 4);
        int people = readInt("Enter estimated people affected: ", 1, 1000000);
        string description = readLine("Enter incident description: ");

        Incident incident;
        incident.id = nextIncidentId++;
        incident.locationId = locationId;
        incident.severity = severity;
        incident.peopleAffected = people;
        incident.description = description;

        incidents.push_back(incident);
        pending.push(incident);

        cout << "Incident #" << incident.id
             << " registered and added to the priority queue.\n";
    }

    void showPendingIncidents() const {
        auto copy = pending;
        cout << "\n--- PENDING INCIDENTS (priority order) ---\n";

        bool found = false;
        while (!copy.empty()) {
            Incident incident = copy.top();
            copy.pop();

            int index = findIncidentIndex(incident.id);
            if (index == -1 || incidents[index].dispatched ||
                incidents[index].resolved)
                continue;

            found = true;
            cout << "ID: " << incident.id
                 << " | Location: " << city.getName(incident.locationId)
                 << " | Severity: " << incident.severity
                 << " | People affected: " << incident.peopleAffected
                 << " | Description: " << incident.description << '\n';
        }

        if (!found) cout << "No pending incidents.\n";
    }

    void dispatchNextIncident() {
        while (!pending.empty()) {
            Incident candidate = pending.top();
            pending.pop();

            int incidentIndex = findIncidentIndex(candidate.id);
            if (incidentIndex == -1 || incidents[incidentIndex].dispatched ||
                incidents[incidentIndex].resolved)
                continue;

            Incident& incident = incidents[incidentIndex];

            int bestTeamIndex = -1;
            int bestDistance = numeric_limits<int>::max();
            vector<int> bestRoute;

            for (int i = 0; i < static_cast<int>(teams.size()); ++i) {
                if (!teams[i].available) continue;

                vector<int> route;
                int distance = city.shortestPath(
                    teams[i].locationId, incident.locationId, route
                );

                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestTeamIndex = i;
                    bestRoute = route;
                }
            }

            if (bestTeamIndex == -1) {
                cout << "No rescue team is currently available.\n";
                pending.push(candidate);
                return;
            }

            if (bestDistance >= numeric_limits<int>::max() / 4) {
                cout << "No open route to " << city.getName(incident.locationId)
                     << ". Incident remains pending.\n";
                pending.push(candidate);
                return;
            }

            RescueTeam& team = teams[bestTeamIndex];
            team.available = false;
            team.currentIncidentId = incident.id;
            incident.dispatched = true;
            incident.assignedTeamId = team.id;

            Allocation allocation = allocateSupplies(
                stock, incident.peopleAffected, incident.severity
            );

            cout << "\n--- DISPATCH CONFIRMED (SIMULATION) ---\n";
            cout << "Incident: #" << incident.id << '\n';
            cout << "Assigned team: " << team.name << '\n';
            cout << "Route: ";
            for (size_t i = 0; i < bestRoute.size(); ++i) {
                if (i) cout << " -> ";
                cout << city.getName(bestRoute[i]);
            }
            cout << "\nRoute distance: " << bestDistance << " km\n";
            cout << "Supplies allocated: "
                 << allocation.medicalKits << " medical kits, "
                 << allocation.foodPackets << " food packets, "
                 << allocation.waterBottles << " water bottles\n";
            cout << "Mark the incident resolved from the main menu when the "
                    "simulation is complete.\n";
            return;
        }

        cout << "There are no pending incidents to dispatch.\n";
    }

    void resolveIncident() {
        int id = readInt("Enter incident ID to mark resolved: ", 1, 1000000000);
        int index = findIncidentIndex(id);

        if (index == -1) {
            cout << "Incident ID not found.\n";
            return;
        }

        Incident& incident = incidents[index];
        if (!incident.dispatched) {
            cout << "This incident has not been dispatched.\n";
            return;
        }
        if (incident.resolved) {
            cout << "This incident is already resolved.\n";
            return;
        }

        incident.resolved = true;

        for (RescueTeam& team : teams) {
            if (team.id == incident.assignedTeamId) {
                team.available = true;
                team.locationId = incident.locationId;
                team.currentIncidentId = -1;
                break;
            }
        }

        cout << "Incident #" << id << " marked resolved. The assigned team "
                "is now available at the incident location.\n";
    }

    void closeRoad() {
        string a = readLine("Enter first location: ");
        string b = readLine("Enter second location: ");

        if (city.closeRoad(a, b))
            cout << "Road closed in the simulation. Routes will be recalculated.\n";
        else
            cout << "Road not found or already closed.\n";
    }

    void showTeams() const {
        cout << "\n--- RESCUE TEAMS ---\n";
        for (const RescueTeam& team : teams) {
            cout << team.id << ". " << team.name
                 << " | Location: " << city.getName(team.locationId)
                 << " | Status: " << (team.available ? "Available" : "Busy");
            if (!team.available)
                cout << " | Incident: #" << team.currentIncidentId;
            cout << '\n';
        }
    }

    void showResources() const {
        cout << "\n--- REMAINING SUPPLIES (SIMULATION) ---\n";
        cout << "Medical kits: " << stock.medicalKits << '\n';
        cout << "Food packets: " << stock.foodPackets << '\n';
        cout << "Water bottles: " << stock.waterBottles << '\n';
    }

    void showAllIncidents() const {
        cout << "\n--- ALL INCIDENTS ---\n";
        if (incidents.empty()) {
            cout << "No incidents registered.\n";
            return;
        }

        for (const Incident& incident : incidents) {
            string status = incident.resolved ? "Resolved" :
                            incident.dispatched ? "Dispatched" : "Pending";

            cout << "#" << incident.id
                 << " | " << city.getName(incident.locationId)
                 << " | Severity " << incident.severity
                 << " | " << incident.peopleAffected << " people"
                 << " | " << status << '\n';
        }
    }

public:
    DisasterRescueSystem() {
        loadSampleCity();
    }

    void run() {
        cout << "===============================================\n";
        cout << " SMART DISASTER RESCUE & RESOURCE ALLOCATION\n";
        cout << "              C++ DSA SIMULATOR\n";
        cout << "===============================================\n";

        while (true) {
            cout << "\nMAIN MENU\n"
                 << "1. Display city road network\n"
                 << "2. Report a new disaster incident\n"
                 << "3. View pending incidents by priority\n"
                 << "4. Dispatch next incident\n"
                 << "5. Mark incident as resolved\n"
                 << "6. Close a road\n"
                 << "7. View rescue teams\n"
                 << "8. View remaining resources\n"
                 << "9. View all incidents\n"
                 << "0. Exit\n";

            int choice = readInt("Choose an option: ", 0, 9);

            switch (choice) {
                case 1: city.display(); break;
                case 2: reportIncident(); break;
                case 3: showPendingIncidents(); break;
                case 4: dispatchNextIncident(); break;
                case 5: resolveIncident(); break;
                case 6: closeRoad(); break;
                case 7: showTeams(); break;
                case 8: showResources(); break;
                case 9: showAllIncidents(); break;
                case 0:
                    cout << "Exiting simulator. Thank you!\n";
                    return;
            }
        }
    }
};

int main() {
    DisasterRescueSystem application;
    application.run();
    return 0;
}
