#define CPPHTTPLIB_FORM_URL_ENCODED_PAYLOAD_MAX_LENGTH 1024 * 1024 * 255

#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include "nlohmann/json.hpp"

#include "httplib.h"
#include <jwt-cpp/jwt.h>

#include <chrono>
#include <iostream>
#include <Windows.h>
#include <string_view>
#include <string>
#include <iterator>
#include <vector>
#include <string_view>

using json = nlohmann::json;

std::vector<int> weekday{3,4,5,6,0,1,2};
std::vector<std::string> oddeven_week{ "Нечетная","Четная" };
std::vector<std::pair<long long, long long>> lessons_time{ {28800,35400}, {35400,41400}, {41400,48000}, {48000,54000}, {54000,60000}, {60000,66000}, {66000,72000}, {72000,77400} };

const std::string SECRET = "7777777";

mongocxx::instance inst{};
auto uri = mongocxx::uri{ "mongodb+srv://Cubik:RUSqYL2MfLMUiiAq@databasetelebot.lfclo6s.mongodb.net/?retryWrites=true&w=majority" };

mongocxx::client connection{ uri };

auto db = connection["КФУ"];

auto ProgEng = db["ФТИ"];

//ТОЛЬКО ДЛЯ ФТИ!

std::string sched_json_to_string(json param_json, int sub_group) {
    std::string final_str = "";
    for (auto& day: param_json.items()) {
        for (auto& day1 : day.value().items()) {
            final_str += ("___" + day1.key() + "___\n\n");
            for (auto& lesson : day1.value().items()) {
                if ((lesson.value()["Подгруппа"] != sub_group && lesson.value()["Подгруппа"] != 0) || lesson.value()["Название урока"] == "") { continue; }
                final_str += ("\n___" + to_string(lesson.value()["Номер занятия"]) + "___");
                final_str += ("\n" + to_string(lesson.value()["Название урока"]));
                final_str += (" : " + to_string(lesson.value()["Тип занятия"]));
                if (lesson.value()["Подгруппа"] != 0) { final_str += ("\nПодгруппа: " + to_string(lesson.value()["Подгруппа"])); }
                final_str += ("\n" + to_string(lesson.value()["Местонахождение"]));
                final_str += ("\n" + to_string(lesson.value()["Преподаватель"]));
                if(to_string(lesson.value()["Комментарий"]) != """"){ final_str += ("\nКомментарий: " + to_string(lesson.value()["Комментарий"])); }
                final_str += "\n";
            }
        }
    }
    return final_str;
}

std::string get_schedule_day(std::string group, int curr_day, int sub_group) {
    auto curr_time_passed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + 3 * 3600; //ПО МСК (+ 3 * 3600)
    std::string curr_week = oddeven_week[((curr_time_passed) / 604800) % 2];
    auto cursor = ProgEng.find({});
    for (auto& doc : cursor) {
        json doc_json = json::parse(bsoncxx::to_json(doc));
        for (auto& doc1 : doc_json.items()) {
            if (doc1.key() == "_id") { continue; }
            for (auto& doc2 : doc1.value().items()) {
                for (auto& doc3 : doc2.value().items()) {
                    if (doc3.key().find(curr_week) == std::string::npos) { continue; }
                    if (doc2.key() == group) {
                        for (auto& doc4 : doc3.value().items()) {
                            if (doc4.key() == std::to_string(curr_day)) {
                                return sched_json_to_string(doc4, sub_group);
                            }
                        }
                    }
                }
            }
        }
    }
    return "";
}

std::string get_schedule_next_lesson(std::string group, int sub_group) {
    auto curr_time_passed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + 3 * 3600; //ПО МСК (+ 3 * 3600)
    auto daytime = curr_time_passed % 86400;
    std::string curr_week = oddeven_week[((curr_time_passed) / 604800) % 2];
    auto cursor = ProgEng.find({});
    for (auto& doc : cursor) {
        json doc_json = json::parse(bsoncxx::to_json(doc));
        for (auto& doc1 : doc_json.items()) {
            if (doc1.key() == "_id") { continue; }
            for (auto& doc2 : doc1.value().items()) {
                for (auto& doc3 : doc2.value().items()) {
                    if (doc3.key().find(curr_week) == std::string::npos) { continue; }
                    if (doc2.key() == group) {
                        for (auto& doc4 : doc3.value().items()) {
                            if (doc4.key() == std::to_string(weekday[(curr_time_passed / 86400) % 7])) {
                                for (auto& lessons : doc4.value().items()) {
                                    for (int i = 0; i < lessons_time.size(); i++) {
                                        if (lessons_time[i].first <= daytime && daytime < lessons_time[i].second) {
                                            for (auto& lesson : lessons.value().items()) {
                                                if (lesson.value()["Номер занятия"] == i + 2 && (lesson.value()["Подгруппа"] == sub_group || lesson.value()["Подгруппа"] == 0)) {
                                                    if (lesson.value()["Местонахождение"] = "") { return "Местонахождение неизвестно, либо пары нет!"; }
                                                    return lesson.value()["Местонахождение"];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return "Местонахождение неизвестно, либо пары нет!";
}

std::string get_schedule_that_day(std::string group, int sub_group) {
    auto curr_time_passed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + 3 * 3600; //ПО МСК (+ 3 * 3600)
    auto daytime = curr_time_passed % 86400;
    std::string curr_week = oddeven_week[(curr_time_passed / 604800) % 2];
    auto cursor = ProgEng.find({});
    for (auto& doc : cursor) {
        json doc_json = json::parse(bsoncxx::to_json(doc));
        for (auto& doc1 : doc_json.items()) {
            if (doc1.key() == "_id") { continue; }
            for (auto& doc2 : doc1.value().items()) {
                for (auto& doc3 : doc2.value().items()) {
                    if (doc3.key().find(curr_week) == std::string::npos) { continue; }
                    if (doc2.key() != group) { continue; }
                    for (auto& doc4 : doc3.value().items()) {
                        if (doc4.key() == std::to_string(weekday[(curr_time_passed / 86400) % 7])) {
                            return sched_json_to_string(doc4, sub_group);
                        }
                    }
                }
            }
        }
    }
    return "";
}

std::string get_schedule_next_day(std::string group, int sub_group) {
    auto curr_time_passed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + 3 * 3600; //ПО МСК (+ 3 * 3600)
    auto daytime = curr_time_passed % 86400;
    std::string curr_week = oddeven_week[(curr_time_passed / 604800) % 2];
    auto cursor = ProgEng.find({});
    for (auto& doc : cursor) {
        json doc_json = json::parse(bsoncxx::to_json(doc));
        for (auto& doc1 : doc_json.items()) {
            if (doc1.key() == "_id") { continue; }
            for (auto& doc2 : doc1.value().items()) {
                for (auto& doc3 : doc2.value().items()) {
                    if (doc3.key().find(curr_week) == std::string::npos) { continue; }
                    if (doc2.key() != group) { continue; }
                    for (auto& doc4 : doc3.value().items()) {
                        if (doc4.key() == std::to_string(weekday[((curr_time_passed / 86400) + 1) % 7])) {
                            return sched_json_to_string(doc4, sub_group);
                        }
                    }
                }
            }
        }
    }
    return "";
}

std::string get_locprepod(std::string prepod) {
    auto curr_time_passed = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count() + 3 * 3600; //ПО МСК (+ 3 * 3600)
    auto daytime = curr_time_passed % 86400;
    std::string curr_week = oddeven_week[(curr_time_passed / 604800) % 2];
    auto cursor = ProgEng.find({});
    for (auto& doc : cursor) {
        json doc_json = json::parse(bsoncxx::to_json(doc));
        for (auto& doc1 : doc_json.items()) {
            if (doc1.key() == "_id") {continue;}
            for (auto& doc2 : doc1.value().items()) {
                for (auto& doc3 : doc2.value().items()) {
                    if (doc3.key().find(curr_week) != std::string::npos) {
                        for (auto& doc4 : doc3.value().items()) {   
                            if (doc4.key() == std::to_string(weekday[(curr_time_passed / 86400) % 7])) {
                                for (auto& lessons : doc4.value().items()) {
                                    for (int i = 0; i < lessons_time.size(); i++) {
                                        if (lessons_time[i].first <= daytime && daytime < lessons_time[i].second) {
                                            for (auto& lesson : lessons.value().items()) {
                                                if (lesson.value()["Преподаватель"] == prepod && lesson.value()["Номер занятия"] == i + 1) {
                                                    if (lesson.value()["Местонахождение"] == "") { return "Местонахождение неизвестно, либо пары нет!"; }
                                                    return lesson.value()["Местонахождение"];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return "Местонахождение неизвестно, либо пары нет!";
}

std::string get_locstud(std::string main_group, int sub_group) {
    auto curr_time_passed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + 3 * 3600; //ПО МСК (+ 3 * 3600)
    auto daytime = curr_time_passed % 86400;
    std::string curr_week = oddeven_week[(curr_time_passed / 604800) % 2];
    auto cursor = ProgEng.find({});
    for (auto& doc : cursor) {
        json doc_json = json::parse(bsoncxx::to_json(doc));
        for (auto& doc1 : doc_json.items()) {
            if (doc1.key() == "_id") { continue; }
            for (auto& doc2 : doc1.value().items()) {
                for (auto& doc3 : doc2.value().items()) {
                    if (doc2.key() != main_group) { continue; }
                    if (doc3.key().find(curr_week) != std::string::npos) {
                        for (auto& doc4 : doc3.value().items()) {
                            if (doc4.key() == std::to_string(weekday[(curr_time_passed / 86400) % 7])) {
                                for (auto& lessons : doc4.value().items()) {
                                    for (int i = 0; i < lessons_time.size(); i++) {
                                        if (lessons_time[i].first <= daytime && daytime < lessons_time[i].second) {
                                            for (auto& lesson : lessons.value().items()) {
                                                if ((lesson.value()["Подгруппа"] == sub_group || lesson.value()["Подгруппа"] == 0) && lesson.value()["Номер занятия"] == i) {
                                                    if (lesson.value()["Местонахождение"] == "") { return "Местонахождение неизвестно, либо пары нет!";}
                                                    return lesson.value()["Местонахождение"];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return "Местонахождение неизвестно, либо пары нет!";
}

//ПОДГРУППА НЕ ОБЯЗАТЕЛЬНА (ЕСЛИ НАДО, ТО МОЖНО ИЗМЕНИТЬ)
std::string get_comment(std::string comment, int lesson_number, std::string main_group ,int sub_group, int to_weekday, int oddevenweek) {
    bool save_comment = false;
    auto cursor = ProgEng.find({});
    for (auto& doc : cursor) {
        auto smth = json::parse(bsoncxx::to_json(doc));
        for (auto& doc1 : smth.items()) {
            if (doc1.key() == "_id") { continue; }
            for (auto& doc2 : doc1.value().items()) {
                if (doc2.key() != main_group) { continue; }
                for (auto& doc3 : doc2.value().items()) {
                    if (doc3.key().find(oddeven_week[oddevenweek]) == std::string::npos) { continue; }
                    for (auto& doc4 : doc3.value().items()) {
                        if (doc4.key() != std::to_string(to_weekday)) { continue; }
                        for (auto& lessons : doc4.value().items()) {
                            for (auto& lesson : lessons.value().items()) {
                                if (lesson.value()["Номер занятия"] == lesson_number) {
                                    lesson.value()["Комментарий"] = comment;
                                    save_comment = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        if (save_comment) {
            try {
                bsoncxx::document::value upd_doc = bsoncxx::from_json(to_string(smth));
                auto update_one_result = ProgEng.delete_one(doc);
                auto insert_one_resule = ProgEng.insert_one(upd_doc.view());
                return "Комментарий успешно оставлен!";
            }
            catch (...) {
                return "";
            }
        }
    }
    return "Не удалось оставить комментарий!";
}

void get_update(std::string new_sched) {
    bsoncxx::document::value upd_doc = bsoncxx::from_json(new_sched);
    auto cursor = ProgEng.find({});
    json upd_shed = json::parse(new_sched);
    std::string change_str = "";
    for (auto& doc : upd_shed.items()) {
        if (doc.key() == "_id") { continue; }
        change_str = doc.key();
        break;
    }
    for (auto& doc : cursor) {
        auto smth = json::parse(bsoncxx::to_json(doc));
        for (auto& doc1 : smth.items()) {
            if (doc1.key() == "_id") { continue; }
            if (doc1.key().find(change_str) != std::string::npos) {
                try {
                    auto update_one_result = ProgEng.delete_one(doc);
                    auto insert_one_result = ProgEng.insert_one(upd_doc.view());
                    return;
                }
                catch (...) {
                    return;
                }
            }
        }
    }
    try {
        auto insert_one_result = ProgEng.insert_one(upd_doc.view());
        return;
    }
    catch (...) {
        return;
    }
}

int main()
{
    SetConsoleOutputCP(65001);

    httplib::Server srv;
    srv.set_payload_max_length(1024 * 1024 * 10);

    srv.Get("/get_change", [](const httplib::Request& req, httplib::Response& res) {
        auto& files = req.params;
        std::string jwtok = files.find("jwtok")->second;
        auto decoded_token = jwt::decode(jwtok);
        std::string final_res = "";
        try {
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{ SECRET });
            verifier.verify(decoded_token);
            auto data = decoded_token.get_payload_claims();
            std::cout << jwtok << "\n\nNEW INFORMATION!\n\n";
            for (auto& dich : data) {
                std::cout << dich.first << " : " << dich.second << '\n';
            }
            std::cout << '\n';
            if (std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count() < data["expires_at"].as_int()) {
                if (data["action"].as_string() == "get_schedule_next_lesson") {
                    final_res = get_schedule_next_lesson(data["main_group"].as_string(), stoi(data["sub_group"].as_string()));
                }
                else if (data["action"].as_string() == "get_schedule_next_day") {
                    final_res = get_schedule_next_day(data["main_group"].as_string(), stoi(data["sub_group"].as_string()));
                }
                else if (data["action"].as_string() == "get_schedule_that_day") {
                    final_res = get_schedule_that_day(data["main_group"].as_string(), stoi(data["sub_group"].as_string()));
                }
                else if (data["action"].as_string() == "0") {
                    final_res = get_schedule_day(data["main_group"].as_string(), 0, stoi(data["sub_group"].as_string()));
                }
                else if (data["action"].as_string() == "1") {
                    final_res = get_schedule_day(data["main_group"].as_string(), 1, stoi(data["sub_group"].as_string()));
                }
                else if (data["action"].as_string() == "2") {
                    final_res = get_schedule_day(data["main_group"].as_string(), 2, stoi(data["sub_group"].as_string()));
                }
                else if (data["action"].as_string() == "3") {
                    final_res = get_schedule_day(data["main_group"].as_string(), 3, stoi(data["sub_group"].as_string()));
                }
                else if (data["action"].as_string() == "4") {
                    final_res = get_schedule_day(data["main_group"].as_string(), 4, stoi(data["sub_group"].as_string()));
                }
                else if (data["action"].as_string() == "getting_prepod_location") {
                    final_res = get_locprepod(data["prepod"].as_string());
                }
                else if (data["action"].as_string() == "getting_students_location") {
                    final_res = get_locstud(data["main_group"].as_string(), stoi(data["sub_group"].as_string()));
                }
                else if (data["action"].as_string() == "getting_prepod_comment") {
                    final_res = get_comment(data["comment"].as_string(), stoi(data["lesson_number"].as_string()), data["main_group"].as_string(), stoi(data["sub_group"].as_string()), stoi(data["weekday"].as_string()) - 1, (stoi(data["oddevenweek"].as_string())+1)%2);
                }
                else {
                    final_res = "";
                }
            }
            else {
                std::cout << "Время действия jwt токена истекло\n";
            }
        }
        catch (...) {
            final_res = "";
            std::cout << "\nERROR\n\n";
        }
        std::cout << "RESULT\n" << final_res << "\n\n";
        res.set_content(final_res, "text/plain");
        });

    srv.Post("/sched_update", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string jwtok = req.get_param_value("jwtok");
            auto decoded_token = jwt::decode(jwtok);
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{ SECRET });
            verifier.verify(decoded_token);
            auto data = decoded_token.get_payload_claims();
            std::cout << jwtok << "\n\nNEW INFORMATION!\n\n";
            for (auto& dich : data) {
                std::cout << dich.first << " : " << dich.second << '\n';
            }
            std::cout << '\n';
            if (std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count() < data["expires_at"].as_int()) {
                get_update(data["sched"].as_string());
            }
        }
        catch (...) {
            std::cout << "\nERROR\n\n";
        }
        });

    srv.listen("26.127.253.74", 8082);
}
