#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

struct Worker {
    std::string id, name, phone, location, occupation, experience, skills, status, workplace;
};
struct Feedback {
    std::string workerId, workerName, ownerName, business, rating, comment, date;
};
std::vector<Worker> workers;
std::vector<Feedback> feedbacks;
int nextId = 1001;

std::string urlDecode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '+') out += ' ';
        else if (s[i] == '%' && i + 2 < s.size()) {
            try {
                out += static_cast<char>(std::stoi(s.substr(i + 1, 2), nullptr, 16));
                i += 2;
            } catch (...) { out += s[i]; }
        } else out += s[i];
    }
    return out;
}

std::map<std::string, std::string> parseForm(const std::string& body) {
    std::map<std::string, std::string> result;
    std::stringstream ss(body);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos)
            result[urlDecode(pair.substr(0, pos))] = urlDecode(pair.substr(pos + 1));
    }
    return result;
}

std::string jsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        if (c == '\n') out += "\\n";
        else if (c != '\r') out += c;
    }
    return out;
}

std::string workersJson() {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < workers.size(); ++i) {
        const auto& w = workers[i];
        if (i) out << ",";
        out << "{\"id\":\"" << jsonEscape(w.id)
            << "\",\"name\":\"" << jsonEscape(w.name)
            << "\",\"phone\":\"" << jsonEscape(w.phone)
            << "\",\"location\":\"" << jsonEscape(w.location)
            << "\",\"occupation\":\"" << jsonEscape(w.occupation)
            << "\",\"experience\":\"" << jsonEscape(w.experience)
            << "\",\"skills\":\"" << jsonEscape(w.skills)
            << "\",\"status\":\"" << jsonEscape(w.status) << "\",\"workplace\":\"" << jsonEscape(w.workplace) << "\"}";
    }
    out << "]";
    return out.str();
}

std::string feedbackJson() {
    std::ostringstream out; out << "[";
    for (size_t i=0;i<feedbacks.size();++i) {
        const auto& f=feedbacks[i]; if(i) out << ",";
        out << "{\"workerId\":\"" << jsonEscape(f.workerId)
            << "\",\"workerName\":\"" << jsonEscape(f.workerName)
            << "\",\"ownerName\":\"" << jsonEscape(f.ownerName)
            << "\",\"business\":\"" << jsonEscape(f.business)
            << "\",\"rating\":\"" << jsonEscape(f.rating)
            << "\",\"comment\":\"" << jsonEscape(f.comment)
            << "\",\"date\":\"" << jsonEscape(f.date) << "\"}";
    }
    out << "]"; return out.str();
}

std::string response(const std::string& body, const std::string& type = "application/json") {
    std::ostringstream out;
    out << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: " << type << "; charset=utf-8\r\n"
        << "Access-Control-Allow-Origin: *\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n\r\n" << body;
    return out.str();
}

void sendAll(int client, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = send(client, data.data() + sent, data.size() - sent, 0);
        if (n <= 0) break;
        sent += static_cast<size_t>(n);
    }
}

int main() {
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) { std::cerr << "Could not create server socket\n"; return 1; }

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        std::cerr << "Port 8080 is busy. Close the other server and try again.\n";
        close(server); return 1;
    }
    if (listen(server, 10) < 0) {
        std::cerr << "Could not start listening\n"; close(server); return 1;
    }

    std::cout << "Digital Work Identity System running at http://localhost:8080\n";
    std::cout << "Press Control+C to stop the server.\n";

    while (true) {
        int client = accept(server, nullptr, nullptr);
        if (client < 0) continue;

        std::string request, chunk;
        char buffer[4096];
        while (true) {
            ssize_t n = recv(client, buffer, sizeof(buffer), 0);
            if (n <= 0) break;
            request.append(buffer, static_cast<size_t>(n));
            size_t headerEnd = request.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                size_t contentLength = 0;
                std::string headers = request.substr(0, headerEnd);
                size_t cl = headers.find("Content-Length:");
                if (cl != std::string::npos) {
                    size_t start = cl + 15;
                    while (start < headers.size() && headers[start] == ' ') ++start;
                    contentLength = static_cast<size_t>(std::stoul(headers.substr(start)));
                }
                if (request.size() >= headerEnd + 4 + contentLength) break;
            } else if (request.size() > 100000) break;
        }

        size_t lineEnd = request.find("\r\n");
        std::string firstLine = request.substr(0, lineEnd);
        std::istringstream line(firstLine);
        std::string method, path, version;
        line >> method >> path >> version;
        size_t queryPos = path.find('?');
        if (queryPos != std::string::npos) path = path.substr(0, queryPos);

        size_t headerEnd = request.find("\r\n\r\n");
        std::string body = headerEnd == std::string::npos ? "" : request.substr(headerEnd + 4);
        std::string output;

        if (method == "GET" && path == "/") {
            std::ifstream file("index.html");
            std::stringstream page;
            page << file.rdbuf();
            output = response(page.str(), "text/html");
        } else if (method == "GET" && path == "/api/workers") {
            output = response(workersJson());
        } else if (method == "POST" && path == "/api/register") {
            auto f = parseForm(body);
            if (f["name"].empty() || f["phone"].empty() || f["occupation"].empty()) {
                output = response("{\"ok\":false,\"message\":\"Name, phone and occupation are required.\"}");
            } else {
                Worker w;
                w.id = "DWI" + std::to_string(nextId++);
                w.name = f["name"]; w.phone = f["phone"]; w.location = f["location"];
                w.occupation = f["occupation"]; w.experience = f["experience"];
                w.skills = f["skills"]; w.status = "Pending"; w.workplace = f["workplace"];
                workers.push_back(w);
                output = response("{\"ok\":true,\"id\":\"" + w.id + "\",\"message\":\"Worker registered successfully.\"}");
            }
        } else if (method == "GET" && path == "/api/feedback") {
            output = response(feedbackJson());
        } else if (method == "POST" && path == "/api/feedback") {
            auto f = parseForm(body); Worker* target = nullptr;
            for (auto& w : workers) if (w.id == f["workerId"]) { target = &w; break; }
            if (!target) output = response("{\"ok\":false,\"message\":\"Worker ID not found.\"}");
            else if (f["ownerName"].empty() || f["rating"].empty()) output = response("{\"ok\":false,\"message\":\"Owner name and rating are required.\"}");
            else {
                Feedback fb; fb.workerId=target->id; fb.workerName=target->name; fb.ownerName=f["ownerName"];
                fb.business=f["business"]; fb.rating=f["rating"]; fb.comment=f["comment"]; fb.date=f["date"];
                feedbacks.push_back(fb);
                if (!fb.business.empty()) target->workplace=fb.business;
                output=response("{\"ok\":true,\"message\":\"Owner feedback saved successfully.\"}");
            }
        } else if (method == "POST" && path == "/api/verify") {
            auto f = parseForm(body);
            bool found = false;
            for (auto& w : workers) {
                if (w.id == f["id"]) { w.status = "Verified"; found = true; }
            }
            output = found ? response("{\"ok\":true,\"message\":\"Worker marked as verified.\"}")
                           : response("{\"ok\":false,\"message\":\"Worker ID not found.\"}");
        } else {
            output = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\nConnection: close\r\n\r\nNot Found";
        }

        sendAll(client, output);
        close(client);
    }
    close(server);
    return 0;
}
